/*
  Description
  This module provides support for a multiple device port driver. To
  initialize the driver, the method drvAsynMW100() is called from the
  startup script with the following calling sequence.

  drvAsynMW100(portName,octetPortName,hostAddress)

  Where:
  portName - MW100 Asyn interface port driver name (i.e. "dau1" )
  octetPortName - MW100 Asyn Communication port driver name 
  (i.e. "tcp-dau1" )
  hostAddress - IP address of MW100 unit

  The method dbior can be called from the IOC shell to display the current
  status of the driver.
*/


/* System related include files */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* EPICS system related include files */
#include <iocsh.h>
#include <epicsStdio.h>
#include <cantProceed.h>
#include <epicsEndian.h>
#include <epicsString.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include <errlog.h>
#include <dbAccess.h>

/* EPICS synApps/Asyn related include files */
#include <asynDriver.h>
#include <asynDrvUser.h>
#include <asynInt32.h>
#include <asynFloat64.h>
#include <asynOctet.h>
#include <asynOctetSyncIO.h>
#include <asynCommonSyncIO.h>
#include <asynStandardInterfaces.h>

#include <drvAsynIPPort.h>

//#include "channel_options.h"

#define STRING_MAX (16384)

typedef enum {Octet,Float64,Int32} Type;

static const char *driver = "drvAsynMW100";      /* String for asynPrint */



// data_status
enum { CH_ST_SKIP, CH_ST_NORMAL, CH_ST_DIFF, CH_ST_UNKNOWN };

struct channel_info
{
  char data_status;
  char unit[7];
  char scale;

  //  struct input_options options;
};

// alarm
enum { VL_NORMAL, VL_OVERRANGE, VL_UNDERRANGE, VL_SKIP_OFF, VL_ERROR, 
       VL_UNCERTAIN };

struct channel_data
{
  char alarm_status[5];
  char alarm;
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


  // char expression[60][121];        // A001-A060  120 max
  // char short_expression[240][11];  // A061-A300  10 or 8 max

// enum { EXPR_ST_OFF, EXPR_ST_ON };

// struct expression_info
// {
//   char status;
//   char expression[121];
//   int low_limit;
//   int high_limit;
//   double scale
// }

struct module
{
  // raw information
  char set_message[14];
  char status_message[14];
  char error_message[14];

  // broken-out information
  //  int which; // redundant with array, 1-6
  int use_flag;
  int model; // 110, 112, 114, 115, 120, 125, 0 means empty
  //  int error; // 1=RomError:, 2=CalError:, 3=SlotError:
  char module_code[4]; 
  char speed; // L, M, H
  int number; // currently: 4, 6, 8, 10, 30
};

enum { CH_TYPE_NONE, CH_TYPE_INPUT, CH_TYPE_OUTPUT, CH_TYPE_RELAY, 
       CH_TYPE_UNKNOWN };

/* Declare port driver structure */
struct Port
{
  char* port_name;
  char* octet_port_name;
  char* host_info;

  int init; // really needed??
  //  int interrupt_flag;

  struct
  {
    int io_errors;
    int writes;
  } stats;

  unsigned char status[8];
  int setting_mode; 
  int compute_mode;

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
  /* Asyn info */
  asynUser *pasynUser;
  asynUser *pasynUserTrace;  /* asynUser for asynTrace on this port */
  asynStandardInterfaces asynStdInterfaces;

  unsigned char error_status;  // 0, 1, 2; for E0, E1, E2
  int error_id;
  char **error_message;
  //  struct error_id *error; // for E1, E2 would need array
  // avoid doing multiple commands at once for now
};


/* Declare command structure */
struct Command
{
  const char *tag;
  char addr_flag;
  int (*readFunc)(int addr, int which, Port *pport, void* data, Type Iface);
  int (*writeFunc)(int addr, int which, Port *pport, void* data, Type Iface);
};


/* Public interface forward references */
asynStatus drvAsynMW100(const char *portName, const char *octetPortName,
                        const char *hostAddr);

/* Forward references for asynCommon methods */
static void report(void* ppvt,FILE* fp,int details);
static asynStatus connect(void* ppvt,asynUser* pasynUser);
static asynStatus disconnect(void* ppvt,asynUser* pasynUser);
static asynCommon ifaceCommon = {report,connect,disconnect};

/* Forward references for asynDrvUser methods */
static asynStatus create(void* ppvt,asynUser* pasynUser,const char* drvInfo,
                         const char** pptypeName,size_t* psize);
static asynStatus destroy(void* ppvt,asynUser* pasynUser);
static asynStatus gettype(void* ppvt,asynUser* pasynUser,
                          const char** pptypeName,size_t* psize);
static asynDrvUser ifaceDrvUser = {create,gettype,destroy};

/* Forward references for asynFloat64 methods */
static asynStatus readFloat64(void* ppvt,asynUser* pasynUser,
                              epicsFloat64* value);
static asynStatus writeFloat64(void* ppvt,asynUser* pasynUser,
                               epicsFloat64 value);
static asynFloat64 ifaceFloat64 = {writeFloat64, readFloat64};

/* Forward references for asynInt32 methods */
static asynStatus readInt32(void* ppvt,asynUser* pasynUser,epicsInt32* value);
static asynStatus writeInt32(void* ppvt,asynUser* pasynUser,epicsInt32 value);
static asynInt32 ifaceInt32 =  {writeInt32, readInt32};

/* Forward references for asynOctet methods */
static asynStatus flushOctet( void* ppvt, asynUser* pasynUser);
static asynStatus writeOctet( void* ppvt, asynUser* pasynUser, const char *data,
                              size_t numchars, size_t* nbytes);
static asynStatus readOctet( void* ppvt, asynUser* pasynUser, char* data,
                             size_t maxchars, size_t *nbytes, int *eom);
static asynOctet ifaceOctet = { writeOctet, readOctet, flushOctet};


static int comm_DAU_ok( Port *pport, const char *outbuffer);
static int comm_DAU_ascii( Port *pport, const char *outbuffer, char *inbuffer, 
                           size_t *numread);
static int comm_DAU_binary( Port *pport, const char *outbuffer, char *inbuffer, 
                            size_t *numread);


static int read_dummy(int addr, int which, Port *pport, void *data, 
                      Type Iface);
static int read_status_values(int addr, int which, Port *pport, void *data, 
                              Type Iface);
// static int read_option(int addr, int which, Port *pport, void *data, 
//                        Type Iface);
static int read_input_value(int addr, int which, Port *pport, void *data, 
                            Type Iface);
static int poll_status_values(int addr, int which, Port *pport, void *data, 
                              Type Iface);
static int poll_input_values(int addr, int which, Port *pport, void *data, 
                             Type Iface);
static int poll_output_values(int addr, int which, Port *pport, void *data, 
                              Type Iface);
static int read_cache(int addr, int which, Port *pport, void *data, 
                      Type Iface);
// static int read_DAC(int addr, int which, Port *pport, void *data, 
//                       Type Iface);

static int write_dummy(int addr, int which, Port *pport,void* data, 
                       Type Iface);
static int write_status_values(int addr, int which, Port *pport,void* data, 
                               Type Iface);
// static int write_option(int addr, int which, Port *pport,void* data, 
//                         Type Iface);
static int write_DAC(int addr, int which, Port *pport,void* data, 
                     Type Iface);
static int write_relay(int addr, int which, Port *pport, void* data, 
                       Type Iface);
static int write_input(int addr, int which, Port *pport, void* data, 
                       Type Iface);

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


enum { Dummy, Op_Mode, Compute, Status_Poll, Input_Poll, Output_Poll, Relay, 
       DAC, DAC_Unit, DAC_Status, ADC, ADC_Unit, ADC_Status, ADC_Alarm,
       Math, Math_Unit, Math_Status, Math_Alarm, DI, DI_Status, DI_Alarm, 
       Comm_Input, Constant, Expr, Expr_Status,
       Error_Msg_1, Error_Msg_2, Error_Msg_3, Error_Flag, Error_Clear, 
       Command_Number };

static Command command_table[Command_Number] = 
  {
    // dummy has to be first
    { "DUMMY",       0, read_dummy,         write_dummy      }, 
    { "OP_MODE",     0, read_status_values, write_status_values },
    { "COMPUTE",     0, read_status_values, write_status_values },
    { "STATUS_POLL", 0, poll_status_values, write_dummy      },
    { "INPUT_POLL",  0, poll_input_values,  write_dummy      },
    { "OUTPUT_POLL", 0, poll_output_values, write_dummy      },
    { "RELAY",       1, read_cache,         write_relay      },
    { "DAC",         1, read_cache,         write_DAC        },
    { "DAC_UNIT",    1, read_cache,         write_dummy      },
    { "DAC_STATUS",  1, read_cache,         write_dummy      },
    { "ADC",         1, read_input_value,   write_dummy      },
    { "ADC_UNIT",    1, read_cache,         write_dummy      },
    { "ADC_STATUS",  1, read_cache,         write_dummy      },
    { "ADC_ALARM",   1, read_cache,         write_dummy      },
    { "MATH",        1, read_input_value,   write_dummy      },
    { "MATH_UNIT",   1, read_cache,         write_dummy      },
    { "MATH_STATUS", 1, read_cache,         write_dummy      },
    { "MATH_ALARM",  1, read_cache,         write_dummy      },
    { "DI",          1, read_input_value,   write_dummy      },
    { "DI_STATUS",   1, read_cache,         write_dummy      },
    { "DI_ALARM",    1, read_cache,         write_dummy      },
    { "COMM_INPUT",  1, read_cache,         write_input      },
    { "CONSTANT",    1, read_cache,         write_input      },
    { "EXPR",        1, read_cache,         write_dummy      },
    { "EXPR_STATUS", 1, read_cache,         write_dummy      },
    { "ERROR_MSG_1", 0, read_cache,         write_dummy      },
    { "ERROR_MSG_2", 0, read_cache,         write_dummy      },
    { "ERROR_MSG_3", 0, read_cache,         write_dummy      },
    { "ERROR_FLAG",  0, read_cache,         write_dummy      },
    { "ERROR_CLEAR", 0, read_dummy,         write_dummy      },
  };

////////////////////////////////

void call_interrupts_octet( Port *pport, int number, int *reasons)
{
  if( interruptAccept) 
    {
      ELLLIST *pclientList;
      interruptNode *pnode;

      asynOctetInterrupt *pOctet;
      char octetValue[40];

      int address;
      int i;

      pasynManager->interruptStart(pport->asynStdInterfaces.octetInterruptPvt, 
                                   &pclientList);
      pnode = (interruptNode *)ellFirst(pclientList);
      while(pnode != NULL) 
        {
          pOctet = (asynOctetInterrupt *) pnode->drvPvt;

          for( i = 0; i < number; i++)
            if( pOctet->pasynUser->reason == reasons[i] )
              {
                switch( pOctet->pasynUser->reason)
                  {
                  case ADC_Unit:
                  case DAC_Unit:
                    pasynManager->getAddr(pOctet->pasynUser, &address);
                    address--;
                    if( (pport->ch_type[address] == CH_TYPE_NONE) ||
                        (address < 0) || (address >= 60) )
                      break;
                    strcpy( octetValue, pport->ch_info[address].unit );
                    pOctet->callback(pOctet->userPvt, pOctet->pasynUser, 
                                     octetValue, strlen(octetValue), 
                                     ASYN_EOM_CNT);
                    break;
                  case Math_Unit:
                    pasynManager->getAddr(pOctet->pasynUser, &address);
                    address--;

                    if( (address < 0) || (address >= 300))
                      break;
                    strcpy( octetValue, pport->calc_info[address].unit );
                    pOctet->callback(pOctet->userPvt, pOctet->pasynUser, 
                                     octetValue, strlen(octetValue), 
                                     ASYN_EOM_CNT);
                    break;
                  case Expr:
                    pasynManager->getAddr(pOctet->pasynUser, &address);
                    address--;

                    if( (address < 0) || (address >= 300))
                      break;
                    if( address < 60)
                      strcpy( octetValue, pport->calc_expr[address].expr );
                    else
                      strcpy( octetValue, 
                              pport->short_calc_expr[address-60].expr );
                    pOctet->callback(pOctet->userPvt, pOctet->pasynUser, 
                                     octetValue, strlen(octetValue), 
                                     ASYN_EOM_CNT);
                    break;
                  case Error_Msg_1:
                  case Error_Msg_2:
                  case Error_Msg_3:
                    if( pport->error_status)
                      {
                        if( pport->error_message == NULL)
                          {
                            if(pOctet->pasynUser->reason == Error_Msg_1)
                              sprintf( octetValue, 
                                       "Unknown error #%d.", pport->error_id);
                            else
                              strcpy( octetValue, "");
                          }
                        else
                          strcpy( octetValue, pport->error_message
                                  [pOctet->pasynUser->reason-Error_Msg_1]);
                      }
                    else
                      strcpy( octetValue, "");
                    pOctet->callback(pOctet->userPvt, pOctet->pasynUser, 
                                     octetValue, strlen(octetValue), 
                                     ASYN_EOM_CNT);
                    break;
                  }
                break;
              }

          pnode = (interruptNode *)ellNext(&pnode->node);
        }
      
      pasynManager->interruptEnd(pport->asynStdInterfaces.octetInterruptPvt);
    }
}


void call_interrupts_int32( Port *pport, int number, int *reasons)
{
  if( interruptAccept) 
    {
      ELLLIST *pclientList;
      interruptNode *pnode;

      asynInt32Interrupt *pInt32;
      epicsInt32 int32Value;

      int address;
      int i;

      pasynManager->interruptStart
        (pport->asynStdInterfaces.int32InterruptPvt, &pclientList);
      pnode = (interruptNode *)ellFirst(pclientList);
      while(pnode != NULL) 
        {
          pInt32 = (asynInt32Interrupt *) pnode->drvPvt;

          for( i = 0; i < number; i++)
            if( pInt32->pasynUser->reason == reasons[i] )
              {
                switch( pInt32->pasynUser->reason)
                  {
                  case DI:
                  case ADC_Status:
                  case DI_Status:
                  case ADC_Alarm:
                  case DI_Alarm:
                    pasynManager->getAddr(pInt32->pasynUser, &address);
                    address--;
                    if( (pport->ch_type[address] == CH_TYPE_NONE) ||
                        (address < 0) || (address >= 60) )
                      break;
                    if( pInt32->pasynUser->reason == DI) 
                      int32Value = pport->ch_data[address].value;
                    else if( (pInt32->pasynUser->reason == ADC_Status) ||
                             (pInt32->pasynUser->reason == DI_Status) )
                      int32Value = pport->ch_info[address].data_status;
                    else
                      int32Value = pport->ch_data[address].alarm;
                    pInt32->callback(pInt32->userPvt, pInt32->pasynUser,
                                     int32Value);
                    break;
                  case Math_Status:
                  case Math_Alarm:
                    pasynManager->getAddr(pInt32->pasynUser, &address);
                    address--;

                    if( (address < 0) || (address >= 300))
                      break;
                    if( pInt32->pasynUser->reason == Math_Status) 
                      int32Value = pport->calc_info[address].data_status;
                    else
                      int32Value = pport->calc_data[address].alarm;
                    pInt32->callback(pInt32->userPvt, pInt32->pasynUser,
                                     int32Value);
                    break;
                  case Op_Mode:
                    int32Value = pport->setting_mode;
                    pInt32->callback(pInt32->userPvt, pInt32->pasynUser,
                                     int32Value);
                    break;
                  case Compute:
                    int32Value = pport->compute_mode;
                    pInt32->callback(pInt32->userPvt, pInt32->pasynUser,
                                     int32Value);
                    break;
                  case Error_Flag:
                    int32Value = pport->error_status;
                    pInt32->callback(pInt32->userPvt, pInt32->pasynUser,
                                     int32Value);
                    break;
                  }
                break;
              }

          pnode = (interruptNode *)ellNext(&pnode->node);
        }

      pasynManager->interruptEnd(pport->asynStdInterfaces.int32InterruptPvt);
    }
}


void call_interrupts_float64( Port *pport, int number, int *reasons)
{
  if( interruptAccept) 
    {
      ELLLIST *pclientList;
      interruptNode *pnode;

      asynFloat64Interrupt *pFloat64;
      epicsFloat64 float64Value;

      int address;
      int i;

      pasynManager->interruptStart
        (pport->asynStdInterfaces.float64InterruptPvt, &pclientList);
      pnode = (interruptNode *)ellFirst(pclientList);
      while(pnode != NULL) 
        {
          pFloat64 = (asynFloat64Interrupt *) pnode->drvPvt;

          for( i = 0; i < number; i++)
            if( pFloat64->pasynUser->reason == reasons[i] )
              {
                pasynManager->getAddr(pFloat64->pasynUser, &address);
                address--;
                switch( pFloat64->pasynUser->reason)
                  {
                  case ADC:
                  case DAC:
                    if( (pport->ch_type[address] == CH_TYPE_NONE) ||
                        (address < 0) || (address >= 60) ||
                        (pport->ch_info[address].data_status == CH_ST_SKIP) )
                      break;
                    float64Value = scaled_value( pport->ch_data[address].value,
                                                 pport->ch_info[address].scale);
                    pFloat64->callback(pFloat64->userPvt, pFloat64->pasynUser,
                                       float64Value);
                    break;
                  case Math:
                    if( (address < 0) || (address >= 300) ||
                        (pport->calc_info[address].data_status == CH_ST_SKIP))
                      break;
                    float64Value = 
                      scaled_value( pport->calc_data[address].value,
                                    pport->calc_info[address].scale);
                    pFloat64->callback(pFloat64->userPvt, pFloat64->pasynUser,
                                       float64Value);
                    break;
                  case Comm_Input:
                    if( (address < 0) || (address >= 300))
                      break;
                    float64Value = (double) pport->comm_input[address];
                    pFloat64->callback(pFloat64->userPvt, pFloat64->pasynUser,
                                       float64Value);
                    break;
                  case Constant:
                    if( (address < 0) || (address >= 60))
                      break;
                    float64Value = (double) pport->constant[address];
                    pFloat64->callback(pFloat64->userPvt, pFloat64->pasynUser,
                                       float64Value);
                    break;
                  }
                break;
              }

          pnode = (interruptNode *)ellNext(&pnode->node);
        }

      pasynManager->interruptEnd(pport->asynStdInterfaces.float64InterruptPvt);
    }
}

/////////////////////////////////



int load_modules( Port *pport)
{
  char inbuffer[STRING_MAX]; // max is really 284: 46*6+8
  size_t nread;

  char *ptr;

  int which;
  int type;

  char *c;
  int i,j;
                

  // grab the settings on the available channels
  if( comm_DAU_ascii( pport, "CF0\r\n", inbuffer, &nread) )
    return 1;

  ptr = inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      which = *ptr - '0';
      ptr += 4;

      c = pport->modules[which].set_message;
      for( i = 0; i < 13; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 3;
      
      c = pport->modules[which].status_message;
      for( i = 0; i < 13; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 1;

      c = pport->modules[which].error_message;
      while( *ptr != '\r')
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 2;
    }

  for( i = 0; i < 6; i++)
    {
      pport->modules[i].use_flag = 0;
      // the strings have to match
      if( strcmp( pport->modules[i].set_message, 
                  pport->modules[i].status_message ) )
        continue;
      // need no errors
      if( pport->modules[i].error_message[0] != '\0')
        continue;

      pport->modules[i].use_flag = 1;

      // the dash will halt the conversion
      ptr = pport->modules[i].set_message + 2;
      pport->modules[i].model = atoi(ptr);
      ptr += 4;

      c = pport->modules[i].module_code;
      for( j = 0; j < 3; j++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr++;

      pport->modules[i].speed = *ptr;
      ptr++;
      
      pport->modules[i].number = atoi(ptr);
    }

  for( i = 0; i < 6; i++)
    if( !pport->modules[i].use_flag)
      {
        for( j = 0; j < 10; j++)
          pport->ch_type[10*i + j] = CH_TYPE_NONE;
      }
    else
      {
        switch( pport->modules[i].model )
          {
          case 110:
          case 112:
          case 114:
          case 115:
            type = CH_TYPE_INPUT;
            break;
          case 120:
            type = CH_TYPE_OUTPUT;
            break;
          case 125:
            type = CH_TYPE_RELAY;
            break;
          default:
            type = CH_TYPE_UNKNOWN;
          }
        
        for( j = 0; j < pport->modules[i].number; j++)
          pport->ch_type[10*i + j] = type;
        for( ; j < 10; j++)
          pport->ch_type[10*i + j] = CH_TYPE_NONE;
      }
  
  return 0;
}

int load_infos( Port *pport)
{
  char inbuffer[STRING_MAX]; // max is really 1028: 17*60+8
  size_t nread;

  int data_status;
  char unit[7];
  char scale[4];
  char channel[5];
  
  int address;

  struct channel_info *ci;
  struct channel_data *cd;

  char *ptr;
  int i;

  int tag_octet[4] = { ADC_Unit, Math_Unit, DAC_Unit, Expr };
  int tag_int[4] = { ADC_Status, Math_Status, DI_Status, Expr_Status };

  // grab the settings on the available channels for ADC, DAC, not RELAY
  // as well as the math chennels

  if( comm_DAU_ascii( pport, "FE1,001,A300\r\n", inbuffer, &nread) )
    return 1;

  // printf(inbuffer);
  data_status = CH_ST_UNKNOWN; // put it in some state;

  ptr = inbuffer;
  ptr += 4;
  while( *ptr != 'E')
    {
      switch( *ptr)
        {
        case 'N':
          data_status = CH_ST_NORMAL;
          break;
        case 'D':
          data_status = CH_ST_DIFF;
          break;
        case 'S':
          data_status = CH_ST_SKIP;
          break;
        }
      ptr += 2;
      for( i = 0; i < 4; i++, ptr++)
        channel[i] = *ptr;
      channel[4] = '\0';
      if( channel[0] == 'A')
        {
          address = atoi( &channel[1]) - 1;
          ci = &(pport->calc_info[address]);
          cd = &(pport->calc_data[address]);
        }
      else
        {
          address = atoi( &channel[0]) - 1;
          ci = &(pport->ch_info[address]);
          cd = &(pport->ch_data[address]);
        }

      ci->data_status = data_status;

      if( data_status == CH_ST_SKIP)
        {
          strcpy( ci->unit, "----");
          ci->scale = 0;
          ptr = strchr( ptr, '\n') + 1;

          // since the input won't do this
          cd->alarm = VL_SKIP_OFF;
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


// HERE

  if( comm_DAU_ascii( pport, "SO?\r\n", inbuffer, &nread) )
    return 1;

  ptr = inbuffer;
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
              pport->calc_expr[address].on_flag = 0;
              pport->calc_expr[address].expr[0] = '\0';
            }
          else
            {
              pport->short_calc_expr[address-60].on_flag = 0;
              pport->short_calc_expr[address-60].expr[0] = '\0';
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
              pport->calc_expr[address].on_flag = 1;
              strcpy(pport->calc_expr[address].expr, ptr);
            }
          else
            {
              pport->short_calc_expr[address-60].on_flag = 1;
              strcpy(pport->short_calc_expr[address-60].expr, ptr);
            }
          ptr = p+1;
        }
      ptr = strchr( ptr, '\n') + 1;
    }

  // call interrupt processing routine
  call_interrupts_octet( pport, 4, tag_octet);
  call_interrupts_int32( pport, 4, tag_int);
  
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


int load_status_values( Port *pport)
{
  char inbuffer[STRING_MAX];
  size_t nread;

  int i;
  char *p;

  // request values for entire range, only those that exist will return
  if( comm_DAU_ascii( pport, "IS0\r\n", inbuffer, &nread) )
    return 1;

  p = inbuffer + 4;
  for( i = 0; i < 8; i++)
    {
      pport->status[i] = 100*(*p-'0') + 10*(*(p+1)-'0') + (*(p+2)-'0');
      p += 4;
    }

  /* only status[4] is used right now */

  if( pport->status[4] & 1)
    pport->setting_mode = 1;
  else
    pport->setting_mode = 0;

  if( pport->status[4] & 4 )
    pport->compute_mode = 1;
  else
    pport->compute_mode = 0;

  return 0;
}


int load_input_values( Port *pport)
{
  char inbuffer[STRING_MAX];  // really only need 502 = 8*60+22
  size_t nread;

  int length;
  int number_values;

  int i;

  unsigned char *q;

  int address;
  int alarm;
  int value;

  struct channel_data *cd;

  // request values for entire range, only those that exist will return
  if( comm_DAU_binary( pport, "FD1,001,A300\r\n", inbuffer, &nread) )
    return 1;

  length = *((epicsUInt32 *) (inbuffer + 4));
  number_values = (length - 22)/8;

  q = (unsigned char *) (inbuffer + 12);
  for( i = 0; i < 6; i++)
    pport->input_poll_time[i] = *(q++);

  q = (unsigned char *) (inbuffer + 28);
  for( i = 0; i < number_values; i++)
    {
      address = *((epicsUInt16 *) q);
      q += 2;
      alarm = *((epicsUInt16 *) q);
      q += 2;
      value = *((epicsUInt32 *) q);
      q += 4;

      if( address > 100)
        cd = &pport->calc_data[address - 101];
      else
        cd = &pport->ch_data[address - 1];

      cd->alarm = input_value_flag( value);
      if( cd->alarm)
        cd->value = 0;
      else
        cd->value = value;
    }

  return 0;
}


int load_output_values( Port *pport)
{
  char inbuffer[STRING_MAX];  // really only need 502 = 8*60+22
  size_t nread;

  int length;
  int number_values;
  
  int i;

  unsigned char *q;
  char *p;

  int address;
  int value;

  struct channel_data *cd;

  // request values for entire range, only those that exist will return
  if( comm_DAU_binary( pport, "FO1,001,060\r\n", inbuffer, &nread) )
    return 1;

  length = *((epicsUInt32 *) (inbuffer + 4));
  number_values = (length - 22)/8;

  q = (unsigned char *) (inbuffer + 12);
  for( i = 0; i < 6; i++)
    pport->output_poll_time[i] = *(q++);

  q = (unsigned char *) (inbuffer + 28);
  for( i = 0; i < number_values; i++)
    {
      address = *((epicsUInt16 *) q);
      q += 4;
      value = *((epicsUInt32 *) q);
      q += 4;

      cd = &pport->ch_data[address - 1];

      cd->alarm = VL_NORMAL;
      cd->value = value;
    }



  if( comm_DAU_ascii( pport, "CM?\r\n", inbuffer, &nread) )
    return 1;

  p = inbuffer;
  p += 4;
  while( *p != 'E')
    {
      p += 3;
      *(p + 3) = '\0';
      address = atoi(p) - 1;
      p += 4;
      pport->comm_input[address] = atof(p);

      p = strchr( p, '\n') + 1;
    }


  if( comm_DAU_ascii( pport, "SK?\r\n", inbuffer, &nread) )
    return 1;

  p = inbuffer;
  p += 4;
  while( *p != 'E')
    {
      p += 3;
      *(p + 2) = '\0';
      address = atoi(p) - 1;
      p += 3;
      pport->constant[address] = atof(p);

      p = strchr( p, '\n') + 1;
    }

  return 0;
}


int set_binary_mode( Port *pport)
{
  return comm_DAU_ok( pport, 
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
                   "BO1\r\n"
#else
                   "BO0\r\n"
#endif
                   );
}




/****************************************************************************
 * Define public interface methods
 ****************************************************************************/

asynStatus drvAsynMW100(const char *portName, const char *hostAddr, 
                        const char *octetPortName, int traceMask, 
                        int IOtraceMask)
{
  asynStatus status;
  Port* pport;
  //  int i;
  asynStandardInterfaces *pInterfaces;

  int ioaddr = -1;

  pport = (Port*)callocMustSucceed(1,sizeof(Port),"drvAsynMW100");
  pport->port_name = epicsStrDup(portName);
  pport->octet_port_name = epicsStrDup(octetPortName);
  pport->host_info = (char *)callocMustSucceed(1, strlen(hostAddr)+16, 
                                              "drvAsynMW100");
  strcpy(pport->host_info, hostAddr);
  strcat(pport->host_info, ":34318 tcp");

  status = (asynStatus) drvAsynIPPortConfigure( octetPortName, 
                                                pport->host_info, 0, 0, 1);
  if( status != asynSuccess) 
    {
      errlogPrintf("%s::drvAsynMW100 port %s can't configure IP server port.\n",
                   driver, portName);
      return asynError;
    }

  status = pasynOctetSyncIO->connect(octetPortName,ioaddr, 
                                     &pport->pasynUser,NULL);
  if (status != asynSuccess)
    {
      errlogPrintf("%s::drvAsynMW100 port %s can't connect "
                   "to asynCommon on Octet server %s host %s.\n",
                   driver, portName, octetPortName, pport->host_info);
      return asynError;
    }
  //  pasynOctetSyncIO->setOutputEos(pport->pasynUser, "\r\n", 2);

  pasynTrace->setTraceMask(pport->pasynUser, traceMask);
  pasynTrace->setTraceIOMask(pport->pasynUser, IOtraceMask);

  /* Create asynUser for asynTrace */
  pport->pasynUserTrace = pasynManager->createAsynUser(0, 0);
  pport->pasynUserTrace->userPvt = pport;

  status = 
    pasynManager->registerPort(portName,ASYN_MULTIDEVICE|ASYN_CANBLOCK,1,0,0);
  if( status != asynSuccess) 
    {
      errlogPrintf("%s::drvAsynMW100 port %s can't register port.\n",
                   driver, portName);
      return asynError;
    }

  pInterfaces = &pport->asynStdInterfaces;
    
  /* Initialize interface pointers */
  pInterfaces->common.pinterface    = (void *)&ifaceCommon;
  pInterfaces->drvUser.pinterface   = (void *)&ifaceDrvUser;
  pInterfaces->octet.pinterface     = (void *)&ifaceOctet;
  pInterfaces->int32.pinterface     = (void *)&ifaceInt32;
  pInterfaces->float64.pinterface   = (void *)&ifaceFloat64;

  pInterfaces->octetCanInterrupt    = 1;
  pInterfaces->int32CanInterrupt    = 1;
  pInterfaces->float64CanInterrupt  = 1;

  status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                   pport->pasynUserTrace, 
                                                   pport);
  if (status != asynSuccess) 
    {
      errlogPrintf("%s::drvAsynMW100 port %s"
                   " can't register standard interfaces: %s\n",
                   driver, portName, pport->pasynUserTrace->errorMessage);
      return asynError;
    }

  /* Complete initialization */
  pport->error_status = 0;
  pport->error_id = 0;
  pport->error_message = NULL; 

  pport->init=1;

  load_modules( pport);
  load_infos( pport);
  set_binary_mode( pport);

  load_status_values( pport);
  load_output_values( pport);
  load_input_values( pport);


  return asynSuccess;
}




/****************************************************************************
 * Define private read parameter methods
 ****************************************************************************/

static int read_dummy(int addr, int which, Port *pport, void *data, 
                             Type Iface)
{
  return 0;
}

// static int read_option(int addr, int which, Port *pport, void *data, 
//                               Type Iface)
// {
//   char inbuffer[STRING_MAX];
//   size_t nread;

//   int value;

//   switch( which)
//     {
//     case OP_MODE:
//       if( Iface == Int32)
//         {
//           if( comm_DAU_ascii( pport, "DS?\r\n", inbuffer, &nread) )
//             return 1;
//           if( (inbuffer[4] != 'D') || (inbuffer[5] != 'S') )
//             return 1;

//           value = inbuffer[6] - '0';

//           *(epicsInt32 *) data = value;
//         }
//       break;
//     }

//   return 0;
// }

static int read_status_values(int addr, int which, Port *pport, 
                              void *data, Type Iface)
{
  if( load_status_values( pport) )
    return 1;

  if( Iface == Int32)
    switch( which)
      {
      case Op_Mode:
        *(epicsInt32*)data = pport->setting_mode;
        break;
      case Compute:
        *(epicsInt32*)data = pport->compute_mode;
        break;
      }
  return 0;
}

static int read_input_value(int addr, int which, Port *pport, 
                                   void *data, Type Iface)
{
  int returner;
  char inbuffer[STRING_MAX];
  char outbuffer[64];
  size_t nread;

  int length;
  int number_values;

  int value;
  int alarm;

  struct channel_info *ci;

  unsigned char *q;

  const char *vl_strings[6] = { "Normal", "Over Range", "Under Range", 
                                "Skipped or Off", "Error", "Uncertain" };
  
  if( which == Math)
    {
      sprintf( outbuffer, "FD1,A%03d,A%03d\r\n", addr, addr);
      ci = &(pport->calc_info[addr - 1]);
    }
  else
    {
      sprintf( outbuffer, "FD1,%03d,%03d\r\n", addr, addr);
      ci = &(pport->ch_info[addr - 1]);
    }

  returner = comm_DAU_binary( pport, outbuffer, inbuffer, &nread);
  if( returner ) 
    return 1;

  length = *((epicsUInt32 *) (inbuffer + 4));
  number_values = (length - 22)/8;
  if( number_values != 1) 
    return 1;

  q = (unsigned char *) (inbuffer + 32);
  value = *((epicsUInt32 *) q);
  alarm = input_value_flag( value);
  if( alarm == VL_NORMAL)
    {
      switch( Iface)
        {
        case Int32:
          *(epicsInt32*)data = value;
          break;
        case Float64:
          *(epicsFloat64*)data = scaled_value( value, ci->scale);
          break;
        case Octet:
          sprintf( (char *) data, "%g", scaled_value( value, ci->scale));
          break;
        }
    }
  else
    {
      switch( Iface)
        {
        case Int32:
          *(epicsInt32*)data = 0;
          break;
        case Float64:
          *(epicsFloat64*)data = 0.0;
          break;
        case Octet:
          sprintf( (char *) data, "%s", vl_strings[alarm]);
          break;
        }
    }

  return 0;
}


static int poll_status_values(int addr, int which, Port *pport, void *data, 
                              Type Iface)
{
  int tag_int[2] = { Op_Mode, Compute };

  if( load_status_values( pport) )
    return 1;

  call_interrupts_int32( pport, 2, tag_int);

  // need to do this before altering buffer
  if( Iface == Octet)
    {
      sprintf( (char *) data, "%d:%d:%d:%d:%d:%d:%d:%d", 
               pport->status[0], pport->status[1], pport->status[2],
               pport->status[3], pport->status[4], pport->status[5],
               pport->status[6], pport->status[7] );
    }

  return 0;
}


static int poll_input_values(int addr, int which, Port *pport, 
                                     void *data, Type Iface)
{
  int tag_float[2] = { ADC, Math };
  int tag_int[4] = { ADC_Alarm, Math_Alarm, DI_Alarm, DI };

  if( load_input_values( pport) )
    return 1;

  call_interrupts_float64( pport, 2, tag_float);
  call_interrupts_int32( pport, 4, tag_int);


  // need to do this before altering buffer
  if( Iface == Octet)
    {
      unsigned char *time = pport->input_poll_time;

      sprintf( (char *) data, "20%02d-%02d-%02d %02d:%02d:%02d", 
                         time[0], time[1], time[2], time[3], time[4], time[5]);
    }

  return 0;
}

static int poll_output_values(int addr, int which, Port *pport, void *data, 
                              Type Iface)
{
  int tag_float[3] = { DAC, Comm_Input, Constant };

  if( load_output_values( pport) )
    return 1;
  
  call_interrupts_float64( pport, 3, tag_float);

  // need to do this before altering buffer
  if( Iface == Octet)
    {
      unsigned char *time = pport->output_poll_time;

      sprintf( (char *) data, "20%02d-%02d-%02d %02d:%02d:%02d", 
                         time[0], time[1], time[2], time[3], time[4], time[5]);
    }

  return 0;
}


  // return command_table[which].readFunc(addr, which, pport, (void *) data, Octet, nbytes);

static int read_cache(int addr, int which, Port *pport, void *data, 
                             Type Iface)
{
  int address;

  address = addr - 1;

  if( Iface == Octet) 
    {
      switch(which)
        {
        case Error_Msg_1:
        case Error_Msg_2:
        case Error_Msg_3:
          {
            if( pport->error_status)
              {
                if( pport->error_message == NULL)
                  {
                    if(which == Error_Msg_1)
                      sprintf( (char *) data, "Unknown error #%d.", 
                               pport->error_id);
                    else
                      strcpy( (char *) data, "");
                  }
                else
                  strcpy( (char*) data, 
                          pport->error_message[which - Error_Msg_1]);
              }
            else
              strcpy( (char*) data, "");
          }
          break;
        case ADC_Unit:
        case DAC_Unit:
          if( pport->ch_type[address] != CH_TYPE_NONE)
            sprintf( (char *) data, "%s", pport->ch_info[address].unit);
          break;
        case Math_Unit:
          sprintf( (char *) data, "%s", pport->calc_info[address].unit);
          break;
        case Expr:
          if( address < 60)
            sprintf( (char *) data, "%s", pport->calc_expr[address].expr);
          else
            sprintf( (char *) data, "%s", 
                     pport->short_calc_expr[address-60].expr);
          break;
        }
    }
  if( Iface == Float64)
    {
      switch( which)
        {
        case DAC:
          if( pport->ch_type[address] != CH_TYPE_NONE) 
            *((epicsFloat64 *) data) = 
              scaled_value( pport->ch_data[address].value,
                            pport->ch_info[address].scale);
          break;
        case Comm_Input:
          *((epicsFloat64 *) data) = (double) pport->comm_input[address];
          break;
        case Constant:
          *((epicsFloat64 *) data) = (double) pport->constant[address];
          break;
        }
    }
  if( Iface == Int32)
    {
      switch( which)
        {
        case Relay:
          if( pport->ch_type[address] != CH_TYPE_NONE)
            *((epicsInt32 *) data) = pport->ch_data[address].value;
          break;
        case DAC_Status:
        case ADC_Status:
        case DI_Status:
          if( pport->ch_type[address] != CH_TYPE_NONE)
            *((epicsInt32 *) data) = pport->ch_info[address].data_status;
          break;
        case Math_Status:
          *((epicsInt32 *) data) = pport->calc_info[address].data_status;
          break;
        case ADC_Alarm:
        case DI_Alarm:
          if( pport->ch_type[address] != CH_TYPE_NONE)
            *((epicsInt32 *) data) = pport->ch_data[address].alarm;
          break;
        case Math_Alarm:
          *((epicsInt32 *) data) = pport->calc_data[address].alarm;
          break;
        case Error_Flag:
          *((epicsInt32 *) data) = pport->error_status;
          break;
        case Expr_Status:
          if( address < 60)
            *((epicsInt32 *) data) = pport->calc_expr[address].on_flag;
          else
            *((epicsInt32 *) data) =
              pport->short_calc_expr[address-60].on_flag;
          break;
        }
    }

  return 0;
}


/****************************************************************************
 * Define private write parameter methods
 ****************************************************************************/
static int write_dummy(int addr, int which, Port *pport, void* data, 
                              Type Iface)
{
  int tag_octet[3] = { Error_Msg_1, Error_Msg_2, Error_Msg_3 };
  int tag_int[1] = { Error_Flag };

  if( (which == Error_Clear) && pport->error_status)
    {
      if( comm_DAU_ok( pport, "CE0\r\n" ) )
        return 1;

      pport->error_status = 0;
      pport->error_id = 0;
      pport->error_message = NULL;

      call_interrupts_octet( pport, 3, tag_octet);
      call_interrupts_int32( pport, 1, tag_int);
    }
  
  return 0;
}

static int write_status_values(int addr, int which, Port *pport, void* data, 
                               Type Iface)
{
  char outbuffer[32];
  int ival;

  if( Iface == Int32)
    {
      ival = *((epicsInt32*)data);
      switch( which)
        {
        case Op_Mode:
          if( (ival != 0 ) && (ival != 1) )
            return 1;
          sprintf( outbuffer, "DS%d\r\n", ival);
          if( comm_DAU_ok( pport, outbuffer ) )
            return 1;
          if( !ival)
            {
              if(load_infos( pport) )
                return 1;
            }
          break;
        case Compute:
          if( (ival < 0 ) || (ival > 3) )
            return 1;
          if( ival < 2)
            ival = !ival; // swap 0 and 1 values
          sprintf( outbuffer, "EX%d\r\n", ival);
          if( comm_DAU_ok( pport, outbuffer ) )
            return 1;
          break;
        }
    }

  return 0;
}

// static int write_option(int addr, int which, Port *pport, void* data, 
//                                Type Iface)
// {
//   char outbuffer[16];
//   char inbuffer[STRING_MAX];
//   size_t nread;

//   int response_value;

//   int ival;

//   if( Iface == Int32)
//     {
//       ival = *((epicsInt32*)data);
//       switch( which)
//         {
//         case OP_MODE:
//           if( (ival < 0 ) || (ival > 1) )
//             return 1;
//           // if( comm_DAU_ok( pport, "EX1\r\n" ) )
//             // return 1;
//           sprintf( outbuffer, "DS%d\r\n", ival);
//           if( !ival)
//             if( load_infos( pport) )
//               return 1;
//           break;
//         }
//       return comm_DAU_ok( pport, outbuffer );
//     }


//   return 0;
// }

static int write_DAC(int addr, int which, Port *pport, void* data, 
                            Type Iface)
{
  char outbuffer[32];

  double dval;
  int sval;


  if( Iface != Float64)
    return asynSuccess;

  dval = *((epicsFloat64*)data);
  if( dval < -10.0)
    dval = -10.0;
  if( dval > 10.0)
    dval = 10.0;
  sval = unscaled_value(dval, pport->ch_info[addr - 1].scale);

  sprintf( outbuffer, "SP%03d,%d\r\n", addr, sval);
  //  sprintf( outbuffer, "SsP%03d,%d\r\n", addr, sval);

  return comm_DAU_ok( pport, outbuffer);
}

static int write_relay(int addr, int which, Port *pport, void* data, 
                              Type Iface)
{
  char outbuffer[32];

  int sval;

  if( Iface != Int32)
    return asynSuccess;

  sval = *((epicsInt32*)data);

  sprintf( outbuffer, "VD%03d,%s\r\n", addr, sval ? "ON" : "OFF" );

  //  printf("%d , %s\n", addr, outbuffer);

  return comm_DAU_ok( pport, outbuffer );
}


static int write_input(int addr, int which, Port *pport, void* data, 
                            Type Iface)
{
  char outbuffer[32];

  double dval;

  if( Iface != Float64)
    return asynSuccess;

  dval = *((epicsFloat64*)data);

  switch( which)
    {
    case Comm_Input:
      sprintf( outbuffer, "CMC%03d,%G\r\n", addr, dval );
      break;
    case Constant:
      sprintf( outbuffer, "SKK%02d,%G\r\n", addr, dval );
      break;
    default:
      return 1;
    }

  //  printf("%d , %s\n", addr, outbuffer);

  /* need to change read back value */ // .comm_input[addr]

  return comm_DAU_ok( pport, outbuffer );
}


/****************************************************************************
 * Define private interface asynCommon methods
 ****************************************************************************/
static void report(void* ppvt,FILE* fp,int details)
{
  //  int i;
  Port* pport = (Port*)ppvt;

  fprintf( fp, "MW100 port: %s\n", pport->port_name);
  if( details)
    {
      fprintf( fp, "    host:        %s\n", pport->host_info);
      fprintf( fp, "    I/O errors:  %d\n", pport->stats.io_errors);
      fprintf( fp, "    writes:      %d\n", pport->stats.writes);
      fprintf( fp, "    support %s initialized\n",(pport->init)?"IS":"IS NOT");
    }

}

static asynStatus connect(void* ppvt,asynUser* pasynUser)
{
  pasynManager->exceptionConnect(pasynUser);
  return asynSuccess;
}

static asynStatus disconnect(void* ppvt,asynUser* pasynUser)
{
  pasynManager->exceptionDisconnect(pasynUser);
  return asynSuccess;
}


/****************************************************************************
 * Define private interface asynDrvUser methods
 ****************************************************************************/
static asynStatus create(void* ppvt, asynUser *pasynUser, const char *drvInfo, 
                         const char **pptypeName, size_t *psize)
{
  Port* pport=(Port*)ppvt;
  
  int address;
  int i;
  
  for(i = 0; i < Command_Number; i++) 
    if( !epicsStrCaseCmp( drvInfo, command_table[i].tag) ) 
      {
        pasynUser->reason = i;
        break;
      }

  // should i change this to just assign it a dummy to keep it quiet?
  if( i == Command_Number ) 
    {
      errlogPrintf("%s::create port %s failed to find tag %s\n",
                   driver, pport->port_name, drvInfo);
      pasynUser->reason = 0; // the dummy record
      return asynError;
    }
  
  // only check if expecting an address
  if( command_table[i].addr_flag)
    {  
      pasynManager->getAddr(pasynUser,&address);
      if( (address < 1) || (address > 300) )
        return asynError;
    }

  // if( i == ADC_UNIT)
  //   printf(" ADC_UNIT %d\n", address);

  return asynSuccess;
}


// what the hell do i do here???
static asynStatus gettype(void* ppvt,asynUser* pasynUser,
                          const char** pptypeName,size_t* psize)
{
  if( pptypeName ) 
    *pptypeName = NULL;
  if( psize ) 
    *psize = 0;

  return asynSuccess;
}

static asynStatus destroy(void* ppvt,asynUser* pasynUser)
{
  return asynSuccess;
}


/****************************************************************************
 * Define private interface asynFloat64 methods
 ****************************************************************************/
static asynStatus writeFloat64(void* ppvt,asynUser* pasynUser,
                               epicsFloat64 value)
{
  Port* pport=(Port*)ppvt;
  int address;
  int which = pasynUser->reason;

  if( pport->init==0 ) 
    return asynError;
  if( command_table[which].addr_flag)
    {
      if( pasynManager->getAddr(pasynUser,&address) != asynSuccess) 
        return asynError;
    }
  if( command_table[which].writeFunc(address, which, pport, &value, Float64) )
    return asynError;
  return asynSuccess;
}

static asynStatus readFloat64(void* ppvt,asynUser* pasynUser,
                              epicsFloat64* value)
{
  Port* pport=(Port*)ppvt;
  int address;
  int which = pasynUser->reason;

  if( pport->init==0 ) 
    return asynError;
  if( command_table[which].addr_flag)
    {
      if( pasynManager->getAddr(pasynUser,&address) != asynSuccess) 
        return( asynError );
    }

  if( command_table[which].readFunc(address, which, pport, value, Float64) )
    return asynError;
  return asynSuccess;
}


/****************************************************************************
 * Define private interface asynInt32 methods
 ****************************************************************************/
static asynStatus writeInt32(void *ppvt, asynUser *pasynUser, epicsInt32 value)
{
  Port* pport=(Port*)ppvt;
  int address;
  int which = pasynUser->reason;

  if( pport->init==0 ) 
    return asynError;
  if( command_table[which].addr_flag)
    {
      if( pasynManager->getAddr(pasynUser,&address) != asynSuccess) 
        return( asynError );
    }

  if( command_table[which].writeFunc( address, which, pport, &value, Int32) )
    return asynError;
  return asynSuccess;
}

static asynStatus readInt32(void *ppvt, asynUser *pasynUser, epicsInt32 *value)
{
  Port* pport=(Port*)ppvt;
  int address;
  int which = pasynUser->reason;

  if( pport->init==0 ) 
    return asynError;
  if( command_table[which].addr_flag)
    {
      if( pasynManager->getAddr(pasynUser,&address) != asynSuccess) 
        return( asynError );
    }

  if( command_table[which].readFunc(address, which, pport, value, Int32))
    return asynError;
  return asynSuccess;
}


/****************************************************************************
 * Define private interface asynOctet methods
 ****************************************************************************/
static asynStatus flushOctet(void *ppvt, asynUser* pasynUser)
{
  return asynSuccess;
}

static asynStatus writeOctet(void *ppvt, asynUser *pasynUser, const char *data,
                             size_t numchars, size_t *nbytes)
{
  Port* pport=(Port*)ppvt;
  int address;
  int which = pasynUser->reason;
  
  if( pport->init==0 ) 
    return asynError;
  if( command_table[which].addr_flag)
    {
      if( pasynManager->getAddr(pasynUser,&address) != asynSuccess) 
        return( asynError );
    }

  *nbytes=strlen(data);
  if(command_table[which].writeFunc(address, which, pport, (void *) data, 
                                    Octet) )
    return asynError;
  return asynSuccess;
}

static asynStatus readOctet(void* ppvt,asynUser* pasynUser,char* data,
                            size_t maxchars, size_t* nbytes, int *eom)
{
  char str_buffer[STRING_MAX];  // nothing should be this big!

  int ret;

  Port* pport=(Port*)ppvt;
  int address;
  int which = pasynUser->reason;

  if( pport->init==0 ) 
    return asynError;
  if( command_table[which].addr_flag)
    {
      if( pasynManager->getAddr(pasynUser,&address) != asynSuccess) 
        return( asynError );
    }

  // preemptive assignment in case of a simple return
  *data = '\0';
  *nbytes = 0;
  *eom = ASYN_EOM_CNT;

  ret = command_table[which].readFunc(address, which, pport, 
                                      (void *) str_buffer, Octet);
  if( maxchars <= STRING_MAX)
    str_buffer[maxchars-1] = '\0';  // if longer, chop off
  strcpy( data, str_buffer);
  *nbytes = strlen( (char *) data);
  if(ret)
    return asynError;
  return asynSuccess;
}


///////////////////////////////////////////////
//           Communication functions
///////////////////////////////////////////////


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
static int errors_num = sizeof( errors)/sizeof(error_value);

enum ResponseType { RESPONSE_OK, RESPONSE_ERROR, RESPONSE_CHAIN_ERRORS, 
                    RESPONSE_ASCII, RESPONSE_BINARY, RESPONSE_INVALID };

// check the validity of the response string with a DFA
// type: err_value, 0: 0 ; 1: error number , 2: number of errors, 
//    3: 0, 4: 0, 5: 0
static int response_check(  Port *pport, char *string, size_t *length, 
                            int *err_value)
{
  asynStatus status;

  int type;
  int val;

  size_t pos;
  int state; 
  int done;

  size_t num_read;

  char *p;

  type = 0;
  val = 0;

  done = 0;
  pos = 0;
  state = 1;
  p = string;

  // printf("[length = %d]\n", *length);

  *err_value = 0;
  while(1)
    {
      while( pos <  *length )
        {
          switch( state)
            {
            case 1:
              state = (*p == 'E') ? 2 : -1;
              break;
            case 2:
              if( *p == '0')  // everything is fine, no reply message
                {
                  type = RESPONSE_OK;
                  state = 3;
                }
              else if( *p == '1')  // error on the single command you sent
                {
                  type = RESPONSE_ERROR;
                  state = 6;
                }
              else if( *p == '2')  // error(s) on the multiple commands you sent
                {
                  type = RESPONSE_CHAIN_ERRORS;
                  state = 13;
                }
              else if( *p == 'A')  // ASCII data response
                {
                  type = RESPONSE_ASCII;
                  state = 21;
                }
              else if( *p == 'B')  // binary data response
                {
                  type = RESPONSE_BINARY;
                  state = 28;
                }
              else 
                state = -1;
              break;
            case 3:
            case 28:
              state = (*p == '\r') ? 4 : -1;
              break;
            case 4:
              state = (*p == '\n') ? 5 : -1;
              break;
            case 5:
              // this is the accept state
              // printf("[%d]\n", *length );
              // printf("%s\n", type == RESPONSE_BINARY ? "BINARY": "OTHER");
              if( type == RESPONSE_BINARY)
                {
                  // make sure we have at least 4
                  while( (*length - pos) < 4)
                    {
                      status = pasynOctetSyncIO->read(pport->pasynUser, 
                                                      string + *length, 
                                                      STRING_MAX - *length - 1, 
                                                      1.0, &num_read, NULL);
                      if( (status != asynSuccess) || !num_read)
                        return RESPONSE_INVALID;
                      *length += num_read;
                    }
                  val = *((epicsUInt32 *) p); // load binary data length
                  
                  p += 4;
                  pos += 4;
                  while( ((int) *length) < (8 + val ) )
                    {
                      // printf("]%d -> %d[\n", val, *length - 8);

                      status = pasynOctetSyncIO->read(pport->pasynUser, 
                                                      string + *length, 
                                                      STRING_MAX - *length - 1, 
                                                      1.0, &num_read, NULL);
                      if( (status != asynSuccess) || !num_read)
                        return RESPONSE_INVALID;
                      *length += num_read;
                    }

                  val = 0;
                  // printf("{%d -> %d}\n", val, *length - 8);

                  done = 1;
                }
              else
                {
                  state = -1; 
                }
              break;
            case 6:
            case 10:
            case 13:
              if( *p == ' ')
                state++;
              else
                state = -1;
              break;
            case 7:
            case 8:
            case 9:
            case 14:
            case 15:
            case 17:
            case 18:
            case 19:
              if( isdigit( *p) )
                {
                  // if valid, it has to go through stages 7, 8, and 9 sequentially
                  if( (state >= 7) && (state <= 9) )  
                    {
                      val *= 10;
                      val += (*p - '0');
                    }
                  state++;
                }
              else
                state = -1;
              break;
            case 11:
              state = isprint( *p) ? 12 : -1;
              break;
            case 12:
              if( isprint( *p) )
                break; // stay in state 12
              state = (*p == '\r') ? 4 : -1;
              break;
            case 16:
              // incrementing val here counts number of errors
              val++;
              state = (*p == ':') ? 17 : -1;
              break;
            case 20:
              if( *p == ',')
                state = 14;
              else
                state = (*p == '\r') ? 4 : -1;
                  break;
            case 21:
              state = (*p == '\r') ? 22 : -1;
              break;
            case 22:
            case 25:
              state = (*p == '\n') ? 23 : -1;
              break;
            case 23:
              if(*p == 'E')
                state = 26;
              else
                state = isprint( *p) ? 24 : -1;
                  break;
            case 24:
              // if alphanumeric, stay at 24
              if(*p == '\r')
                state = 25;
              else if( !isprint( *p) )
                state = -1;
              break;
            case 26:
              if(*p == 'N')
                state = 27;
              else
                state = isprint( *p) ? 24 : -1;
                  break;
            case 27:
              if(*p == '\r')
                state = 4;
              else
                state = isprint( *p) ? 24 : -1;
                  break;
            default:
              state = -1;
            }
          if( state == -1)
            return RESPONSE_INVALID;
          if( done)
            break;

          // go to next character
          pos++;
          p++;
        }

      if( state == 5)
        {
          *err_value = val;
          return type;
        }
      else
        {
          // append data to string
          status = pasynOctetSyncIO->read(pport->pasynUser, string + *length, 
                                          STRING_MAX - *length - 1, 1.0, 
                                          &num_read, NULL);

          if( (status != asynSuccess) || !num_read)
            return RESPONSE_INVALID;
          *length += num_read;
        }
    }
}


//  check the validity of the response string with a DFA
//  type: value, 0: 0 ; 1: error number , 2: number of errors, 
//        3: number of message lines, 4: binary message length, 5: bad response
//  reponse_value can be NULL
int comm_DAU( Port *pport, ResponseType type,
              const char *outbuffer, char *inbuffer, size_t *numread)
{
  asynStatus status;
  int returner;

  size_t num_written, num_to_write;

  int response, err_val;
  int i;

  int error;

  int tag_octet[3] = { Error_Msg_1, Error_Msg_2, Error_Msg_3 };
  int tag_int[1] = { Error_Flag };


  // have to bake in a delay to not get the previous response
  epicsThreadSleep(.02);
  returner = 0;

  num_to_write=strlen(outbuffer);
  status = pasynOctetSyncIO->writeRead(pport->pasynUser, outbuffer,
                                       num_to_write, inbuffer, STRING_MAX - 1,
                                       5.0, &num_written, numread, NULL);
  if( num_written != num_to_write ) 
    status = asynError;

  if( status!=asynSuccess )
    {
      pport->stats.io_errors++;
      asynPrint(pport->pasynUserTrace,ASYN_TRACE_ERROR,
                "%s comm_DAU: error %d wrote \"%s\"\n",
                pport->port_name,status,outbuffer);

      returner = 1;
    }
  else
    {
      pport->stats.writes++;

      response = response_check( pport, inbuffer, numread, &err_val);
      inbuffer[*numread]='\0';

      if( response == RESPONSE_INVALID)
        {
          inbuffer[0] = '\0';
          *numread = 0;
        }
      if( response != type) // then error
        {
          returner = 1;

          // if it's not the right type, then it's invalid
          if( response == RESPONSE_ERROR)
            error = err_val;
          else
            error = 0;
          pport->error_status = 1;
          if( pport->error_id != error)
            {
              pport->error_id = error;
              // Can't find error, or zero was specified
              pport->error_message = NULL; 
              for( i = 0; i < errors_num; i++)
                {
                  if( errors[i].id == error)
                    pport->error_message = (char **) errors[i].string;
                }

              call_interrupts_octet( pport, 3, tag_octet);
              call_interrupts_int32( pport, 1, tag_int);
            }
        }
    }

  asynPrint(pport->pasynUserTrace,ASYN_TRACEIO_FILTER,
            "%s comm_DAU: wrote \"%s\" read \"%s\"\n",
            pport->port_name,outbuffer,inbuffer);

  
  return returner;
}

static int comm_DAU_ok( Port *pport, const char *outbuffer)
{
  size_t nread;
  char inbuffer[STRING_MAX];

  return comm_DAU( pport, RESPONSE_OK, outbuffer, inbuffer, &nread);
}

static int comm_DAU_ascii( Port *pport, const char *outbuffer, char *inbuffer, 
                           size_t *numread)
{
  return comm_DAU( pport, RESPONSE_ASCII, outbuffer, inbuffer, numread);
}

static int comm_DAU_binary( Port *pport, const char *outbuffer, char *inbuffer, 
                            size_t *numread)
{
  return comm_DAU( pport, RESPONSE_BINARY, outbuffer, inbuffer, numread);
}



/****************************************************************************
 * Register public methods
 ****************************************************************************/
 
/* Initialization method definitions */
static const iocshArg arg0 = {"Port Name", iocshArgString};
static const iocshArg arg1 = {"Host Address", iocshArgString};
static const iocshArg arg2 = {"Octet Port Name", iocshArgString};
static const iocshArg arg3 = {"Asyn Trace Mask", iocshArgInt};
static const iocshArg arg4 = {"Asyn IO Trace Mask", iocshArgInt};
static const iocshArg* args[]= {&arg0,&arg1,&arg2,&arg3,&arg4};
static const iocshFuncDef drvAsynMW100FuncDef = {"drvAsynMW100",5,args};
static void drvAsynMW100CallFunc(const iocshArgBuf* args)
{
  drvAsynMW100(args[0].sval,args[1].sval,args[2].sval,args[3].ival,
               args[4].ival);
}

/* Registration method */
static void drvAsynMW100Register(void)
{
  static int first_time = 1;

  if( first_time )
    {
      first_time = 0;
      iocshRegister( &drvAsynMW100FuncDef,drvAsynMW100CallFunc );
    }
}
epicsExportRegistrar( drvAsynMW100Register );



