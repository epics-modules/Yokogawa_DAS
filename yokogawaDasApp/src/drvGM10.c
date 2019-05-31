#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errlog.h>
#include <ctype.h>

#include <recSup.h>

#include <dbScan.h>
#include <drvSup.h>
#include <devLib.h>
#include <dbAccess.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsExit.h>
#include <epicsThread.h>
//#include <epicsInterrupt.h>

#include <pthread.h>

#include <unistd.h>

#include "drvGM10_comm.h"

////////////////////////////////////

enum { ADDR_SIGNAL, ADDR_MATH, ADDR_COMM, ADDR_CONST, ADDR_VARCONST };
enum { TRIG_INFO, TRIG_CHANNELS, TRIG_MISC, TRIG_STATUS };
enum { MODE_SETTINGS, MODE_COMPUTE, MODE_COMPUTE_CMD, MODE_RECORDING };
enum { MODULE_PRESENCE, MODULE_STRING };

//////////////////////////////////

enum { CH_TYPE_NONE, CH_TYPE_INPUT_ANALOG, CH_TYPE_INPUT_BINARY, 
       CH_TYPE_INPUT_INTEGER, CH_TYPE_OUTPUT_ANALOG, CH_TYPE_OUTPUT_BINARY, 
       CH_TYPE_PID, CH_TYPE_UNKNOWN };

// mode
enum { CH_STATUS_SKIP, CH_STATUS_NORMAL, CH_STATUS_DIFF, CH_STATUS_UNKNOWN };
enum { CH_MODE_RELAY_ALARM, CH_MODE_RELAY_MANUAL, CH_MODE_RELAY_FAIL,
       CH_MODE_RELAY_UNKNOWN=16 };
enum { CH_MODE_DAC_SKIP, CH_MODE_DAC_RETRANS, CH_MODE_DAC_MANUAL, 
       CH_MODE_DAC_UNKNOWN=16 };       

struct channel_info
{
  char ch_status;
  char ch_mode;
  char unit[11];
  char scale;
};


// data_status values; 32 is not number from hardware
enum { VL_NORMAL=0, VL_SKIP=1, VL_POS_OVERRANGE=2, VL_NEG_OVERRANGE=3,
       VL_POS_BURNOUT=4, VL_NEG_BURNOUT=5, VL_AD_ERROR=6, VL_INVALID_DATA=7,
       VL_MATH_NAN=16, VL_COMM_ERROR=17, VL_UNKNOWN=32 };

enum { AL_OFF, AL_HIGH, AL_LOW, AL_DIFF_HIGH, AL_DIFF_LOW, AL_ROC_HIGH, 
       AL_ROC_LOW, AL_DELAY_HIGH, AL_DELAY_LOW };

struct channel_data
{
  char alarm_status;
  char alarm[4];
  char data_status;
  int  value;
};





/////////////////////////

struct expr_info
{
  char on_flag;
  char expr[121];
};


static double scaled_value( int value, char scale)
{
  // scale is only allowed to be 0-6
  double scaler[7] = { 1.0, 0.1, 0.01, 1.0e-3, 1.0e-4, 1.0e-5, 1.0e-6 };

  return ((double) value) * scaler[ (int) scale];
}

/* static int unscaled_value( double value, char scale) */
/* { */
/*   double scaler[7] = { 1.0, 10.0, 100.0, 1.0e3, 1.0e4, 1.0e5, 1.0e6  }; */

/*   return (int) (((double) value) * scaler[ (int) scale]); */
/* } */


/////////////////////////////////////


enum ModType 
  { MOD_TYPE_UNKNOWN, MOD_TYPE_INPUT_ANALOG, MOD_TYPE_INPUT_DIGITAL, 
    MOD_TYPE_INPUT_PULSE, MOD_TYPE_OUTPUT_ANALOG, MOD_TYPE_OUTPUT_DIGITAL, 
    MOD_TYPE_INPUT_OUTPUT_DIGITAL, MOD_TYPE_PID };

struct module
{
  char module_string[17];

  // broken-out information
  int use_flag;
  
  enum ModType type; 
  int channel_number[2];  // input, output
};



/////////////////////////////////////


LOCAL long gm10_report();
LOCAL long gm10_init();

struct 
{
  long number;
  DRVSUPFUN report;
  DRVSUPFUN init;
} drvGM10 = { 2, gm10_report, gm10_init };

epicsExportAddress(drvet,drvGM10);



union datum
  {
    int int_d;
    double flt_d;
    char *str_d;
  };

struct req_pkt
{
  struct dbCommon *precord;
  int cmd_id;
  int channel;
  union datum value;
    
  struct req_pkt *next;
};

static union datum datum_empty( void)
{
  union datum dt;
  dt.int_d = 0;
  return dt;
}
static union datum datum_int( int val)
{
  union datum dt;
  dt.int_d = val;
  return dt;
}
static union datum datum_float( double val)
{
  union datum dt;
  dt.flt_d = val;
  return dt;
}
/* static union datum datum_string( char *val) */
/* { */
/*   union datum dt; */
/*   dt.str_d = strdup(val); */
/*   return dt; */
/* } */

#define MAX_MODULES (10)

#define MAX_MEAS_CHAN (1000)
#define MAX_CALC_CHAN (200)
#define MAX_COMM_CHAN (500)
#define MAX_CONST_CHAN (100)
#define MAX_VARCONST_CHAN (100)

struct error_value
{
  int code;
  int parameter;

  char strings[3][40];
};



struct devqueue
{
  char *name;
  struct gm10_comm comm; // holds network reading and writing stuff

  int error_flag; // nonzero if error
  struct error_value error; // NULL if unknown error and flag is set

  IOSCANPVT channel_ioscanpvt;  // input channels
  IOSCANPVT misc_ioscanpvt;  // constants 
  IOSCANPVT info_ioscanpvt;   // info for channels: prec, egu, ...
  IOSCANPVT status_ioscanpvt; // device status
  IOSCANPVT error_ioscanpvt;  // error messages

  pthread_mutex_t list_lock;
  pthread_cond_t list_cond;
  struct req_pkt *list;

  //////////////////////////

  int settings_mode; 
  int recording_mode; 
  int compute_mode;

  int alarm_flag;


  unsigned char input_poll_time[8]; // changed with each poll
  unsigned char output_poll_time[8]; // changed with each poll

  // channels are inside module structure
  struct module modules[MAX_MODULES];

  unsigned char meas_type[MAX_MEAS_CHAN];
  struct channel_info meas_info[MAX_MEAS_CHAN]; // changed only when not measuring
  struct channel_data meas_data[MAX_MEAS_CHAN]; // changed with each data call


  // calc_info 0-299 are math channels A001-A300
  struct channel_info calc_info[MAX_CALC_CHAN]; // changed only when not measuring
  struct channel_data calc_data[MAX_CALC_CHAN]; // changed with each data call
  struct expr_info calc_expr[MAX_CALC_CHAN]; // A001-A200  120 characters max

  struct channel_info comm_info[MAX_COMM_CHAN];
  struct channel_data comm_data[MAX_COMM_CHAN];
  //  float comm_input[MAX_COMM_CHAN];

  float constant[MAX_CONST_CHAN];
  float varconstant[MAX_VARCONST_CHAN];
};

struct queue_link
{
  struct devqueue *dq;
  struct queue_link *next;
};

// just define head of linked list
static struct queue_link *gm10_queue_list = NULL; 




LOCAL long gm10_init()
{
  return 0;
}

LOCAL long gm10_report(int level)
{
/* Level=0 Display a one line summary for each device */
/* Level=1 Display more information */
/* Level=2 Display a lot of information. It is even permissible to  */
/*     prompt for what is wanted. */

  struct queue_link *qlp;
  struct devqueue *dq;
  
  int cnt;
  
  int i;

  int buflen=20;
  char buffer[20];

  cnt = 0;
  qlp = gm10_queue_list;
  while( qlp != NULL)
    {
      cnt++;
      dq = qlp->dq;

      inet_ntop(AF_INET, &(dq->comm.r_addr.sin_addr), buffer, buflen);
      
      printf("    Yokogawa GM10 #%d: %s @ %s\n", cnt, dq->name, buffer);

      if( level)
        {
          printf("        Modules:\n");

          for( i = 0; i < 6; i++)
            {
              if( !dq->modules[i].use_flag)
                printf("        #%d: empty\n", i+1);
              else
                printf("        #%d: %s\n", i+1, dq->modules[i].module_string);
            }
                     
        }

      qlp = qlp->next;
    }

  return 0;
}




enum { CMD_LOAD_MODULES, CMD_READ_ALL_INFOS, CMD_READ_STATUS, 
       CMD_SET_OPMODE, CMD_SET_COMPUTE, CMD_CLEAR_ERROR,
       CMD_ACKNOWLEDGE_ALARMS,
       CMD_READ_ALL_DATA, CMD_READ_ALL_MISC, CMD_READ_SIGNAL,
       CMD_READ_MATH, CMD_READ_COMM, CMD_READ_CONST, CMD_READ_VARCONST,
       CMD_SET_SIGNAL_OUTPUT, CMD_SET_BINARY_OUTPUT,
       CMD_SET_COMM, CMD_SET_CONST, CMD_SET_VARCONST};


static int qmesg( struct devqueue *dq, dbCommon *precord, int cmd, int channel,
           union datum dt);


// returns rest of string not grabbed
static char *chopstring40( char *dest, char *src)
{
  char *p;

  if( strlen( src) < 40)
    {
      strcpy( dest, src);
      return NULL;
    }

  p = &src[39];
  while( *p != ' ')
    {
      p--;
      if( p == src) // should not really happen
        {
          strncpy( dest, src, 39);
          dest[39] = '\0';

          return &src[39];
        }
    }

  *p = '\0';
  strcpy( dest, src);
  return p + 1;
}


// need to do error handling locally, different between models
static int response_reader( struct devqueue *dq )
{
  int response;

  char *ptr, *eptr;

  dq->error_flag = 0;

  response = gm10_response_reader( &(dq->comm) );
  if( response == RESPONSE_ERROR)
    {
      dq->error_flag = 1;

      // multiple errors (RESPONSE_CHAIN_ERRORS) not handled right now
      if( sscanf( dq->comm.inbuffer, "E1,%d:1:%d\r\n", &(dq->error.code),
                  &(dq->error.parameter)) >= 1)
        {
          sprintf( dq->comm.outbuffer, "_ERR,%d\r\n", dq->error.code);
          gm10_writer( &(dq->comm) );
          response = gm10_response_reader( &(dq->comm) );

          if( response != RESPONSE_ASCII)
            return RESPONSE_INVALID;

          ptr = dq->comm.inbuffer;
          ptr = strchr( ptr, '\'');
          ptr ++;
          eptr = strchr( ptr, '\'');
          *eptr = '\0';

          eptr = chopstring40( dq->error.strings[0], ptr);
          if( eptr == NULL)
            {
              dq->error.strings[1][0] = '\0';
              dq->error.strings[2][0] = '\0';
            }
          else
            {
              eptr = chopstring40( dq->error.strings[1], eptr);
              if( eptr == NULL)
                dq->error.strings[2][0] = '\0';
              else
                chopstring40( dq->error.strings[2], eptr);
            }          

          scanIoRequest(dq->error_ioscanpvt);
          return RESPONSE_ERROR;
        }
      else
        return RESPONSE_INVALID;
    }

  return response;
}


static int load_modules( struct devqueue *dq)
{
  char set_message[MAX_MODULES][17];
  char status_message[MAX_MODULES][17];
  char error_message[MAX_MODULES][17];

  int check[MAX_MODULES];

  char *ptr;

  int which;

  char *c;
  int i, j, module;

  // setting all channels as empty
  for( i = 0; i < MAX_MEAS_CHAN; i++)
    dq->meas_type[i] = CH_TYPE_NONE;

  gm10_simple_writer( &(dq->comm), "FSysConf\r\n" );
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;

  // We are not allowing expandable IO for now (which has different format)
  if( strncmp( ptr, "Unit:00", 7) )
    return 1;
  ptr += 9;

  for( i = 0; i < MAX_MODULES; i++)
    {
      check[i] = 0;
    }
  while( *ptr != 'E')
    {
      which = 10 * (*(ptr++) - '0');
      which += (*(ptr++) - '0');
      ptr++;

      check[which] = 1;

      c = set_message[which];
      for( i = 0; *ptr != ' '; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += (17 - i);

      c = status_message[which];
      for( i = 0; *ptr != ' '; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += (17 - i);

      c = error_message[which];
      while( *ptr != '\r')
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 2;
    }


  for( i = 0; i < MAX_MODULES; i++)
    {
      dq->modules[i].use_flag = 0;
      dq->modules[i].module_string[0] = '\0';
    }
  for( module = 0; module < MAX_MODULES; module++)
    {
      if( !check[module])
        continue;

      // the strings have to match
      if( strcmp( set_message[module], status_message[module] ) )
        continue;
      // need no errors
      if( strcmp( error_message[module], "----------------") )
        continue;
      if( !strcmp( set_message[module], "----------------") )
        continue;

      strcpy( dq->modules[module].module_string, set_message[module] );

      ptr = dq->modules[module].module_string;
      if( strncmp( ptr, "GX90", 4) )  // use only GX90 modules
        continue;
      ptr+=4;

      dq->modules[module].type = MOD_TYPE_UNKNOWN;
      switch( *(ptr++) )
        {
        case 'U':
          if( *(ptr++) == 'T')
            dq->modules[module].type = MOD_TYPE_PID;
          break;
        case 'X':
          switch( *(ptr++) )
            {
            case 'A':
              dq->modules[module].type = MOD_TYPE_INPUT_ANALOG;
              break;
            case 'D':
              dq->modules[module].type = MOD_TYPE_INPUT_DIGITAL;
              break;
            case 'P':
              dq->modules[module].type = MOD_TYPE_INPUT_PULSE;
              break;
            }
          break;
        case 'Y':
          switch( *(ptr++) )
            {
            case 'A':
              dq->modules[module].type = MOD_TYPE_OUTPUT_ANALOG;
              break;
            case 'D':
              dq->modules[module].type = MOD_TYPE_OUTPUT_DIGITAL;
              break;
            }
          break;
        case 'W':
          if( *(ptr++) == 'D')
            dq->modules[module].type = MOD_TYPE_INPUT_OUTPUT_DIGITAL;
          else
            continue;
          break;
        }
      if( *(ptr++) != '-')
        continue;

      switch( dq->modules[module].type)
        {
        case MOD_TYPE_INPUT_ANALOG:
        case MOD_TYPE_INPUT_DIGITAL:
        case MOD_TYPE_INPUT_PULSE:
          dq->modules[module].channel_number[0] = atoi(ptr);
          dq->modules[module].channel_number[1] = 0;
          break;
        case MOD_TYPE_OUTPUT_ANALOG:
        case MOD_TYPE_OUTPUT_DIGITAL:
          dq->modules[module].channel_number[0] = 0;
          dq->modules[module].channel_number[1] = atoi(ptr);
          break;
        case MOD_TYPE_INPUT_OUTPUT_DIGITAL:
          {
            int v;
            v = atoi(ptr);
            dq->modules[module].channel_number[0] = v/100;
            dq->modules[module].channel_number[1] = v%100;
          }
          break;
          ////////////////  Need to figure out what to do with PID!
        case MOD_TYPE_PID: 
        case MOD_TYPE_UNKNOWN: 
          dq->modules[module].channel_number[0] = 0;
          dq->modules[module].channel_number[1] = 0;
          continue;
        }

      dq->modules[module].use_flag = 1;

     
  /*   } */

  /* for( module = 0; module < MAX_MODULES; module++) */
  /*   { */
  /*     if( !dq->modules[module].use_flag) */
  /*       continue; */

      switch( dq->modules[module].type)
        {
        case MOD_TYPE_INPUT_ANALOG:
          for( j = 0; j < dq->modules[module].channel_number[0]; j++)
            dq->meas_type[module*100 + j] = CH_TYPE_INPUT_ANALOG;
          break;
        case MOD_TYPE_INPUT_DIGITAL:
          for( j = 0; j < dq->modules[module].channel_number[0]; j++)
            dq->meas_type[module*100 + j] = CH_TYPE_INPUT_BINARY;
          break;
        case MOD_TYPE_INPUT_PULSE:
          for( j = 0; j < dq->modules[module].channel_number[0]; j++)
            dq->meas_type[module*100 + j] = CH_TYPE_INPUT_INTEGER;
          break;
        case MOD_TYPE_OUTPUT_ANALOG:
          for( j = 0; j < dq->modules[module].channel_number[1]; j++)
            dq->meas_type[module*100 + j] = CH_TYPE_OUTPUT_ANALOG;
          break;
        case MOD_TYPE_OUTPUT_DIGITAL:
          for( j = 0; j < dq->modules[module].channel_number[1]; j++)
            dq->meas_type[module*100 + j] = CH_TYPE_OUTPUT_BINARY;
          break;
        case MOD_TYPE_INPUT_OUTPUT_DIGITAL:
          for( j = 0; j < dq->modules[module].channel_number[0]; j++)
            dq->meas_type[module*100 + j] = CH_TYPE_INPUT_BINARY;
          for( j = 0; j < dq->modules[module].channel_number[1]; j++)
            dq->meas_type[module*100 + dq->modules[module].channel_number[0]+j] =
              CH_TYPE_OUTPUT_BINARY;
          break;
        case MOD_TYPE_PID:
        case MOD_TYPE_UNKNOWN:
          // SHOULD NOT MAKE IT HERE
          break;
        }
    }
  
  return 0;
}

static int load_status( struct devqueue *dq)
{
  char *p;

  gm10_simple_writer( &(dq->comm), "ORec?\r\n" );
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;
  p = dq->comm.inbuffer + 4;
  sscanf( p, "ORec,%d", &(dq->recording_mode) );

  gm10_simple_writer( &(dq->comm), "OMath?\r\n" );
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;
  p = dq->comm.inbuffer + 4;
  sscanf( p, "OMath,%d", &(dq->compute_mode) );

  if( dq->recording_mode && dq->compute_mode )
    dq->settings_mode = 1;
  else
    {
      if( dq->settings_mode == 1)
        qmesg( dq, NULL, CMD_READ_ALL_INFOS, 0, datum_empty());
      dq->settings_mode = 0;
    }

  scanIoRequest(dq->status_ioscanpvt);

  /* printf("%d,%d,%d\n", dq->recording_mode, dq->compute_mode,  */
  /*        dq->settings_mode); */

  return 0;
}




static int load_infos( struct devqueue *dq  )
{
  int ch_status;
  char unit[7];

  int address;

  struct channel_info *ci;
  struct channel_data *cd;

  char *ptr;
  int i;


  // grab the settings on the available channels for ADC, DAC, not RELAY
  // as well as the math channels

  //  gm10_simple_writer( &(dq->comm), "FE1,001,A300\r\n");
  gm10_simple_writer( &(dq->comm), "FChInfo\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    {
      return 1;
    }

  /* printf("%s\n", dq->comm.inbuffer); */
  
  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      switch( *ptr)
        {
        case 'N':
          ch_status = CH_STATUS_NORMAL;
          break;
        case 'D':
          ch_status = CH_STATUS_DIFF;
          break;
        case 'S':
          ch_status = CH_STATUS_SKIP;
          break;
        default:
          ch_status = CH_STATUS_UNKNOWN; // put it in some state;
        }
      ptr += 2;
      ci = NULL;
      cd = NULL;
      switch(*(ptr++))
        {
        case '0':
          address = atoi( ptr) - 1; // space stops conversion
          if( !dq->modules[address/100].use_flag)
            {
              ptr = strchr( ptr, '\n') + 1;
              continue;
            }
          ptr += 4;
          ci = &(dq->meas_info[address]);
          cd = &(dq->meas_data[address]);

          break;
        case 'A':
          address = atoi( ptr) - 1; // space stops conversion
          ptr += 4;
          ci = &(dq->calc_info[address]);
          cd = &(dq->calc_data[address]);

          break;
        case 'C':
          address = atoi( ptr) - 1; // space stops conversion
          ptr += 4;
          ci = &(dq->comm_info[address]);
          cd = &(dq->comm_data[address]);

          break;
        default:
          ptr = strchr( ptr, '\n') + 1;
          continue;
        }
      ci->ch_status = ch_status;
      if( ch_status == CH_STATUS_SKIP)
        {
          strcpy( ci->unit, "----");
          ci->scale = 0;
          ptr = strchr( ptr, '\n') + 1;

          // since the input won't do this
          cd->data_status = VL_SKIP;
          continue;
        }

      
      //      for( i = 0; *ptr != ','; i++, ptr++)
      for( i = 0; i < 10; i++, ptr++)
        {
          if( *ptr == ' ')
            break;
          unit[i] = *ptr;
        }
      unit[i] = '\0';
      ptr = strchr( ptr, ',');
      strcpy( ci->unit, unit);
      ptr++;
      ci->scale = atoi( ptr);
      
      ptr = strchr( ptr, '\n') + 1;

      /* i = ci->ch_status; */
      /* printf("Channel: %c %03d %s %d\n", 83 - 5*(i + i*i)/2, */
      /*        address + 1, ci->unit, ci->scale); */
    }


  // grab the DAC mode information

  gm10_simple_writer( &(dq->comm), "SRangeAO?\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      ptr += 9;
      if( *(ptr++) != '0')
        return 1;

      address = atoi( ptr) - 1; // space stops conversion
      if( !dq->modules[address/100].use_flag)
        {
          ptr = strchr( ptr, '\n') + 1;
          continue;
        }
      ptr += 4;

      ci = &(dq->meas_info[address]);
      switch( *ptr)
        {
        case 'S':
          ci->ch_mode = CH_MODE_DAC_SKIP;
          break;
        case 'M':
          ci->ch_mode = CH_MODE_DAC_MANUAL;
          break;
        case 'T':
          ci->ch_mode = CH_MODE_DAC_RETRANS;
          break;
        default:
          ci->ch_mode = CH_MODE_DAC_UNKNOWN;
        }

      ptr = strchr( ptr, '\n') + 1;


      /* printf("AO: %04d %s\n",   100*module + address + 1,  */
      /*        !ci->ch_mode ? "Skip" : ci->ch_mode == 1 ? "ReTrans" : "Manual") ; */

    }


  // grab the relay mode information
  gm10_simple_writer( &(dq->comm), "SRangeDO?\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      ptr += 9;
      if( *(ptr++) != '0')
        return 1;

      address = atoi( ptr) - 1; // space stops conversion
      if( !dq->modules[address/100].use_flag)
        {
          ptr = strchr( ptr, '\n') + 1;
          continue;
        }
      ptr += 4;

      ci = &(dq->meas_info[address]);
      // JUST IN CASE IT'S NEEDED
      /* if( ci->ch_status == CH_STATUS_SKIP) */
      /*   { */
      /*     ci->ch_mode = CH_MODE_RELAY_SKIP; */
      /*   } */
      /* else */
      switch( *ptr)
        {
        case 'A':
          ci->ch_mode = CH_MODE_RELAY_ALARM;
          break;
        case 'M':
          ci->ch_mode = CH_MODE_RELAY_MANUAL;
          break;
        case 'F':
          ci->ch_mode = CH_MODE_RELAY_FAIL;
          break;
        default:
          ci->ch_mode = CH_MODE_RELAY_UNKNOWN;
        }

      ptr = strchr( ptr, '\n') + 1;


      /* printf("DO: %04d %s\n",   100*module + address + 1,  */
      /*        !ci->ch_mode ? "Alarm" : ci->ch_mode == 1 ? "Manual" : "Fail") ; */
    }


  // grab the calculation expressions

  gm10_simple_writer( &(dq->comm), "SRangeMath?\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;
  
  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      ptr += 11;
      address = atoi( ptr) - 1;

      ptr += 5;
      if(*ptr == 'f')
        {
          dq->calc_expr[address].on_flag = 0;
          dq->calc_expr[address].expr[0] = '\0';
        }
      else
        {
          char *p;

          p = strchr(ptr, '\'');
          ptr = p+1;
          p = strchr(ptr, '\'');
          //          *p = '\0';

          dq->calc_expr[address].on_flag = 1;
          strncpy(dq->calc_expr[address].expr, ptr, (int) (p - ptr));
        }

      ptr = strchr( ptr, '\n') + 1;

      /* printf("A%03d %d %s\n",   address + 1,  */
      /*        dq->calc_expr[address].on_flag, */
      /*        dq->calc_expr[address].expr); */
    }



  scanIoRequest(dq->info_ioscanpvt );
 
  return 0;
}



// type 0 means get all, 1 means data 001-060, 2 means calc A001-A300
static int load_data_values( struct devqueue *dq, int type, int channel )
{
  //  int readlen;

  int length;
  int number_values;

  int i, cnt;

  unsigned char *q;

  unsigned char data_type;
  unsigned char channel_type;
  unsigned char status;
  unsigned short address;
  unsigned char alarms[4];
  int value;

  int alarm_flag;
  int io_flag;

  struct channel_data *cd;

  // request values for entire range, only those that exist will return


  switch( type)
    {
    case CMD_READ_ALL_DATA:
      gm10_simple_writer( &(dq->comm), "FData,1\r\n");
      break;
    case CMD_READ_SIGNAL:
      sprintf( dq->comm.outbuffer, "FData,1,%04d,%04d\r\n", channel, channel);
      gm10_writer( &(dq->comm));
      break;
    case CMD_READ_MATH:
      sprintf( dq->comm.outbuffer, "FData,1,A%03d,A%03d\r\n", channel, channel);
      gm10_writer( &(dq->comm));
      break;
    case CMD_READ_COMM:
      sprintf( dq->comm.outbuffer, "FData,1,C%03d,C%03d\r\n", channel, channel);
      gm10_writer( &(dq->comm));
      break;
    }
  if( response_reader( dq) != RESPONSE_BINARY)
    return 1;

  q = (unsigned char *) (dq->comm.inbuffer + 18);

  length = ntohs( *((epicsUInt16 *) q) );
  number_values = (length - 16)/12;
  q += 2;

  for( i = 0; i < 8; i++)
    dq->input_poll_time[i] = *(q++);

  q += 8;

  alarm_flag = 0;
  
  for( cnt = 0; cnt < number_values; cnt++)
    {
      data_type = (*q & 0xF0) >> 4;
      channel_type = *q & 0x0F;
      q++;
      status = *q;
      q++;
      address = ntohs( *((epicsUInt16 *) q) ) & 0x03FF;
      q += 2;
      for( i = 0; i < 4; i++)
        alarms[i] = *(q++);
      value = ntohl( *((epicsUInt32 *) q) );
      q += 4;

      if( !dq->modules[address/100].use_flag || (data_type != 1) )
        continue;
    

      /* printf("data_type: %d  channel_type: %d   status: %d  ch = %04d  val = %d\n", */
      /*        data_type, channel_type, status, address, value); */

      cd = NULL;
      switch( channel_type)
        {
        case 1: 
          cd = &dq->meas_data[address - 1];
          break;
        case 2:
          cd = &dq->calc_data[address - 1];
          break;
        case 3:
          cd = &dq->comm_data[address - 1];
          break;
        }

      switch( status & 0x1F)
        {
        case 0: // no error
          cd->data_status = VL_NORMAL;
          break;
        case 1: // skip
          cd->data_status = VL_SKIP;
          break;
        case 2: // +over
          cd->data_status = VL_POS_OVERRANGE;
          break;
        case 3: // -over
          cd->data_status = VL_NEG_OVERRANGE;
          break;
        case 4: // +burnout
          cd->data_status = VL_POS_BURNOUT;
          break;
        case 5: // -burnout
          cd->data_status = VL_NEG_BURNOUT;
          break;
        case 6: // A/D error
          cd->data_status = VL_AD_ERROR;
          break;
        case 7: // invalid data
          cd->data_status = VL_INVALID_DATA;
          break;
        case 16: // math result is NaN
          cd->data_status = VL_MATH_NAN;
          break;
        case 17: // communications error
          cd->data_status = VL_COMM_ERROR;
          break;
        default:
          cd->data_status = VL_UNKNOWN;
        }
      if( cd->data_status != VL_NORMAL)
        cd->value = 0;
      else
        cd->value = value;

      cd->alarm[0] = alarms[0] & 0x3F;
      cd->alarm[1] = alarms[1] & 0x3F;
      cd->alarm[2] = alarms[2] & 0x3F;
      cd->alarm[3] = alarms[3] & 0x3F;

      // could use dedicated bit in alarm for this (number 6)
      cd->alarm_status = ((cd->alarm[0] != 0) +
                          ((cd->alarm[1] != 0) << 1) +
                          ((cd->alarm[2] != 0) << 2) +
                          ((cd->alarm[3] != 0) << 3) );

      if( cd->alarm_status)
        alarm_flag = 1;
    }

  io_flag = 0;
  if( alarm_flag != dq->alarm_flag)
    io_flag = 1;
  dq->alarm_flag = alarm_flag;

  // if alarm_flag got set and it wasn't before, do i/o request
  if( (type == CMD_READ_ALL_DATA) || io_flag )
    scanIoRequest(dq->channel_ioscanpvt );

  return 0;
}




static int load_misc_values( struct devqueue *dq, int type, int channel )
{
  char *p;

  int address;


  if( (type == CMD_READ_ALL_MISC) || (type == CMD_READ_CONST) )
    {
      if( type == CMD_READ_ALL_MISC)
        gm10_simple_writer( &(dq->comm), "SKConst?\r\n" );
      else
        {
          sprintf( dq->comm.outbuffer, "SKConst,%d?\r\n", channel);
          gm10_writer( &(dq->comm));
        }
      if( response_reader( dq) != RESPONSE_ASCII)
        return 1;

      p = dq->comm.inbuffer;
      p += 4;
      while( *p != 'E')
        {
          p += 8;
          address = atoi(p) - 1;
          p = strchr( p, ',') + 1;
          dq->constant[address] = atof(p);
          
          p = strchr( p, '\n') + 1;

          /* printf("C%03d = %f\n", address + 1, dq->constant[address] ); */
        }
    }


  if( (type == CMD_READ_ALL_MISC) || (type == CMD_READ_VARCONST) )
    {
      if( type == CMD_READ_ALL_MISC)
        gm10_simple_writer( &(dq->comm), "SWConst?\r\n" );
      else
        {
          sprintf( dq->comm.outbuffer, "SWConst,%d?\r\n", channel);
          gm10_writer( &(dq->comm));
        }
      if( response_reader( dq) != RESPONSE_ASCII)
        return 1;

      p = dq->comm.inbuffer;
      p += 4;
      while( *p != 'E')
        {
          p += 8;
          address = atoi(p) - 1;
          p = strchr( p, ',') + 1;
          dq->varconstant[address] = atof(p);
          
          p = strchr( p, '\n') + 1;

          /* printf("W%03d = %f\n", address + 1, dq->varconstant[address] ); */
        }
    }

  if( type == CMD_READ_ALL_MISC)
    scanIoRequest(dq->misc_ioscanpvt );

  return 0;
}





static int set_output_value( struct devqueue *dq, int type, int channel,
                             double value)
{
  switch( type)
    {
    case CMD_SET_SIGNAL_OUTPUT:
      sprintf( dq->comm.outbuffer, "OCmdAO,%04d,%d\r\n", channel, 
               (int) (value * 1000.0) );
      break;
    case CMD_SET_COMM:
      sprintf( dq->comm.outbuffer, "OCommCh,C%03d,%G\r\n", channel, value );
      break;
    case CMD_SET_CONST:
      sprintf( dq->comm.outbuffer, "SKConst,%d,%G\r\n", channel, value );
      break;
    case CMD_SET_VARCONST:
      sprintf( dq->comm.outbuffer, "SWConst,%d,%G\r\n", channel, value );

      /* printf("%s\n",  dq->comm.outbuffer); */

      break;
    }

  gm10_writer( &(dq->comm));
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  return 0;
}


static int set_binary_value( struct devqueue *dq, int channel, int value)
{
  if( value < 0)
    value = 0;
  if( value > 1)
    value = 1;

  sprintf( dq->comm.outbuffer, "OCmdRelay,%04d-%s\r\n", channel, 
           value ? "On" : "Off" );
  gm10_writer( &(dq->comm));
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  return 0;
}


static int set_mode( struct devqueue *dq, int type, int value)
{
  if( type == CMD_SET_OPMODE)
    {
      if( (value != 0) && (value != 1) )
        return 1;

      sprintf( dq->comm.outbuffer, "ORec,%c\r\n", value ? '1' : '0');
      gm10_writer( &(dq->comm));
      if( response_reader( dq) != RESPONSE_OK)
        return 1;
    }
  else if( type == CMD_SET_COMPUTE)
    {
      if( (value < 0) || (value > 3) )
        return 1;
      sprintf( dq->comm.outbuffer, "OMath,%c\r\n", '0' + ((char) value));
      gm10_writer( &(dq->comm));
      if( response_reader( dq) != RESPONSE_OK)
        return 1;
    }
  /* printf("out = %d\n", value); fflush(stdout); */

  return 0;
}


static int clear_error(struct devqueue *dq)
{
  gm10_simple_writer( &(dq->comm), "OErrorClear,0\r\n");
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  dq->error_flag = 0;
  dq->error.code = -1;
  dq->error.strings[0][0] = '\0';
  dq->error.strings[1][0] = '\0';
  dq->error.strings[2][0] = '\0';

  scanIoRequest(dq->error_ioscanpvt );

  return 0;
}

static int acknowledge_alarms(struct devqueue *dq)
{
  gm10_simple_writer( &(dq->comm), "OAlarmAck,0\r\n");
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  // just do this in case scan is slow
  load_data_values(dq, CMD_READ_ALL_DATA, 0);

  return 0;
}


static int command_process( struct devqueue *dq, int cmd_id, int channel,
                     union datum dt)
{
  switch(cmd_id)
    {
    case CMD_LOAD_MODULES:
      load_modules(dq);
      break;
    case CMD_READ_ALL_DATA: // all channels
    case CMD_READ_SIGNAL:
    case CMD_READ_MATH:
    case CMD_READ_COMM:
      load_data_values( dq, cmd_id, channel);
      break;
    case CMD_READ_ALL_MISC: // all channels
    case CMD_READ_CONST:
    case CMD_READ_VARCONST:
      load_misc_values( dq, cmd_id, channel);
      break;
    case CMD_READ_ALL_INFOS:
      load_infos(dq);
      break;
    case CMD_READ_STATUS:
      load_status(dq);
      break;
    case CMD_SET_SIGNAL_OUTPUT:
    case CMD_SET_COMM:
    case CMD_SET_CONST:
    case CMD_SET_VARCONST:
      set_output_value( dq, cmd_id, channel, dt.flt_d);
      break;
    case CMD_SET_BINARY_OUTPUT:
      set_binary_value( dq, channel, dt.int_d);
      break;
    case CMD_SET_OPMODE:
    case CMD_SET_COMPUTE:
      set_mode(dq, cmd_id, dt.int_d);
      break;
    case CMD_CLEAR_ERROR:
      clear_error( dq);
      break;
    case CMD_ACKNOWLEDGE_ALARMS:
      acknowledge_alarms( dq);
      break;
    }
  return 0;
}

static void *queue_func(void *arg)
{
  struct devqueue *dq;
  struct req_pkt *pkt;
  struct rset *prset;

  dq = arg;
  while(1)
    {
      pthread_mutex_lock( &(dq->list_lock));
      if( dq->list == NULL)
        pthread_cond_wait(&(dq->list_cond), &(dq->list_lock));

      pkt = dq->list;
      dq->list = pkt->next;
      pthread_mutex_unlock( &(dq->list_lock));

      //      if( !error)
      command_process( dq, pkt->cmd_id, pkt->channel, pkt->value);

      if( pkt->precord != NULL)
        {
          prset = (struct rset *) (pkt->precord->rset);
          dbScanLock(pkt->precord);
          (*prset->process)(pkt->precord);
          dbScanUnlock(pkt->precord);
        }

      free( pkt);
      
      //      printf("%s", pkt->recv);
    }

  return NULL;
}

static int qmesg( struct devqueue *dq, dbCommon *precord, int cmd, int channel,
           union datum dt)
{
  struct req_pkt *pkt, *plp;


  pkt = malloc( sizeof( struct req_pkt) );

  pkt->cmd_id = cmd;
  pkt->channel = channel;
  pkt->precord = precord;
  pkt->value = dt;
  pkt->next = NULL;

  pthread_mutex_lock( &(dq->list_lock));
  if( dq->list == NULL)
    dq->list = pkt;
  else
    {
      plp = dq->list;
      while( plp->next != NULL)
        plp = plp->next;
      plp->next = pkt;
    }
  pthread_cond_signal( &(dq->list_cond));
  pthread_mutex_unlock( &(dq->list_lock));

  return 0;
}


static int init_gm10( char *device, char *address)
{
  struct queue_link *qlp;
  struct devqueue *dq;

  pthread_t thread;
  pthread_attr_t attr;

  char *p;
  

  if((device == NULL) || (*device == '\0'))
    return 1;

  // this validates the name;
  // has to start with letter, can have letters, numbers, or underscores
  p = device;
  if( !isalpha(*(p++)) )
    return 1;
  while( *p != '\0')
    {
      if( !isalnum(*p) && (*p != '_'))
        return 1;
      p++;
    }

  qlp = gm10_queue_list;
  while( qlp != NULL)
    {
      if( !strcmp(qlp->dq->name, device))
        {
          // device already exists
          return 1;
        }
      qlp = qlp->next;
    }

  qlp = calloc( 1, sizeof( struct queue_link) );
  if( qlp == NULL)
    return 1;
  qlp->next = gm10_queue_list;
  gm10_queue_list = qlp;

  dq = calloc( 1, sizeof(struct devqueue) );
  if( dq == NULL)
    return 1;
  qlp->dq = dq;
  pthread_mutex_init( &(dq->list_lock), NULL);
  pthread_cond_init( &(dq->list_cond), NULL);
  dq->list = NULL;

  dq->name = strdup( device);
  scanIoInit( &(dq->channel_ioscanpvt));
  scanIoInit( &(dq->misc_ioscanpvt));
  scanIoInit( &(dq->info_ioscanpvt));
  scanIoInit( &(dq->status_ioscanpvt));
  scanIoInit( &(dq->error_ioscanpvt));
  dq->comm.sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero( &(dq->comm.r_addr), sizeof(dq->comm.r_addr) );
  dq->comm.r_addr.sin_family = AF_INET;
  dq->comm.r_addr.sin_port = htons(34434);
  if( !inet_aton( address, &(dq->comm.r_addr.sin_addr)) )
    {
      free(dq);
      return 1;
    }

  if( gm10_socket_connect( &(dq->comm)) )
    return 1;

  // initial E0 after socket connect
  if( response_reader( dq) != RESPONSE_OK)
    return 1;


  /* // the order below has meaning, load_modules must go first */
  if( load_modules(dq) || load_status(dq) || load_infos(dq) || 
      load_data_values(dq, CMD_READ_ALL_DATA, 0) || 
      load_misc_values(dq, CMD_READ_ALL_MISC, 0) )
    return 1;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread, &attr, queue_func, (void *) dq);
  pthread_attr_destroy( &attr);

  return 0;
}




/////////////////////////////////
// device functions

int gm10_test_module( struct devqueue *dq, int module)
{
  if( !dq->modules[module].use_flag)
    return 1;
  
  return 0;
}

int gm10_test_signal( struct devqueue *dq, int channel)
{
  int type;

  type = dq->meas_type[channel - 1];
  if( (type == CH_TYPE_NONE) || (type == CH_TYPE_UNKNOWN) )
    return 1;
  
  return 0;
}


int gm10_test_analog_signal( struct devqueue *dq, int channel)
{
  int type;

  type = dq->meas_type[channel - 1];
  if( (type != CH_TYPE_OUTPUT_ANALOG) && (type != CH_TYPE_INPUT_ANALOG) )
    return 1;
  
  return 0;
}

int gm10_test_binary_signal( struct devqueue *dq, int channel)
{
  int type;

  type = dq->meas_type[channel - 1];
  if( (type != CH_TYPE_OUTPUT_BINARY) && (type != CH_TYPE_INPUT_BINARY) )
    return 1;
  
  return 0;
}

int gm10_test_integer_signal( struct devqueue *dq, int channel)
{
  if( dq->meas_type[channel - 1] != CH_TYPE_INPUT_INTEGER)
    return 1;
  
  return 0;
}

int gm10_test_output_analog_signal( struct devqueue *dq, int channel)
{
  if( dq->meas_type[channel - 1] != CH_TYPE_OUTPUT_ANALOG)
    return 1;
  
  return 0;
}

int gm10_test_output_binary_signal( struct devqueue *dq, int channel)
{
  if( dq->meas_type[channel - 1] != CH_TYPE_OUTPUT_BINARY)
    return 1;
  
  return 0;
}

struct devqueue *gm10_connect( char *device)
{
  struct queue_link *qlp;

  qlp = gm10_queue_list;
  while( qlp != NULL)
    {
      if( !strcmp(qlp->dq->name, device))
        return qlp->dq;
      qlp = qlp->next;
    }

  return NULL;
}

IOSCANPVT gm10_address_io_handler( struct devqueue *dq, int type)
{
  switch(type)
    {
    case ADDR_SIGNAL:
    case ADDR_MATH:
    case ADDR_COMM:
      return dq->channel_ioscanpvt;
      break;
    case ADDR_CONST:
    case ADDR_VARCONST:
      return dq->misc_ioscanpvt;
      break;
    }

  return NULL;
}
IOSCANPVT gm10_info_io_handler( struct devqueue *dq)
{
  return dq->info_ioscanpvt;
}
IOSCANPVT gm10_status_io_handler( struct devqueue *dq)
{
  return dq->status_ioscanpvt;
}
IOSCANPVT gm10_error_io_handler( struct devqueue *dq)
{
  return dq->error_ioscanpvt;
}
// just for alarm flag
IOSCANPVT gm10_channel_io_handler( struct devqueue *dq)
{
  return dq->channel_ioscanpvt;
}


// val or str is used depending on type, other is NULL
int gm10_module_info( struct devqueue *dq, int type, int module,
                       int *val, char *str)
{
  if( type == MODULE_PRESENCE)
    {
      *val = dq->modules[module].use_flag;
      return 0;
    }
  if( type == MODULE_STRING)
    {
      if( !dq->modules[module].use_flag)
        {
          if( dq->modules[module].module_string[0] != '\0')
            sprintf( str, "(unused) %s", dq->modules[module].module_string);
          else
            sprintf( str, "(empty)");
        }
      else
        strcpy( str, dq->modules[module].module_string);
      return 0;
    }
  if( !dq->modules[module].use_flag)
    return 0;

  return 0;
}



int gm10_system_info( struct devqueue *dq, int which, char *info)
{
  switch( which)
    {
    case 0:
      inet_ntop(AF_INET, &(dq->comm.r_addr.sin_addr), info, 39);
      break;
    default:
      return 1;
    }
  return 0;
}


int gm10_analog_set(struct devqueue *dq, dbCommon *precord, int type,
                      int channel, double value)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      if( dq->meas_type[channel-1] == CH_TYPE_OUTPUT_ANALOG)
        qmesg( dq, precord, CMD_SET_SIGNAL_OUTPUT, channel,
               datum_float(value));
      break;
    case ADDR_COMM:
      qmesg( dq, precord, CMD_SET_COMM, channel, datum_float(value));
      break;
    case ADDR_CONST:
      qmesg( dq, precord, CMD_SET_CONST, channel, datum_float(value));
      break;
    case ADDR_VARCONST:
      qmesg( dq, precord, CMD_SET_VARCONST, channel, datum_float(value));
      break;
    }

  return 0;
}

int gm10_binary_set(struct devqueue *dq, dbCommon *precord, int channel,
                     int value)
{
  if( dq->meas_type[channel-1] == CH_TYPE_OUTPUT_BINARY)
    qmesg( dq, precord, CMD_SET_BINARY_OUTPUT, channel, datum_int(value));

  return 0;
}

int gm10_trigger( struct devqueue *dq, dbCommon *precord, int type)
{
  /* printf("TRIG "); fflush(stdout); */

  switch(type)
    {
    case TRIG_INFO:
      qmesg( dq, precord, CMD_READ_ALL_INFOS, 0, datum_empty());
      break;
    case TRIG_CHANNELS:
      qmesg( dq, precord, CMD_READ_ALL_DATA, 0, datum_empty());
      break;
    case TRIG_MISC:
      qmesg( dq, precord, CMD_READ_ALL_MISC, 0, datum_empty());
      break;
    case TRIG_STATUS:
      qmesg( dq, precord, CMD_READ_STATUS, 0, datum_empty());
      break;
    }
  return 0;
}


int gm10_mode_set( struct devqueue *dq, dbCommon *precord, int type,
                    int value)
{

  switch(type)
    {
    case MODE_RECORDING:
      qmesg( dq, precord, CMD_SET_OPMODE, 0, datum_int(value) );
      break;
    case MODE_COMPUTE_CMD:
      qmesg( dq, precord, CMD_SET_COMPUTE, 0, datum_int(value) );
      break;
    }

  return 0;
}


int gm10_clear_error( struct devqueue *dq, dbCommon *precord)
{
  qmesg( dq, precord, CMD_CLEAR_ERROR, 0, datum_empty() );

  return 0;
}


int gm10_acknowledge_alarms( struct devqueue *dq, dbCommon *precord)
{
  qmesg( dq, precord, CMD_ACKNOWLEDGE_ALARMS, 0, datum_empty() );

  return 0;
}



int gm10_channel_start(struct devqueue *dq, dbCommon *precord, int type,
                            int channel)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      switch( dq->meas_type[channel-1])
        {
        case CH_TYPE_INPUT_ANALOG:
        case CH_TYPE_INPUT_INTEGER:
        case CH_TYPE_INPUT_BINARY:
        case CH_TYPE_OUTPUT_BINARY:
        case CH_TYPE_OUTPUT_ANALOG:
          qmesg( dq, precord, CMD_READ_SIGNAL, channel, datum_empty());
          break;
        case CH_TYPE_UNKNOWN:
        case CH_TYPE_NONE:
          return 0;
        }
      break;
    case ADDR_MATH:
      qmesg( dq, precord, CMD_READ_MATH, channel, datum_empty());
      break;
    case ADDR_COMM:
      qmesg( dq, precord, CMD_READ_COMM, channel, datum_empty());
      break;
    case ADDR_CONST:
      qmesg( dq, precord, CMD_READ_CONST, channel, datum_empty());
      break;
    case ADDR_VARCONST:
      qmesg( dq, precord, CMD_READ_VARCONST, channel, datum_empty());
      break;
    }

  return 0;
}

int gm10_analog_get( struct devqueue *dq, int type, int channel,
                       double *value)
{
  struct channel_info *ci = NULL;
  struct channel_data *cd = NULL;

  int ctype;


  switch(type)
    {
    case ADDR_SIGNAL:
      ctype =  dq->meas_type[channel-1];
      if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
        return 0;
      ci = &(dq->meas_info[channel - 1]);
      cd = &(dq->meas_data[channel - 1]);
      break;
    case ADDR_MATH:
      ci = &(dq->calc_info[channel - 1]);
      cd = &(dq->calc_data[channel - 1]);
      break;
    case ADDR_COMM:
      ci = &(dq->comm_info[channel - 1]);
      cd = &(dq->comm_data[channel - 1]);
      break;
    case ADDR_CONST:
      *value = (double) dq->constant[channel-1];
      return 0;  // leaves here
    case ADDR_VARCONST:
      *value = (double) dq->varconstant[channel-1];
      return 0;  // leaves here
    }


  *value = scaled_value( cd->value, ci->scale);

  return 0;
}

int gm10_integer_get( struct devqueue *dq, int channel, int *value)
{
  int ctype;

  ctype =  dq->meas_type[channel-1];

  if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
    return 0;

  // hope it doesn't need to be scaled!!!
  *value = (int) dq->meas_data[channel - 1].value;

  return 0;
}

int gm10_binary_get( struct devqueue *dq, int channel, int *value)
{
  int ctype;

  ctype = dq->meas_type[channel - 1];
  if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
    return 0;

  *value = (int) dq->meas_data[channel - 1].value;;

  return 0;
}


int gm10_channel_get_egu( struct devqueue *dq, int type, int channel,
                           char *egu)
{
  int ctype;

  ctype = dq->meas_type[channel - 1];
  switch(type)
    {
    case ADDR_SIGNAL:
      if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
        return 0;
      strcpy( egu, dq->meas_info[channel-1].unit);
      break;
    case ADDR_MATH:
      strcpy( egu, dq->calc_info[channel-1].unit);
      break;
    case ADDR_COMM:
      strcpy( egu, dq->comm_info[channel-1].unit);
      break;
    }

  return 0;
}

int gm10_channel_get_expr( struct devqueue *dq, int channel, char *expr)
{
  strncpy( expr, dq->calc_expr[channel-1].expr, 40 );
  expr[39] = '\0';

  return 0;
}

int gm10_get_channel_status( struct devqueue *dq, int type, int channel,
                              int *value)
{
  int ctype;

  switch(type)
    {
    case ADDR_SIGNAL:
      ctype = dq->meas_type[channel - 1];
      if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
        return 0;
      *value = dq->meas_info[channel-1].ch_status;
      break;
    case ADDR_MATH:
      *value = dq->calc_info[channel-1].ch_status;
      break;
    case ADDR_COMM:
      *value = dq->comm_info[channel-1].ch_status;
      break;
    }

  return 0;
}

int gm10_get_channel_mode( struct devqueue *dq, int channel, int *value)
{
  int ctype;

  ctype = dq->meas_type[channel - 1];
  if( (ctype != CH_TYPE_OUTPUT_BINARY) && (ctype != CH_TYPE_OUTPUT_ANALOG) )
    return 0;
  *value = dq->meas_info[channel-1].ch_mode;

  return 0;
}


int gm10_get_data_status( struct devqueue *dq, int type, int channel,
                           int *value)
{
  int ctype;

  switch(type)
    {
    case ADDR_SIGNAL:
      ctype = dq->meas_type[channel-1];
      if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
        return 0;
      *value = dq->meas_data[channel-1].data_status;
      break;
    case ADDR_MATH:
      *value = dq->calc_data[channel-1].data_status;
      break;
    case ADDR_COMM:
      *value = dq->comm_data[channel-1].data_status;
      break;
    }

  return 0;
}


int gm10_get_alarm_flag( struct devqueue *dq, int *value)
{
  *value = dq->alarm_flag;

  return 0;
}


int gm10_get_alarm( struct devqueue *dq, int type, int channel,
                     int sub_channel, int *value)
// sub_channel = 0 get alarm_status for channel
{
  int ctype;

  switch(type)
    {
    case ADDR_SIGNAL:
      ctype = dq->meas_type[channel-1];
      if( (ctype == CH_TYPE_NONE) || (ctype == CH_TYPE_UNKNOWN) )
        return 0;
      if( sub_channel)
        *value = dq->meas_data[channel-1].alarm[sub_channel-1];
      else
        *value = dq->meas_data[channel-1].alarm_status;
      break;
    case ADDR_MATH:
      if( sub_channel)
        *value = dq->calc_data[channel-1].alarm[sub_channel-1];
      else
        *value = dq->calc_data[channel-1].alarm_status;
      break;
    case ADDR_COMM:
      if( sub_channel)
        *value = dq->comm_data[channel-1].alarm[sub_channel-1];
      else
        *value = dq->comm_data[channel-1].alarm_status;
      break;
    }

  return 0;
}


int gm10_get_error( struct devqueue *dq, int channel, char *error)
{
  error[0] = '\0';
  if( dq->error_flag )
    {
      strcpy( error, dq->error.strings[channel-1]);
    }

  return 0;
}

int gm10_get_error_flag( struct devqueue *dq, int *flag)
{
  *flag = dq->error_flag;

  return 0;
}

int gm10_get_mode( struct devqueue *dq, int type, int *value)
{
  switch(type)
    {
    case MODE_SETTINGS:
      *value = dq->settings_mode;
      break;
    case MODE_RECORDING:
      *value = dq->recording_mode;
      break;
    case MODE_COMPUTE:
      *value = dq->compute_mode;
      break;
    }

  return 0;
}

/* int gm10_get_status_byte( struct devqueue *dq, int channel, int *value) */
/* { */
/*   *value = dq->status[channel-1]; */

/*   return 0; */
/* } */






/* Initialization method definitions */

static const iocshArg arg0 = {"netDevice",iocshArgString};
static const iocshArg arg1 = {"address",iocshArgString};
static const iocshArg* args[]= {&arg0,&arg1};
static const iocshFuncDef gm10InitDef = {"gm10Init",2,args};
static void gm10InitFunc(const iocshArgBuf* args)
{
  init_gm10( args[0].sval, args[1].sval);
}

/* Registration method */
static void gm10Register(void)
{
  static int firstTime = 1;

  if( firstTime )
    {
      firstTime = 0;
      iocshRegister( &gm10InitDef,gm10InitFunc );
    }
}


epicsExportRegistrar( gm10Register );
