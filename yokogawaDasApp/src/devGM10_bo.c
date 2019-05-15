/* devGM10_bo.c */

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

#include "drvGM10.h"
#include "devGM10_common.h"


/*Create the dset for devGM10_bo */
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
} devGM10_bo = 
  {
    5,
    NULL,
    init,
    init_record,
    NULL,
    write_bo
  };
epicsExportAddress(dset,devGM10_bo);

struct gmboPvt
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

  
static long init_record(struct boRecord *pgmbo)
{
  struct gmboPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pgmbo->dpvt = calloc( 1, sizeof(struct gmboPvt));
  dpvt = pgmbo->dpvt;
  dpvt->invalid_flag = 1;

  gm10_parse_link( strdup(pgmbo->out.value.instio.string),
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
          if( (i <= 0) || (i > MAX_SIGNAL) )
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
      if( !strcmp("CHAN_TRIG", cmd))
        {
          dpvt->rec_type = REC_TRIG;
          dpvt->sub_type = TRIG_CHANNELS;
        }
      else if( !strcmp("MISC_TRIG", cmd))
        {
          dpvt->rec_type = REC_TRIG;
          dpvt->sub_type = TRIG_MISC;
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
      else if( !strcmp("RECORDING_MODE", cmd))
        {
          dpvt->rec_type = REC_MODE;
          dpvt->sub_type = MODE_RECORDING;
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

  dpvt->dq = gm10_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  // if doesn't exist on device, disable it and set to passive
  if( (dpvt->rec_type == REC_VAL) && (dpvt->sub_type == ADDR_SIGNAL) && 
      gm10_test_output_binary_signal( dpvt->dq, dpvt->channel) )
    return 1;
  
 // if it made it this far, it's okay
  dpvt->invalid_flag = 0;

  if( dpvt->rec_type == REC_VAL)
    {
      int value;

      gm10_binary_get( dpvt->dq, dpvt->channel, &value);
      pgmbo->rval = value;

      pgmbo->udf = FALSE;      
    }

  return(0);
}

static long write_bo(struct boRecord *pgmbo)
{
  struct gmboPvt *dpvt;

  dpvt = (struct gmboPvt *) pgmbo->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  if( pgmbo->pact == TRUE) 
    {
      if( (dpvt->rec_type == REC_TRIG) || (dpvt->rec_type == REC_ERROR) )
        {
          pgmbo->val = 0;
        }

      pgmbo->udf = FALSE;

      return 0;
    }

  pgmbo->pact = TRUE;

  if( dpvt->rec_type == REC_VAL)
    {
      gm10_binary_set( dpvt->dq, (dbCommon *) pgmbo, dpvt->channel, 
                        (int) pgmbo->val);
    }
  else if( dpvt->rec_type == REC_TRIG)
    gm10_trigger( dpvt->dq, (dbCommon *) pgmbo, dpvt->sub_type);
  else if( dpvt->rec_type == REC_MODE)
    gm10_mode_set( dpvt->dq, (dbCommon *) pgmbo, dpvt->sub_type, pgmbo->val );
  else if( dpvt->rec_type == REC_ERROR)
    gm10_clear_error( dpvt->dq, (dbCommon *) pgmbo );
  else if( dpvt->rec_type == REC_ALARM)
    gm10_acknowledge_alarms( dpvt->dq, (dbCommon *) pgmbo);

  return 0;
}

