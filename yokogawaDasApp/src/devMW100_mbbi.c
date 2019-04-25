/* devMW100_mbbi.c */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "dbEvent.h"
#include "dbScan.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "link.h"
#include "callback.h"
#include "epicsExport.h"


#include "mbbiRecord.h"

#include "drvMW100.h"
#include "devMW100_common.h"

/*Create the dset for devMW100_mbbi */
static long init();
static long init_record();
static long get_ioint_info();
static long read_mbbi();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	read_mbbi;
} devMW100_mbbi = 
  {
    5,
    NULL,
    init,
    init_record,
    get_ioint_info,
    read_mbbi
  };
epicsExportAddress(dset,devMW100_mbbi);

static IOSCANPVT dummy_ioscanpvt;

struct mwmbbiPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  //  int io_flag; not really needed, just run everything, no async here

  int rec_type;
  int sub_type;
  int channel;
  int sub_channel;
};

enum { REC_CH_STATUS, REC_CH_MODE, REC_VAL_STATUS, REC_ALARM, REC_ALARMS,
       REC_MODULE};


static long init(int pass)
{
  if( pass == 0)
    scanIoInit( &dummy_ioscanpvt);

  return 0;
}


static long init_record(struct mbbiRecord *pmwmbbi)
{
  struct mwmbbiPvt *dpvt;
 
  char *cmd, *arg;

  int i;
  char *p;

  pmwmbbi->dpvt = calloc( 1, sizeof(struct mwmbbiPvt));
  dpvt = pmwmbbi->dpvt;
  dpvt->invalid_flag = 1;
  //  dpvt->io_flag = 0;

  //  printf("%s\n", pmwmbbi->inp.value.instio.string);

  mw100_parse_link( strdup(pmwmbbi->inp.value.instio.string),
                    &dpvt->devname, &cmd, &arg);

  dpvt->dq = mw100_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  if( !strcmp("MODULE_SPEED", cmd) )
    {
      dpvt->rec_type = REC_MODULE;
      dpvt->sub_type = MODULE_SPEED;

      if( arg == NULL)
        return 1;

      // as atoi returns 0 on error, and 0 is a valid address,
      // this makes sure there is a valid number stub
      // but I don't want to disallow 004, even though that would be weird
      if( (*arg < '0') || (*arg > '5') )
        return 1;
      i = atoi(arg);
      if( (i < 0) || ( i > 5) )
        return 1;
      dpvt->channel = i;
    }
  else
    {
      if( !strcmp("CH_STATUS", cmd) )
        {
          dpvt->rec_type = REC_CH_STATUS;
        }
      else if( !strcmp("CH_MODE", cmd) )
        {
          dpvt->rec_type = REC_CH_MODE;
        }
      else if( !strcmp("VAL_STATUS", cmd) )
        {
          dpvt->rec_type = REC_VAL_STATUS;
        }
      else if( !strcmp("ALARM", cmd) )
        {
          dpvt->rec_type = REC_ALARM;
        }
      else if( !strcmp("ALARMS", cmd) )
        {
          dpvt->rec_type = REC_ALARMS;
        }
      else
        return 1;

      if(arg == NULL)
        return 1;
      switch(arg[0])
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          dpvt->sub_type = ADDR_SIGNAL;
          i = strtol( arg, &p, 10);
          if( (i <= 0) || (i > 60) )
            return 1;
          dpvt->channel = i;
          break;
        case 'A':
          dpvt->sub_type = ADDR_MATH;
          i = strtol( arg+1, &p, 10);
          if( (i <= 0) || (i > 300) )
            return 1;
          dpvt->channel = i;
          break;
        default:
          return 1;
        }
    }
  

  if( (dpvt->rec_type == REC_MODULE) &&
      mw100_test_module( dpvt->dq, dpvt->channel) )
    return 1;

  if( (dpvt->sub_type == ADDR_SIGNAL) && 
      mw100_test_signal( dpvt->dq, dpvt->channel) )
    return 1;

  if( (dpvt->rec_type == REC_CH_MODE) && (dpvt->sub_type != ADDR_SIGNAL) )
    return 1;
    
  if( dpvt->rec_type == REC_ALARMS)
    dpvt->sub_channel = 0;
  else if( dpvt->rec_type == REC_ALARM)
    {
      if( *p != '.')
        return 1;
      p++;
      if( (*p < '1') || (*p > '4') || (*(p+1) != '\0') )
        return 1;
      dpvt->sub_channel = *p - '0';
    }


  // if it made it this far, it's okay
  dpvt->invalid_flag = 0;
  
  return 0;
}

static long get_ioint_info( int cmd, struct mbbiRecord *pmwmbbi, 
                            IOSCANPVT *ppvt)
{
  struct mwmbbiPvt *dpvt;

  dpvt = (struct mwmbbiPvt *) pmwmbbi->dpvt;
  //  dpvt->io_flag = !cmd;

  if( dpvt->invalid_flag)
    *ppvt = dummy_ioscanpvt;
  else
    {
      switch( dpvt->rec_type)
        {
        case REC_VAL_STATUS:
        case REC_ALARM:
        case REC_ALARMS:
          *ppvt = mw100_channel_io_handler(dpvt->dq, dpvt->sub_type, 
                                           dpvt->channel);
          break;
        case REC_MODULE:
        case REC_CH_STATUS:
        case REC_CH_MODE:
          *ppvt = mw100_info_io_handler(dpvt->dq);
          break;
        }
     
    }
  
  return(0);
}
  


static long read_mbbi(struct mbbiRecord *pmwmbbi)
{
  struct mwmbbiPvt *dpvt;
  int value;


  dpvt = (struct mwmbbiPvt *) pmwmbbi->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  switch( dpvt->rec_type)
    {
    case REC_MODULE:
      mw100_module_info( dpvt->dq, dpvt->sub_type, dpvt->channel, &value,
                         NULL);
      break;
    case REC_CH_STATUS:
      mw100_get_channel_status( dpvt->dq, dpvt->sub_type, dpvt->channel, 
                                &value);
      break;
    case REC_CH_MODE:
      mw100_get_channel_mode( dpvt->dq, dpvt->channel, &value);
      break;
    case REC_VAL_STATUS:
      mw100_get_data_status( dpvt->dq, dpvt->sub_type, dpvt->channel, &value);
      break;
    case REC_ALARM:
    case REC_ALARMS:
      mw100_get_alarm( dpvt->dq, dpvt->sub_type, dpvt->channel,
                       dpvt->sub_channel, &value);
      break;
    }

  pmwmbbi->rval = value;
  pmwmbbi->udf = FALSE;

  return 0;
}

