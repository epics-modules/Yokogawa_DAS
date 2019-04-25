/* devMW100_ai.c */

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


#include "aiRecord.h"

#include "drvMW100.h"
#include "devMW100_common.h"

/*Create the dset for devMW100_ai */
static long init();
static long init_record();
static long get_ioint_info();
static long read_ai();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	read_ai;
  DEVSUPFUN	special_linconv;
} devMW100_ai = 
  {
    6,
    NULL,
    init,
    init_record,
    get_ioint_info,
    read_ai,
    NULL /* special_linconv */
  };
epicsExportAddress(dset,devMW100_ai);

static IOSCANPVT dummy_ioscanpvt;

struct mwaiPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  int io_flag;

  int rec_type;
  int sub_type;
  int channel;
};

enum {REC_VAL};


static long init(int pass)
{
  if( pass == 0)
    scanIoInit( &dummy_ioscanpvt);

  return 0;
}

  
static long init_record(struct aiRecord *pmwai)
{
  struct mwaiPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pmwai->dpvt = calloc( 1, sizeof(struct mwaiPvt));
  dpvt = pmwai->dpvt;
  dpvt->invalid_flag = 1;
  dpvt->io_flag = 0;

  //  printf("%s\n", pmwai->inp.value.instio.string);

  mw100_parse_link( strdup(pmwai->inp.value.instio.string),
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
        case 'A':
          dpvt->sub_type = ADDR_MATH;
          i = atoi( arg+1);
          if( (i <= 0) || (i > 300) )
            return 1;
          dpvt->channel = i;
        break;
        case 'C':
          dpvt->sub_type = ADDR_COMM;
          i = atoi( arg+1);
          if( (i <= 0) || (i > 300) )
            return 1;
          dpvt->channel = i;
        break;
        case 'K':
          dpvt->sub_type = ADDR_CONST;
          i = atoi( arg+1);
          if( (i <= 0) || (i > 60) )
            return 1;
          dpvt->channel = i;
        break;
        default:
          return 1;
        }
    }
  else 
    return 1;

  dpvt->dq = mw100_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  // if doesn't exist on device, disable it and set to passive
  if( (dpvt->rec_type == REC_VAL) && (dpvt->sub_type == ADDR_SIGNAL) && 
      mw100_test_analog_signal( dpvt->dq, dpvt->channel) )
    return 1;

  dpvt->invalid_flag = 0;

  return 0;
}

static long get_ioint_info( int cmd, struct aiRecord *pmwai, IOSCANPVT *ppvt)
{
  struct mwaiPvt *dpvt;

  dpvt = (struct mwaiPvt *) pmwai->dpvt;
  dpvt->io_flag = !cmd;

  if( dpvt->invalid_flag)
    *ppvt = dummy_ioscanpvt;
  else
    // return interrupt handler for queue
    //    if( dpvt->rec_type == REC_VAL)
    *ppvt = mw100_channel_io_handler(dpvt->dq, dpvt->sub_type, dpvt->channel);

  return(0);
}



static long read_ai(struct aiRecord *pmwai)
{
  struct mwaiPvt *dpvt;

  dpvt = (struct mwaiPvt *) pmwai->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  if( (pmwai->pact == TRUE) || dpvt->io_flag) 
    {
      //    if( dpvt->rec_type == REC_VAL)
      mw100_analog_get( dpvt->dq, dpvt->sub_type, dpvt->channel, &pmwai->val);
      pmwai->udf = FALSE;

      return 2;
    }

  pmwai->pact = TRUE;

  // queue reading, give record to process later
  //    if( dpvt->rec_type == REC_VAL)
  mw100_channel_start( dpvt->dq, (dbCommon *) pmwai, dpvt->sub_type, 
                           dpvt->channel);

  return 0;
}

/* static long special_linconv(struct aiRecord *pmwai, int after) */
/* { */
/*   return 0; */
/* } */
