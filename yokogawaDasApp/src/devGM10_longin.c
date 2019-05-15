/* devGM10_longin.c */

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


#include "longinRecord.h"

#include "drvGM10.h"
#include "devGM10_common.h"

/*Create the dset for devGM10_longin */
static long init();
static long init_record();
static long get_ioint_info();
static long read_longin();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	read_longin;
} devGM10_longin = 
  {
    5,
    NULL,
    init,
    init_record,
    get_ioint_info,
    read_longin
  };
epicsExportAddress(dset,devGM10_longin);

static IOSCANPVT dummy_ioscanpvt;

struct gmlonginPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  int io_flag;

  int rec_type;
  int sub_type;
  int channel;
};

enum { REC_VAL, REC_MODULE };


static long init(int pass)
{
  if( pass == 0)
    scanIoInit( &dummy_ioscanpvt);

  return 0;
}


static long init_record(struct longinRecord *pgmlongin)
{
  struct gmlonginPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pgmlongin->dpvt = calloc( 1, sizeof(struct gmlonginPvt));
  dpvt = pgmlongin->dpvt;
  dpvt->invalid_flag = 1;
  dpvt->io_flag = 0;
  
  //  printf("%s\n", pgmlongin->inp.value.instio.string);

  gm10_parse_link( strdup(pgmlongin->inp.value.instio.string),
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
    return 1;

  dpvt->dq = gm10_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  // if doesn't exist on device, disable it and set to passive
  if( (dpvt->rec_type == REC_VAL) && (dpvt->sub_type == ADDR_SIGNAL) && 
      gm10_test_integer_signal( dpvt->dq, dpvt->channel) )
    return 1;
  
  // if it made it this far, it's okay
  dpvt->invalid_flag = 0;
  
  return 0;
}

static long get_ioint_info( int cmd, struct longinRecord *pgmlongin, 
                            IOSCANPVT *ppvt)
{
  struct gmlonginPvt *dpvt;

  dpvt = (struct gmlonginPvt *) pgmlongin->dpvt;
  dpvt->io_flag = !cmd;

  if( dpvt->invalid_flag)
    *ppvt = dummy_ioscanpvt;
  else
    {
      /* switch( dpvt->rec_type) */
      /*   { */
      /*   case REC_VAL: */
      *ppvt = gm10_address_io_handler(dpvt->dq, dpvt->sub_type);
        /*   break; */
        /* } */
    }

  return 0;
}



static long read_longin(struct longinRecord *pgmlongin)
{
  struct gmlonginPvt *dpvt;
  int value;


  dpvt = (struct gmlonginPvt *) pgmlongin->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  /* if( dpvt->rec_type == REC_VAL) */
  /*   { */

  if( (pgmlongin->pact == TRUE) || dpvt->io_flag) 
    {
      //    if( dpvt->rec_type == REC_VAL)
      gm10_integer_get( dpvt->dq, dpvt->channel, &value);
      pgmlongin->val = value;
      pgmlongin->udf = FALSE;
          
      return 0;
    }

  // this one is asynchronous
  pgmlongin->pact = TRUE;
  // can use the generic start function, as it's only converted to 
  // binary at end
  gm10_channel_start( dpvt->dq, (dbCommon *) pgmlongin, dpvt->sub_type, 
                      dpvt->channel);
      
    /*   return 0; */
    /* } */


  return 0;
}

