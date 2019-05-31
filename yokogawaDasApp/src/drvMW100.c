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

#include "drvMW100_comm.h"

////////////////////////////////////

//      001-060     A001-A300  C001-C300   K01-K60 
enum { ADDR_SIGNAL, ADDR_MATH, ADDR_COMM, ADDR_CONST };
enum { TRIG_INFO, TRIG_INPUT, TRIG_OUTPUT, TRIG_STATUS };
enum { MODE_SETTINGS, MODE_MEASUREMENT, MODE_COMPUTE, MODE_OPERATING,
       MODE_COMPUTE_CMD };
enum { MODULE_PRESENCE, MODULE_STRING, MODULE_MODEL, MODULE_CODE, MODULE_SPEED,
       MODULE_NUMBER };


struct module
{
  // raw information
  char set_message[14];
  char status_message[14];
  char error_message[14];

  char module_string[14];

  // broken-out information
  int use_flag;
  // 110 = analog input: ADC
  // 112 = analog input: strain gauge
  // 114 = integer digital input: pulse counting
  // 115 = binary digital input: level measurement
  // 120 = analog output: DAC
  // 125 = binary digital output: relay
  int model; // 110, 112, 114, 115, 120, 125, 0 means empty
  //  int error; // 1=RomError:, 2=CalError:, 3=SlotError:
  char code[4]; 
  int speed; // 0=Low, 1=Medium, 2=High
  int number; // currently: 4, 6, 8, 10, 30
};

enum { CH_TYPE_NONE, CH_TYPE_INPUT_ANALOG, CH_TYPE_INPUT_BINARY, 
       CH_TYPE_INPUT_INTEGER, CH_TYPE_OUTPUT_ANALOG, CH_TYPE_OUTPUT_BINARY, 
       CH_TYPE_UNKNOWN };


// mode
enum { CH_STATUS_SKIP, CH_STATUS_NORMAL, CH_STATUS_DIFF, CH_STATUS_UNKNOWN };
enum { CH_MODE_RELAY_SKIP, CH_MODE_RELAY_ALARM, CH_MODE_RELAY_COM,
       CH_MODE_RELAY_MEDIA, CH_MODE_RELAY_FAIL, CH_MODE_RELAY_ERROR, 
       CH_MODE_RELAY_UNKNOWN };
enum { CH_MODE_DAC_SKIP, CH_MODE_DAC_TRANS, CH_MODE_DAC_COM, 
       CH_MODE_DAC_UNKNOWN };       

struct channel_info
{
  char ch_status;
  char ch_mode;
  char unit[7];
  char scale;

  //  struct input_options options;
};

// data_status values
enum { VL_NORMAL, VL_OVERRANGE, VL_UNDERRANGE, VL_SKIP_OFF, VL_ERROR, 
       VL_UNCERTAIN };

enum { AL_OFF, AL_HIGH, AL_LOW, AL_DIFF_HIGH, AL_DIFF_LOW, AL_ROC_HIGH, 
       AL_ROC_LOW, AL_DELAY_HIGH, AL_DELAY_LOW };

struct channel_data
{
  char alarm_status;
  char alarm[4];
  char data_status;
  int  value;
};

struct expr_info
{
  char on_flag;
  char expr[121];
};

struct short_expr_info
{
  char on_flag;
  char expr[11];
};

static double scaled_value( int value, char scale)
{
  // scale is only allowed to be 0-4
  double scaler[5] = { 1.0, 0.1, 0.01, 0.001, 0.0001 };

  return ((double) value) * scaler[ (int) scale];
}

static int unscaled_value( double value, char scale)
{
  double scaler[5] = { 1.0, 10.0, 100.0, 1000.0, 10000.0 };

  return (int) (((double) value) * scaler[ (int) scale]);
}


/////////////////////////////////////


LOCAL long mw100_report();
LOCAL long mw100_init();

struct 
{
  long number;
  DRVSUPFUN report;
  DRVSUPFUN init;
} drvMW100 = { 2, mw100_report, mw100_init };

epicsExportAddress(drvet,drvMW100);



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




struct error_value
{
  int id;
  const char *string[3];
};

static struct error_value errors[] =
  {
    // this first entry is not on device, added to make life easier
    {   0, { "Invalid response from DAU.", "", "" }},

    {   1, { "Invalid function parameter.", "", "" }},
    {   2, { "Value exceeds the setting range.", "", "" }},
    {   3, { "Incorrect real number format.", "", "" }},
    {   4, { "Real number value exceeds the setting", "range.", "" }},
    {   5, { "Incorrect character string.", "", "" }},
    {   6, { "Character string too long.", "", "" }},
    {   7, { "Incorrect display color format.", "", "" }},
    {   8, { "Incorrect date format.", "", "" }},
    {   9, { "Date value exceeds the setting range.", "", "" }},
    {  10, { "Incorrect time format.", "", "" }},
    {  11, { "Time value exceeds the setting range., "", "" "}},
    {  12, { "Incorrect time zone format.", "", "" }},
    {  13, { "Time zone value exceeds the setting", "range.", "" }},
    {  14, { "Incorrect IP address format.", "", "" }},
    {  20, { "Invalid channel number.", "", "" }},
    {  21, { "Invalid sequence of first and last", "channel.", ""}},
    {  22, { "Invalid alarm number.", "", "" }},
    {  23, { "Invalid relay number.", "", "" }},
    {  24, { "Invalid sequence of first and last", "relay.", ""}},
    {  25, { "Invalid MATH group number.", "", "" }},
    {  26, { "Invalid box number.", "", "" }},
    {  27, { "Invalid timer number.", "", "" }},
    {  28, { "Invalid match time number.", "", "" }},
    {  29, { "Invalid measurement group number.", "", "" }},
    {  30, { "Invalid module number.", "", "" }},
    {  31, { "Invalid start and end time of DST.", "", "" }},
    {  32, { "Invalid display group number.", "", "" }},
    {  33, { "Invalid tripline number.", "", "" }},
    {  34, { "Invalid message number.", "", "" }},
    {  35, { "Invalid user number.", "", "" }},
    {  36, { "Invalid server type.", "", "" }},
    {  37, { "Invalid e-mail contents.", "", "" }},
    {  38, { "Invalid server number.", "", "" }},
    {  39, { "Invalid command number.", "", "" }},
    {  40, { "Invalid client type.", "", "" }},
    {  41, { "Invalid server type.", "", "" }},
    {  50, { "Invalid input type.", "", "" }},
    {  51, { "Module of an invalid input type found",
             "in the range of specified channels.", "" }},
    {  52, { "Invalid measuring range.", "", "" }},
    {  53, { "Module of an invalid measuring range",
             "found in the range of specified", "channels." }},
    {  54, { "Upper and lower limits of span cannot", "be equal.", "" }},
    {  55, { "Upper and lower limits of scale cannot", "be equal.", "" }},
    {  56, { "Invalid reference channel number.", "", "" }},
    {  60, { "Cannot set an alarm for a skipped", "channel.", "" }},
    {  61, { "Cannot set an alarm for a channel on", 
             "which MATH function is turned OFF.", "" }},
    {  62, { "Invalid alarm type.", "", "" }},
    {  63, { "Invalid alarm relay number.", "", "" }},
    {  65, { "Cannot set hysteresis for a channel on", 
             "which alarms are turned OFF.", "" }},
    {  70, { "Nonexistent channel specified in MATH", 
             "expression.", "" }},
    {  71, { "Nonexistent constant specified in MATH",
             "expression.", "" }},
    {  72, { "Invalid syntax found in MATH", "expression.", "" }},
    {  73, { "Too many operators for MATH", "expression.", "" }},
    {  74, { "Invalid order of operators.", "", "" }},
    {  75, { "Upper an lower limits of MATH span", "cannot be equal.", "" }},
    {  80, { "Incorrect MATH group format.", "", "" }},
    {  81, { "Incorrect channels for MATH group.", "", "" }},
    {  82, { "Too many channels for MATH group.", "", "" }},
    {  90, { "Incorrect break point format.", "", "" }},
    {  91, { "Time value of break point exceeds the", "setting range.", "" }},
    {  92, { "Output value of break point exceeds", "the setting range.", "" }},
    {  93, { "No break point found.", "", "" }},
    {  94, { "Invalid time value of first break", "point.", "" }},
    {  95, { "Invalid time sequence found in break", "points.", "" }},
    { 100, { "Invalid output type.", "", "" }},
    { 101, { "Module of an invalid output type found", 
             "in the range of specified channels.", ""}},
    { 102, { "Invalid output range.", "", "" }},
    { 103, { "Module of an invalid output range", 
             "found in the range of specified", "channels." }},
    { 104, { "Upper and lower limits of output span", "cannot be equal.", "" }},
    { 105, { "Invalid transmission reference", "channel.", ""}},
    { 110, { "Invalid channel number for contact", "input event.", "" }},
    { 111, { "Invalid channel number for alarm", "event.", "" }},
    { 112, { "Invalid relay number for relay event.", "", "" }},
    { 113, { "Invalid action type.", "", "" }},
    { 114, { "Invalid combination of edge and level", 
             "detection actions.", "" }},
    { 115, { "Invalid combination of level detection", "actions.", "" }},
    { 116, { "Invalid flag number", "", "" }},
    { 120, { "Invalid measurement group number.", "", "" }},
    { 121, { "Invalid measurement group number for", "MATH interval.", "" }},
    { 130, { "Size of data file for measurement group", 
             "1 exceeds the upper limit.", "" }},
    { 131, { "Size of data file for measurement group", 
             "2 exceeds the upper limit.", "" }},
    { 132, { "Size of data file for measurement group", 
             "3 exceeds the upper limit.", ""}},
    { 133, { "Size of MATH data file exceeds the", "upper limit.", "" }},
    { 134, { "Size of thinned data file exceeds the", "upper limit.", "" }},
    { 135, { "Cannot set smaller value for thinning", 
             "recording interval than measuring or", "MATH interval." }},
    { 136, { "Invalid combination of thinning", 
             "recording, measuring and MATH interval.", "" }},
    { 137, { "The combination of the thinning", 
             "recording interval and the thinning", 
             "recording data length is not correct." }},
    { 138, { "Cannot set recording operation for", 
             "measurement group with no measuring", "interval." }},
    { 139, { "Invalid recording interval.", "", "" }},
    { 140, { "Upper and lower limits of the display", 
             "zone cannot be equal.", "" }},
    { 141, { "Cannot set smaller value than lower", 
             "limit of display zone for upper limit.", "" }},
    { 142, { "Width of display zone must be 5% of", 
             "that of the entire display or more.", "" }},
    { 145, { "Incorrect display group format.", "", "" }},
    { 150, { "IP address must belong to class A, B,", "or C.", "" }},
    { 151, { "Net or host part of IP address is all", "0's or 1's.", ""}},
    { 152, { "Invalid subnet mask.", "", "" }},
    { 153, { "Invalid gateway address.", "", "" }},
    { 160, { "Incorrect alarm e-mail channel format.", "", "" }},
    { 165, { "Invalid channel number for Modbus", "command.", "" }},
    { 166, { "Invalid combination of start and end", 
             "channel for Modbus command.", "" }},
    { 167, { "Invalid sequence of start and end", 
             "channel for Modbus command.", "" }},
    { 168, { "Too many channels for command number.", "", "" }},
    { 170, { "Invalid channel number for report.", "", "" }},
    { 201, { "Cannot execute due to different", "operation mode.", "" }},
    { 202, { "Cannot execute while in setting mode.", "", "" }},
    { 203, { "Cannot execute while in measurement", "mode.", ""}},
    { 204, { "Cannot change or execute during memory", "sampling.", "" }},
    { 205, { "Cannot execute during MATH operation.", "", "" }},
    { 206, { "Cannot change or execute during MATH", "operation.", "" }},
    { 207, { "Cannot change or execute while", 
             "saving/loading settings.", "" }},
    { 209, { "Cannot execute while memory sample is", "stopped.", "" }},
    { 211, { "No relays for communication input", "found.", "" }},
    { 212, { "Initial balance failed.", "", "" }},
    { 213, { "No channels for initial balance found.", "", "" }},
    { 214, { "No channels for transmission output", "found.", "" }},
    { 215, { "No channels for arbitrary output", "found.", "" }},
    { 221, { "No measurement channels found.", "", "" }},
    { 222, { "Invalid measurement interval.", "", "" }},
    { 223, { "Too many measurement channels.", "", "" }},
    { 224, { "No MATH channels found.", "", "" }},
    { 225, { "Invalid MATH interval.", "", "" }},
    { 226, { "Cannot start/stop MATH operation.", "", "" }},
    { 227, { "Cannot start/stop recording.", "", "" }},
    { 301, { "CF card error detected.", "", "" }},
    { 302, { "Not enough free space on CF card.", "", "" }},
    { 303, { "CF card is write-protected.", "", "" }},
    { 311, { "CF card not inserted.", "", "" }},
    { 312, { "CF card format damaged.", "", "" }},
    { 313, { "CF card damaged or not formatted.", "", "" }},
    { 314, { "File is write-protected.", "", "" }},
    { 315, { "No such file or directory.", "", "" }},
    { 316, { "Number of files exceeds the upper", "limit.", "" }},
    { 317, { "Invalid file or directory name.", "", "" }},
    { 318, { "Unknown file type.", "", "" }},
    { 319, { "Same name of file or directory already", "exists.", "" }},
    { 320, { "Invalid file or directory operation.", "", "" }},
    { 321, { "File in use.", "", "" }},
    { 331, { "Setting file not found.", "", "" }},
    { 332, { "Setting file is broken.", "", "" }},
    { 341, { "FIFO buffer overflow.", "", "" }},
    { 342, { "Data to be save to file not found.", "", "" }},
    { 343, { "Power failed while opening file.", "", "" }},
    { 344, { "Some or all data prior to power outage", 
             "could not be recovered.", "" }},
    { 345, { "Could not restart recording after",
             "recovery from power failure.", "" }},
    { 346, { "Recording could not be started due to", "power outage.", "" }},
    { 401, { "Command string too long.", "", "" }},
    { 402, { "Too many commands enumerated.", "", "" }},
    { 403, { "Invalid type of commands enumerated.", "", "" }},
    { 404, { "Invalid command.", "", "" }},
    { 405, { "Not allowed to execute this command.", "", "" }},
    { 406, { "Cannot execute due to different", "operation mode.", "" }},
    { 407, { "Invalid number of parameters.", "", "" }},
    { 408, { "Parameter string too long.", "", "" }},
    { 411, { "Daylight saving time function not", "available.", ""}},
    { 412, { "Temperature unit selection not", "available.", ""}},
    { 413, { "MATH operation not available.", "", "" }},
    { 414, { "Serial communication interface option", "not available.", "" }},
    { 415, { "Report option not available.", "", "" }},
    { 501, { "Login first.", "", "" }},
    { 502, { "Login failed, try again.", "", "" }},
    { 503, { "Connection count exceeded the upper", "limit.", "" }},
    { 504, { "Connection has been lost.", "", "" }},
    { 505, { "Connection has timed out.", "", "" }},
    { 520, { "FTP function not available.", "", "" }},
    { 521, { "FTP control connection failed.", "", "" }},
    { 530, { "SMTP function not available.", "", "" }},
    { 531, { "SMTP connection failed.", "", "" }},
    { 532, { "POP3 connection failed.", "", "" }},
    { 550, { "SNTP function not available.", "", "" }},
    { 551, { "SNTP command/response failed.", "", "" }},
    { 999, { "System error.", "", "" }}
  };
static int errors_num = sizeof( errors)/sizeof(struct error_value);


struct devqueue
{
  char *name;
  struct mw100_comm comm; // holds network reading and writing stuff

  int error_flag; // nonzero if error
  struct error_value *error; // NULL if unknown error and flag is set

  IOSCANPVT input_ioscanpvt;  // input channels
  IOSCANPVT output_ioscanpvt; // output channels 
  IOSCANPVT info_ioscanpvt;   // info for channels: prec, egu, ...
  IOSCANPVT status_ioscanpvt; // device status
  IOSCANPVT error_ioscanpvt;  // error messages

  pthread_mutex_t list_lock;
  pthread_cond_t list_cond;
  struct req_pkt *list;

  //////////////////////////

  unsigned char status[8];
  int settings_mode; 
  int measurement_mode; 
  int compute_mode;

  int alarm_flag;

  struct module modules[6]; // only 6 possible

  unsigned char input_poll_time[6]; // changed with each poll
  unsigned char output_poll_time[6]; // changed with each poll
  // ch_info 0-59 are regular channels 001-060, 
  unsigned char ch_type[60];
  struct channel_info ch_info[60];    // changed only when not measuring
  struct channel_data ch_data[60];    // changed with each data call
  // calc_info 0-299 are math channels A001-A300
  struct channel_info calc_info[300]; // changed only when not measuring
  struct channel_data calc_data[300]; // changed with each data call
  float comm_input[300];
  float constant[60];
  struct expr_info calc_expr[60]; // A001-A060  120 max
  struct short_expr_info short_calc_expr[240]; // A061-A300  10 or 8 max
};

struct queue_link
{
  struct devqueue *dq;
  struct queue_link *next;
};

// just define head of linked list
static struct queue_link *mw100_queue_list = NULL; 




LOCAL long mw100_init()
{
  return 0;
}

LOCAL long mw100_report(int level)
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
  qlp = mw100_queue_list;
  while( qlp != NULL)
    {
      cnt++;
      dq = qlp->dq;

      inet_ntop(AF_INET, &(dq->comm.r_addr.sin_addr), buffer, buflen);
      
      printf("    Yokogawa MW100 #%d: %s @ %s\n", cnt, dq->name, buffer);

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
       CMD_READ_ALL_INPUTS, CMD_READ_ALL_OUTPUTS, 
       CMD_READ_SIGNAL_INPUT, CMD_READ_SIGNAL_OUTPUT, 
       CMD_READ_MATH, CMD_READ_COMM, CMD_READ_CONST,
       CMD_SET_SIGNAL_OUTPUT, CMD_SET_BINARY_OUTPUT,
       CMD_SET_COMM, CMD_SET_CONST};


static int qmesg( struct devqueue *dq, dbCommon *precord, int cmd, int channel,
           union datum dt);



// need to do error handling locally, different between models
static int response_reader( struct devqueue *dq )
{
  int response;
  int error_code;

  dq->error_flag = 0;
  dq->error = NULL;


  response = mw100_response_reader( &(dq->comm) );
  if( response == RESPONSE_ERROR)
    {
      dq->error_flag = 1;

      if( sscanf( dq->comm.inbuffer, "E1 %d %*s\r\n", &error_code) == 1)
        {
          int i;

          for( i = 0; i < errors_num; i++)
            {
              if( errors[i].id == error_code)
                dq->error = &(errors[i]);
            }

          scanIoRequest(dq->error_ioscanpvt);
          return RESPONSE_ERROR;
        }
      else
        return RESPONSE_INVALID;
    }

  // multiple errors not really handles right now
  if( response == RESPONSE_CHAIN_ERRORS)
      dq->error_flag = 1;
    
  return response;
}


static int load_modules( struct devqueue *dq)
{
  char *ptr;

  int which;
  int type;

  char *c;
  int i,j;

  mw100_simple_writer( &(dq->comm), "CF0\r\n" );
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      which = *ptr - '0';
      ptr += 4;

      c = dq->modules[which].set_message;
      for( i = 0; i < 13; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 3;

      c = dq->modules[which].status_message;
      for( i = 0; i < 13; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 1;

      c = dq->modules[which].error_message;
      while( *ptr != '\r')
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 2;
    }

  for( i = 0; i < 6; i++)
    {
      dq->modules[i].use_flag = 0;

      // the strings have to match
      if( strcmp( dq->modules[i].set_message, 
                  dq->modules[i].status_message ) )
        continue;
      // need no errors
      if( dq->modules[i].error_message[0] != '\0')
        continue;
      if( !strcmp( dq->modules[i].set_message, "-------------") )
        continue;

      dq->modules[i].use_flag = 1;
      strcpy( dq->modules[i].module_string, dq->modules[i].set_message );
      
      // the dash will halt the conversion
      ptr = dq->modules[i].module_string + 2;
      dq->modules[i].model = atoi(ptr);
      ptr += 4;

      c = dq->modules[i].code;
      for( j = 0; j < 3; j++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr++;

      switch( *ptr)
        {
        case 'L':
          dq->modules[i].speed = 0;
          break;
        case 'M':
          dq->modules[i].speed = 1;
          break;
        case 'H':
          dq->modules[i].speed = 2;
          break;
        default:
          dq->modules[i].speed = -1;
        }
      ptr++;
      
      dq->modules[i].number = atoi(ptr);
    }

  for( j = 0; j < 60; j++)
    dq->ch_type[j] = CH_TYPE_NONE;
  for( i = 0; i < 6; i++)
    if( dq->modules[i].use_flag)
      {
        switch( dq->modules[i].model )
          {
          case 110:
          case 112:
            type = CH_TYPE_INPUT_ANALOG;
            break;
          case 114:
            type = CH_TYPE_INPUT_INTEGER;
            break;
          case 115:
            type = CH_TYPE_INPUT_BINARY;
            break;
          case 120:
            type = CH_TYPE_OUTPUT_ANALOG;
            break;
          case 125:
            type = CH_TYPE_OUTPUT_BINARY;
            break;
          default:
            type = CH_TYPE_UNKNOWN;
          }

        for( j = 0; j < dq->modules[i].number; j++)
          dq->ch_type[10*i + j] = type;
      }
  
  return 0;
}

static int load_status( struct devqueue *dq)
{
  int i;
  char *p;

  mw100_simple_writer( &(dq->comm), "IS0\r\n" );
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  p = dq->comm.inbuffer + 4;
  for( i = 0; i < 8; i++)
    {
      dq->status[i] = 100*(*p-'0') + 10*(*(p+1)-'0') + (*(p+2)-'0');
      p += 4;
    }

  /* only status[4] is used right now */

  if( dq->status[4] & 1)
    dq->settings_mode = 1;
  else
    {
      if( dq->settings_mode == 1)
        {
          qmesg( dq, NULL, CMD_READ_ALL_INFOS, 0, datum_empty());
        }
      dq->settings_mode = 0;
    }

  if( dq->status[4] & 2)
    dq->measurement_mode = 1;
  else
    dq->measurement_mode = 0;

  if( dq->status[4] & 4 )
    dq->compute_mode = 1;
  else
    dq->compute_mode = 0;

  scanIoRequest(dq->status_ioscanpvt);

  return 0;
}

static int load_infos( struct devqueue *dq  )
{
  int ch_status;
  char unit[7];
  char scale[4];
  char channel[5];
  
  int address;

  struct channel_info *ci;
  struct channel_data *cd;

  char *ptr;
  int i;

  // grab the settings on the available channels for ADC, DAC, not RELAY
  // as well as the math channels

  mw100_simple_writer( &(dq->comm), "FE1,001,A300\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  //  printf("%s\n", inbuffer);

  // printf(inbuffer);

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
      for( i = 0; i < 4; i++, ptr++)
        channel[i] = *ptr;
      channel[4] = '\0';
      if( channel[0] == 'A')
        {
          address = atoi( &channel[1]) - 1;
          ci = &(dq->calc_info[address]);
          cd = &(dq->calc_data[address]);
        }
      else
        {
          address = atoi( &channel[0]) - 1;
          ci = &(dq->ch_info[address]);
          cd = &(dq->ch_data[address]);
        }

      ci->ch_status = ch_status;

      if( ch_status == CH_STATUS_SKIP)
        {
          strcpy( ci->unit, "----");
          ci->scale = 0;
          ptr = strchr( ptr, '\n') + 1;

          // since the input won't do this
          cd->data_status = VL_SKIP_OFF;
          continue;
        }
      
      for( i = 0; i < 6; i++, ptr++)
        {
          if( *ptr == ' ')
            break;
          unit[i] = *ptr;
        }
      unit[i] = '\0';
      ptr = strchr( ptr, ',');
      strcpy( ci->unit, unit);
        
      ptr++;
      for( i = 0; i < 3; i++, ptr++)
        scale[i] = *ptr;
      scale[3] = '\0';
      ci->scale = atoi( scale);

      ptr = strchr( ptr, '\n') + 1;
    }






  mw100_simple_writer( &(dq->comm), "FO0,001,060\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ch_status = CH_STATUS_UNKNOWN; // put it in some state;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      switch( *ptr)
        {
        case 'N':
          ch_status = CH_STATUS_NORMAL;
          break;
        case 'S':
          ch_status = CH_STATUS_SKIP;
          break;
        }
      ptr += 2;
      address = atoi( ptr) - 1;
      ptr = strchr( ptr, '\n') + 1;

      dq->ch_info[address].ch_status = ch_status;
    }




  // grab the DAC mode information

  mw100_simple_writer( &(dq->comm), "AO?\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      ptr += 2;
      *(ptr+3) = '\0';
      address = atoi( ptr) - 1;

      ci = &(dq->ch_info[address]);

      ptr += 4;
      if( *ptr != 'A')
        {
          ci->ch_mode = CH_MODE_DAC_SKIP;
        }
      else
        {
          ptr += 3;

          switch( *ptr)
            {
            case 'C':
              ci->ch_mode = CH_MODE_DAC_COM;
              break;
            case 'T':
              ci->ch_mode = CH_MODE_DAC_TRANS;
              break;
            default:
              ci->ch_mode = CH_MODE_DAC_UNKNOWN;
            }
        }

      ptr = strchr( ptr, '\n') + 1;
    }


  // grab the relay mode information
  mw100_simple_writer( &(dq->comm), "XD?\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      ptr += 2;
      *(ptr+3) = '\0';
      address = atoi( ptr) - 1;
      ptr += 4;

      ci = &(dq->ch_info[address]);
      if( ci->ch_status == CH_STATUS_SKIP)
        {
          ci->ch_mode = CH_MODE_RELAY_SKIP;
        }
      else
        {
          switch( *ptr)
            {
            case 'A':
              ci->ch_mode = CH_MODE_RELAY_ALARM;
              break;
            case 'C':
              ci->ch_mode = CH_MODE_RELAY_COM;
              break;
            case 'M':
              ci->ch_mode = CH_MODE_RELAY_MEDIA;
              break;
            case 'F':
              ci->ch_mode = CH_MODE_RELAY_FAIL;
              break;
            case 'E':
              ci->ch_mode = CH_MODE_RELAY_ERROR;
              break;
            default:
              ci->ch_mode = CH_MODE_RELAY_UNKNOWN;
            }
        }

      ptr = strchr( ptr, '\n') + 1;
    }




  // grab the calculation expressions

  mw100_simple_writer( &(dq->comm), "SO?\r\n");
  if( response_reader( dq) != RESPONSE_ASCII)
    return 1;

  ptr = dq->comm.inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      ptr += 3;
      *(ptr+3) = '\0';
      address = atoi( ptr) - 1;
      ptr += 5;
      if(*ptr == 'F')
        {
          if( address < 60)
            {
              dq->calc_expr[address].on_flag = 0;
              dq->calc_expr[address].expr[0] = '\0';
            }
          else
            {
              dq->short_calc_expr[address-60].on_flag = 0;
              dq->short_calc_expr[address-60].expr[0] = '\0';
            }
        }
      else
        {
          char *p;

          ptr += 2;
          p = strchr(ptr, ',');
          *p = '\0';

          // printf("%d %s\n", address, p);
          if( address < 60)
            {
              dq->calc_expr[address].on_flag = 1;
              strcpy(dq->calc_expr[address].expr, ptr);
            }
          else
            {
              dq->short_calc_expr[address-60].on_flag = 1;
              strcpy(dq->short_calc_expr[address-60].expr, ptr);
            }
          ptr = p+1;
        }
      ptr = strchr( ptr, '\n') + 1;
    }



  scanIoRequest(dq->info_ioscanpvt );
 
  return 0;
}



static int input_value_flag( unsigned int value)
{ 
  if( value < 0x7fff7fff)
    return VL_NORMAL;
  else
    {
      switch( value)
        {
        case 0x7fff7fff:
          return VL_OVERRANGE;
          break;
        case 0x80018001:
          return VL_UNDERRANGE;
          break;
        case 0x80028002:
          return VL_SKIP_OFF;
          break;
        case 0x80048004:
          return VL_ERROR;
          break;
        case 0x80058005:
          return VL_UNCERTAIN;
          break;
        }
    }
  return VL_NORMAL; // nothing should get here
}

// type 0 means get all, 1 means data 001-060, 2 means calc A001-A300
static int load_input_values( struct devqueue *dq, int type, int channel )
{
  //  int readlen;

  int length;
  int number_values;

  int i;

  unsigned char *q;

  int address;
  int value;

  int alarms1, alarms2;
  int alarm_flag;
  int io_flag;

  struct channel_data *cd;

  // request values for entire range, only those that exist will return


  switch( type)
    {
    case CMD_READ_ALL_INPUTS:
      mw100_simple_writer( &(dq->comm), "FD1,001,A300\r\n");
      break;
    case CMD_READ_SIGNAL_INPUT:
      sprintf( dq->comm.outbuffer, "FD1,%03d,%03d\r\n", channel, channel);
      mw100_writer( &(dq->comm));
      break;
    case CMD_READ_MATH:
      sprintf( dq->comm.outbuffer, "FD1,A%03d,A%03d\r\n", channel, channel);
      mw100_writer( &(dq->comm));
      break;
    }
  if( response_reader( dq) != RESPONSE_BINARY)
    return 1;

  q = (unsigned char *) (dq->comm.inbuffer + 4);
  length = *((epicsUInt32 *) q);
  number_values = (length - 22)/8;

  q = (unsigned char *) (dq->comm.inbuffer + 12);
  for( i = 0; i < 6; i++)
    dq->input_poll_time[i] = *(q++);

  alarm_flag = 0;
  
  q = (unsigned char *) (dq->comm.inbuffer + 28);
  for( i = 0; i < number_values; i++)
    {
      address = *((epicsUInt16 *) q);
      q += 2;
      alarms1 = *((epicsUInt8 *) q);
      q += 1;
      alarms2 = *((epicsUInt8 *) q);
      q += 1;
      value = *((epicsUInt32 *) q);
      q += 4;

      if( address > 100)
        cd = &dq->calc_data[address - 101];
      else
        cd = &dq->ch_data[address - 1];

      cd->data_status = input_value_flag( value);
      if( cd->data_status)
        cd->value = 0;
      else
        cd->value = value;

      cd->alarm[0] = (alarms1 & 0xF);
      cd->alarm[1] = (alarms1 & 0xF0);
      cd->alarm[2] = (alarms2 & 0xF);
      cd->alarm[3] = (alarms2 & 0xF0);

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
  if( (type == CMD_READ_ALL_INPUTS) || io_flag )
    scanIoRequest(dq->input_ioscanpvt );

  return 0;
}

// CMD_READ_SIGNAL is for DAC channel
static int load_output_values( struct devqueue *dq, int type, int channel )
{
  int length;
  int number_values;
  
  int i;

  unsigned char *q;
  char *p;

  int address;
  int value;

  struct channel_data *cd;

  if( (type == CMD_READ_ALL_OUTPUTS) || (type == CMD_READ_SIGNAL_OUTPUT) )
    {
      if( type == CMD_READ_ALL_OUTPUTS)
        mw100_simple_writer( &(dq->comm), "FO1,001,060\r\n");
      else
        {
          sprintf( dq->comm.outbuffer, "FO1,%03d,%03d\r\n", channel, channel);
          mw100_writer( &(dq->comm));
        }

      if( response_reader( dq) != RESPONSE_BINARY)
        return 1;

      q = (unsigned char *) (dq->comm.inbuffer + 4);
      length = *((epicsUInt32 *) q);
      number_values = (length - 22)/8;

      q = (unsigned char *) (dq->comm.inbuffer + 12);
      for( i = 0; i < 6; i++)
        dq->output_poll_time[i] = *(q++);

      q = (unsigned char *) (dq->comm.inbuffer + 28);
      for( i = 0; i < number_values; i++)
        {
          address = *((epicsUInt16 *) q);
          q += 4;
          value = *((epicsUInt32 *) q);
          q += 4;
          
          cd = &dq->ch_data[address - 1];

          cd->data_status = VL_NORMAL;
          cd->value = value;
        }
    }

  if( (type == CMD_READ_ALL_OUTPUTS) || (type == CMD_READ_COMM) )
    {
      if( type == CMD_READ_ALL_OUTPUTS)
        mw100_simple_writer( &(dq->comm), "CM?\r\n");
      else
        {
          sprintf( dq->comm.outbuffer, "CMC%03d?\r\n", channel);
          mw100_writer( &(dq->comm));
        }
      if( response_reader( dq) != RESPONSE_ASCII)
        return 1;

      p = dq->comm.inbuffer;
      p += 4;
      while( *p != 'E')
        {
          p += 3;
          *(p + 3) = '\0';
          address = atoi(p) - 1;
          p += 4;
          dq->comm_input[address] = atof(p);

          p = strchr( p, '\n') + 1;
        }
    }

  if( (type == CMD_READ_ALL_OUTPUTS) || (type == CMD_READ_CONST) )
    {
      if( type == CMD_READ_ALL_OUTPUTS)
        mw100_simple_writer( &(dq->comm), "SK?\r\n" );
      else
        {
          sprintf( dq->comm.outbuffer, "SKK%02d?\r\n", channel);
          mw100_writer( &(dq->comm));
        }
      if( response_reader( dq) != RESPONSE_ASCII)
        return 1;

      p = dq->comm.inbuffer;
      p += 4;
      while( *p != 'E')
        {
          p += 3;
          *(p + 2) = '\0';
          address = atoi(p) - 1;
          p += 3;
          dq->constant[address] = atof(p);
          
          p = strchr( p, '\n') + 1;
        }
    }

  if( type == CMD_READ_ALL_OUTPUTS)
    scanIoRequest(dq->output_ioscanpvt );

  return 0;
}



static int set_output_value( struct devqueue *dq, int type, int channel,
                             double value)
{
  int sval;

  switch( type)
    {
    case CMD_SET_SIGNAL_OUTPUT:
      if( value < -10.0)
        value = -10.0;
      if( value > 10.0)
        value = 10.0;
      sval = unscaled_value(value, dq->ch_info[channel - 1].scale);

      sprintf( dq->comm.outbuffer, "SP%03d,%d\r\n", channel, sval);
      break;
    case CMD_SET_COMM:
      sprintf( dq->comm.outbuffer, "CMC%03d,%G\r\n", channel, value );
      break;
    case CMD_SET_CONST:
      sprintf( dq->comm.outbuffer, "SKK%02d,%G\r\n", channel, value );
      break;
    }

  mw100_writer( &(dq->comm));
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

  sprintf( dq->comm.outbuffer, "VD%03d,%s\r\n", channel, value ? "ON" : "OFF" );
  mw100_writer( &(dq->comm));
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

      sprintf( dq->comm.outbuffer, "DS%c\r\n", value ? '1' : '0');
      mw100_writer( &(dq->comm));
      if( response_reader( dq) != RESPONSE_OK)
        return 1;
    }
  else if( type == CMD_SET_COMPUTE)
    {
      if( (value < 0) || (value > 3) )
        return 1;
      sprintf( dq->comm.outbuffer, "EX%c\r\n", '0' + ((char) value));
      mw100_writer( &(dq->comm));
      if( response_reader( dq) != RESPONSE_OK)
        return 1;
    }


  /* printf("out = %d\n", value); fflush(stdout); */

  return 0;
}

static int clear_error(struct devqueue *dq)
{
  mw100_simple_writer( &(dq->comm), "CE0\r\n");
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  dq->error_flag = 0;
  dq->error = NULL;

  scanIoRequest(dq->error_ioscanpvt );

  return 0;
}

static int acknowledge_alarms(struct devqueue *dq)
{
  mw100_simple_writer( &(dq->comm), "AK0\r\n");
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  // just do this in case scan is slow
  load_input_values(dq, CMD_READ_ALL_INPUTS, 0);

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
    case CMD_READ_ALL_INPUTS: // all channels
    case CMD_READ_SIGNAL_INPUT:
    case CMD_READ_MATH:
      load_input_values( dq, cmd_id, channel);
      break;
    case CMD_READ_ALL_OUTPUTS: // all channels
    case CMD_READ_SIGNAL_OUTPUT:
    case CMD_READ_COMM:
    case CMD_READ_CONST:
      load_output_values( dq, cmd_id, channel);
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


static int init_mw100( char *device, char *address)
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
  
  qlp = mw100_queue_list;
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
  qlp->next = mw100_queue_list;
  mw100_queue_list = qlp;

  dq = calloc( 1, sizeof(struct devqueue) );
  if( dq == NULL)
    return 1;
  qlp->dq = dq;
  pthread_mutex_init( &(dq->list_lock), NULL);
  pthread_cond_init( &(dq->list_cond), NULL);
  dq->list = NULL;

  dq->name = strdup( device);
  scanIoInit( &(dq->input_ioscanpvt));
  scanIoInit( &(dq->output_ioscanpvt));
  scanIoInit( &(dq->info_ioscanpvt));
  scanIoInit( &(dq->status_ioscanpvt));
  scanIoInit( &(dq->error_ioscanpvt));
  dq->comm.sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero( &(dq->comm.r_addr), sizeof(dq->comm.r_addr) );
  dq->comm.r_addr.sin_family = AF_INET;
  dq->comm.r_addr.sin_port = htons(34318);
  if( !inet_aton( address, &(dq->comm.r_addr.sin_addr)) )
    {
      free(dq);
      return 1;
    }

  if( mw100_socket_connect( &(dq->comm)) )
    return 1;

  // initial E0 after socket connect
  if( response_reader( dq) != RESPONSE_OK)
    return 1;

  // set binary mode
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
  mw100_simple_writer( &(dq->comm), "BO1\r\n"); 
#else
  mw100_simple_writer( &(dq->comm), "BO0\r\n"); 
#endif
  if( response_reader( dq) != RESPONSE_OK)
    return 1;


  // the order below has meaning, load_modules must go first
  if( load_modules(dq) || load_status(dq) || load_infos(dq) || 
      load_input_values(dq, CMD_READ_ALL_INPUTS, 0) ||
      load_output_values(dq, CMD_READ_ALL_OUTPUTS, 0) )
    return 1;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread, &attr, queue_func, (void *) dq);
  pthread_attr_destroy( &attr);


  return 0;
}




/////////////////////////////////
// device functions

int mw100_test_module( struct devqueue *dq, int module)
{
  if( !dq->modules[module].use_flag)
    return 1;
  
  return 0;
}

int mw100_test_signal( struct devqueue *dq, int channel)
{
  if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
      (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
    return 1;
  
  return 0;
}

int mw100_test_analog_signal( struct devqueue *dq, int channel)
{
  if( (dq->ch_type[channel-1] != CH_TYPE_OUTPUT_ANALOG) &&
      (dq->ch_type[channel-1] != CH_TYPE_INPUT_ANALOG) )
    return 1;
  
  return 0;
}

int mw100_test_binary_signal( struct devqueue *dq, int channel)
{
  if( (dq->ch_type[channel-1] != CH_TYPE_OUTPUT_BINARY) &&
      (dq->ch_type[channel-1] != CH_TYPE_INPUT_BINARY) )
    return 1;
  
  return 0;
}

int mw100_test_integer_signal( struct devqueue *dq, int channel)
{
  if(dq->ch_type[channel-1] != CH_TYPE_INPUT_INTEGER)
    return 1;
  
  return 0;
}

int mw100_test_output_analog_signal( struct devqueue *dq, int channel)
{
  if( dq->ch_type[channel-1] != CH_TYPE_OUTPUT_ANALOG)
    return 1;
  
  return 0;
}

int mw100_test_output_binary_signal( struct devqueue *dq, int channel)
{
  if( dq->ch_type[channel-1] != CH_TYPE_OUTPUT_BINARY)
    return 1;
  
  return 0;
}

struct devqueue *mw100_connect( char *device)
{
  struct queue_link *qlp;

  qlp = mw100_queue_list;
  while( qlp != NULL)
    {
      if( !strcmp(qlp->dq->name, device))
        return qlp->dq;
      qlp = qlp->next;
    }

  return NULL;
}

IOSCANPVT mw100_channel_io_handler( struct devqueue *dq, int type, int channel)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      switch( dq->ch_type[channel-1])
        {
        case CH_TYPE_INPUT_ANALOG:
        case CH_TYPE_INPUT_INTEGER:
        case CH_TYPE_INPUT_BINARY:
          return dq->input_ioscanpvt;
          break;
        case CH_TYPE_OUTPUT_BINARY:
        case CH_TYPE_OUTPUT_ANALOG:
          return dq->output_ioscanpvt;
          break;
        }
      break;
    case ADDR_MATH:
      return dq->input_ioscanpvt;
      break;
    case ADDR_COMM:
    case ADDR_CONST:
      return dq->output_ioscanpvt;
      break;
    }

  return NULL;
}
IOSCANPVT mw100_info_io_handler( struct devqueue *dq)
{
  return dq->info_ioscanpvt;
}
IOSCANPVT mw100_status_io_handler( struct devqueue *dq)
{
  return dq->status_ioscanpvt;
}
IOSCANPVT mw100_error_io_handler( struct devqueue *dq)
{
  return dq->error_ioscanpvt;
}
// just for alarm flag
IOSCANPVT mw100_input_io_handler( struct devqueue *dq)
{
  return dq->input_ioscanpvt;
}


// val or str is used depending on type, other is NULL
int mw100_module_info( struct devqueue *dq, int type, int module, 
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
        sprintf( str, "empty");
      else
        strcpy( str, dq->modules[module].module_string);
      return 0;
    }
  if( !dq->modules[module].use_flag)
    return 0;
    
  switch( type)
    {
    case MODULE_MODEL:
      *val = dq->modules[module].model;
      break;
    case MODULE_CODE:
      strcpy( str, dq->modules[module].code);
      break;
    case MODULE_SPEED:
      *val = dq->modules[module].speed;
      break;
    case MODULE_NUMBER:
      *val = dq->modules[module].number;
      break;
    }

  return 0;
}


int mw100_system_info( struct devqueue *dq, int which, char *info)
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


int mw100_analog_set(struct devqueue *dq, dbCommon *precord, int type, 
                      int channel, double value)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      if( dq->ch_type[channel-1] == CH_TYPE_OUTPUT_ANALOG)
        qmesg( dq, precord, CMD_SET_SIGNAL_OUTPUT, channel,
               datum_float(value));
      break;
    case ADDR_COMM:
      qmesg( dq, precord, CMD_SET_COMM, channel, datum_float(value));
      break;
    case ADDR_CONST:
      qmesg( dq, precord, CMD_SET_CONST, channel, datum_float(value));
      break;
    }    

  return 0;
}

int mw100_binary_set(struct devqueue *dq, dbCommon *precord, int channel, 
                     int value)
{
  if( dq->ch_type[channel-1] == CH_TYPE_OUTPUT_BINARY)
    qmesg( dq, precord, CMD_SET_BINARY_OUTPUT, channel, datum_int(value));

  return 0;
}

int mw100_trigger( struct devqueue *dq, dbCommon *precord, int type)
{
  /* printf("TRIG "); fflush(stdout); */

  switch(type)
    {
    case TRIG_INFO:
      qmesg( dq, precord, CMD_READ_ALL_INFOS, 0, datum_empty());
      break;
    case TRIG_INPUT:
      qmesg( dq, precord, CMD_READ_ALL_INPUTS, 0, datum_empty());
      break;
    case TRIG_OUTPUT:
      qmesg( dq, precord, CMD_READ_ALL_OUTPUTS, 0, datum_empty());
      break;
    case TRIG_STATUS:
      qmesg( dq, precord, CMD_READ_STATUS, 0, datum_empty());
      break;
    }
  return 0;
}


int mw100_mode_set( struct devqueue *dq, dbCommon *precord, int type,
                    int value)
{
  switch(type)
    {
    case MODE_OPERATING:
      qmesg( dq, precord, CMD_SET_OPMODE, 0, datum_int(value) );
      break;
    case MODE_COMPUTE_CMD:
      qmesg( dq, precord, CMD_SET_COMPUTE, 0, datum_int(value) );
      break;
    }

  return 0;
}


int mw100_clear_error( struct devqueue *dq, dbCommon *precord)
{
  qmesg( dq, precord, CMD_CLEAR_ERROR, 0, datum_empty() );

  return 0;
}


int mw100_acknowledge_alarms( struct devqueue *dq, dbCommon *precord)
{
  qmesg( dq, precord, CMD_ACKNOWLEDGE_ALARMS, 0, datum_empty() );

  return 0;
}



int mw100_channel_start(struct devqueue *dq, dbCommon *precord, int type, 
                            int channel)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      switch(dq->ch_type[channel-1])
        {
        case CH_TYPE_INPUT_ANALOG:
        case CH_TYPE_INPUT_INTEGER:
        case CH_TYPE_INPUT_BINARY:
          qmesg( dq, precord, CMD_READ_SIGNAL_INPUT, channel, datum_empty());
          break;
        case CH_TYPE_OUTPUT_BINARY:
        case CH_TYPE_OUTPUT_ANALOG:
          qmesg( dq, precord, CMD_READ_SIGNAL_OUTPUT, channel, datum_empty());
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
    }    

  return 0;
}

int mw100_analog_get( struct devqueue *dq, int type, int channel, 
                       double *value)
{

  switch(type)
    {
    case ADDR_SIGNAL:
      if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
          (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
        return 0;
      *value = scaled_value( dq->ch_data[channel-1].value,
                             dq->ch_info[channel-1].scale);
      break;
    case ADDR_MATH:
      *value = scaled_value( dq->calc_data[channel-1].value,
                             dq->calc_info[channel-1].scale);
      break;
    case ADDR_COMM:
      *value = (double) dq->comm_input[channel-1];
      break;
    case ADDR_CONST:
      *value = (double) dq->constant[channel-1];
      break;
    }    


  return 0;
}

int mw100_integer_get( struct devqueue *dq, int channel, int *value)
{
  if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
      (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
    return 0;
  *value = (int) dq->ch_data[channel-1].value;

  return 0;
}

int mw100_binary_get( struct devqueue *dq, int channel, int *value)
{
  if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
      (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
    return 0;
  *value = (int) dq->ch_data[channel-1].value;

  return 0;
}


int mw100_channel_get_egu( struct devqueue *dq, int type, int channel, 
                           char *egu)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
          (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
        return 0;
      strcpy( egu, dq->ch_info[channel-1].unit);
      break;
    case ADDR_MATH:
      strcpy( egu, dq->calc_info[channel-1].unit);
      break;
    }    

  return 0;
}

int mw100_channel_get_expr( struct devqueue *dq, int channel, char *expr)
{
  if( channel <= 60)
    strcpy( expr, dq->calc_expr[channel-1].expr );
  else
    strcpy( expr, dq->short_calc_expr[channel-61].expr );

  return 0;
}

int mw100_get_channel_status( struct devqueue *dq, int type, int channel, 
                              int *value)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
          (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
        return 0;
      *value = dq->ch_info[channel-1].ch_status;
      break;
    case ADDR_MATH:
      *value = dq->calc_info[channel-1].ch_status;
      break;
    }    

  return 0;
}

int mw100_get_channel_mode( struct devqueue *dq, int channel, int *value)
{
  if( (dq->ch_type[channel-1] != CH_TYPE_OUTPUT_BINARY) && 
      (dq->ch_type[channel-1] != CH_TYPE_OUTPUT_ANALOG) )
    return 0;
  *value = dq->ch_info[channel-1].ch_mode;

  return 0;
}

int mw100_get_data_status( struct devqueue *dq, int type, int channel, 
                           int *value)
{
  switch(type)
    {
    case ADDR_SIGNAL:
      if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
          (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
        return 0;
      *value = dq->ch_data[channel-1].data_status;
      break;
    case ADDR_MATH:
      *value = dq->calc_data[channel-1].data_status;
      break;
    }    

  return 0;
}

int mw100_get_alarm_flag( struct devqueue *dq, int *value)
{
  *value = dq->alarm_flag;

  return 0;
}

int mw100_get_alarm( struct devqueue *dq, int type, int channel,
                     int sub_channel, int *value)
// sub_channel = 0 get alarm_status for channel
{
  switch(type)
    {
    case ADDR_SIGNAL:
      if( (dq->ch_type[channel-1] == CH_TYPE_NONE) ||
          (dq->ch_type[channel-1] == CH_TYPE_UNKNOWN) )
        return 0;
      if( sub_channel)
        *value = dq->ch_data[channel-1].alarm[sub_channel-1];
      else
        *value = dq->ch_data[channel-1].alarm_status;
      break;
    case ADDR_MATH:
      if( sub_channel)
        *value = dq->calc_data[channel-1].alarm[sub_channel-1];
      else
        *value = dq->calc_data[channel-1].alarm_status;
      break;
    }    

  return 0;
}

int mw100_get_error( struct devqueue *dq, int channel, char *error)
{
  error[0] = '\0';
  if( dq->error_flag )
    {
      if( dq->error != NULL)
        strcpy( error, dq->error->string[channel-1]);
      else if( channel == 0)
        strcpy( error, "Unknown error.");
    }

  return 0;
}

int mw100_get_error_flag( struct devqueue *dq, int *flag)
{
  *flag = dq->error_flag;

  return 0;
}

int mw100_get_mode( struct devqueue *dq, int type, int *value)
{
  switch(type)
    {
    case MODE_SETTINGS:
      *value = dq->settings_mode;
      break;
    case MODE_MEASUREMENT:
      *value = dq->measurement_mode;
      break;
    case MODE_COMPUTE:
      *value = dq->compute_mode;
      break;
    }

  return 0;
}

int mw100_get_status_byte( struct devqueue *dq, int channel, int *value)
{
  *value = dq->status[channel-1];

  return 0;
}






/* Initialization method definitions */

static const iocshArg arg0 = {"netDevice",iocshArgString};
static const iocshArg arg1 = {"address",iocshArgString};
static const iocshArg* args[]= {&arg0,&arg1};
static const iocshFuncDef mw100InitDef = {"mw100Init",2,args};
static void mw100InitFunc(const iocshArgBuf* args)
{
  init_mw100( args[0].sval, args[1].sval);
}

/* Registration method */
static void mw100Register(void)
{
  static int firstTime = 1;

  if( firstTime )
    {
      firstTime = 0;
      iocshRegister( &mw100InitDef,mw100InitFunc );
    }
}


epicsExportRegistrar( mw100Register );
