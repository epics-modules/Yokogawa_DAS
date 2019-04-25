/* devMW100_bo.c */

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


#include "boRecord.h"

#include "drvMW100.h"
#include "devMW100_common.h"


/*Create the dset for devMW100_bo */
static long init();
static long init_record();
/* static long get_ioint_info(); */
static long write_bo();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	write_bo;
} devMW100_bo = 
  {
    5,
    NULL,
    init,
    init_record,
    NULL,
    write_bo
  };
epicsExportAddress(dset,devMW100_bo);

struct mwboPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  int rec_type;
  int sub_type;
  int channel;
};

enum { REC_VAL, REC_TRIG, REC_MODE, REC_ERROR, REC_ALARM };

static long init(int pass)
{

  return 0;
}

  
static long init_record(struct boRecord *pmwbo)
{
  struct mwboPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pmwbo->dpvt = calloc( 1, sizeof(struct mwboPvt));
  dpvt = pmwbo->dpvt;
  dpvt->invalid_flag = 1;

  mw100_parse_link( strdup(pmwbo->out.value.instio.string),
                    &dpvt->devname, &cmd, &arg);
  
  if( !strcmp("VAL", cmd))
    {
      dpvt->rec_type = REC_VAL;
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
          i = atoi( arg);
          if( (i <= 0) || (i > 60) )
            return 1;
          dpvt->channel = i;
          break;
        default:
          return 1;
        }
    }
  else 
    {
      if(arg != NULL)
        return 1;
      if( !strcmp("INP_TRIG", cmd))
        {
          dpvt->rec_type = REC_TRIG;
          dpvt->sub_type = TRIG_INPUT;
        }
      else if( !strcmp("OUT_TRIG", cmd))
        {
          dpvt->rec_type = REC_TRIG;
          dpvt->sub_type = TRIG_OUTPUT;
        }
      else if( !strcmp("INFO_TRIG", cmd))
        {
          dpvt->rec_type = REC_TRIG;
          dpvt->sub_type = TRIG_INFO;
        }
      else if( !strcmp("STAT_TRIG", cmd))
        {
          dpvt->rec_type = REC_TRIG;
          dpvt->sub_type = TRIG_STATUS;
        }
      else if( !strcmp("OPMODE", cmd))
        {
          dpvt->rec_type = REC_MODE;
          dpvt->sub_type = MODE_OPERATING;
        }
      else if( !strcmp("ERROR_CLEAR", cmd))
        {
          dpvt->rec_type = REC_ERROR;
          dpvt->sub_type = 0;
        }
      else if( !strcmp("ALARM_ACK", cmd))
        {
          dpvt->rec_type = REC_ALARM;
          dpvt->sub_type = 0;
        }
      else 
        return 1;
    }

  dpvt->dq = mw100_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  // if doesn't exist on device, disable it and set to passive
  if( (dpvt->rec_type == REC_VAL) && (dpvt->sub_type == ADDR_SIGNAL) && 
      mw100_test_output_binary_signal( dpvt->dq, dpvt->channel) )
    return 1;
  
 // if it made it this far, it's okay
  dpvt->invalid_flag = 0;

  if( dpvt->rec_type == REC_VAL)
    {
      int value;

      mw100_binary_get( dpvt->dq, dpvt->channel, &value);
      pmwbo->rval = value;

      pmwbo->udf = FALSE;      
    }

  return(0);
}

static long write_bo(struct boRecord *pmwbo)
{
  struct mwboPvt *dpvt;

  dpvt = (struct mwboPvt *) pmwbo->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  if( pmwbo->pact == TRUE) 
    {
      if( (dpvt->rec_type == REC_TRIG) || (dpvt->rec_type == REC_ERROR) )
        {
          pmwbo->val = 0;
        }

      pmwbo->udf = FALSE;

      return 0;
    }

  pmwbo->pact = TRUE;

  if( dpvt->rec_type == REC_VAL)
    {
      mw100_binary_set( dpvt->dq, (dbCommon *) pmwbo, dpvt->channel, 
                        (int) pmwbo->val);
    }
  else if( dpvt->rec_type == REC_TRIG)
    mw100_trigger( dpvt->dq, (dbCommon *) pmwbo, dpvt->sub_type);
  else if( dpvt->rec_type == REC_MODE)
    mw100_mode_set( dpvt->dq, (dbCommon *) pmwbo, dpvt->sub_type, pmwbo->val );
  else if( dpvt->rec_type == REC_ERROR)
    mw100_clear_error( dpvt->dq, (dbCommon *) pmwbo );
  else if( dpvt->rec_type == REC_ALARM)
    mw100_acknowledge_alarms( dpvt->dq, (dbCommon *) pmwbo);

  return 0;
}

