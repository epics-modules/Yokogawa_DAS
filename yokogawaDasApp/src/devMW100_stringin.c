/* devMW100_stringin.c */

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


#include "stringinRecord.h"

#include "drvMW100.h"
#include "devMW100_common.h"

/*Create the dset for devMW100_stringin */
static long init();
static long init_record();
static long get_ioint_info();
static long read_stringin();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	read_stringin;
  DEVSUPFUN	special_linconv;
} devMW100_stringin = 
  {
    6,
    NULL,
    init,
    init_record,
    get_ioint_info,
    read_stringin,
    NULL /* special_linconv */
  };
epicsExportAddress(dset,devMW100_stringin);

static IOSCANPVT dummy_ioscanpvt;

struct mwstringinPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  //  int io_flag; not really needed, just run everything, no async here

  int rec_type;
  int sub_type;
  int channel;
};


enum { REC_IPADDR, REC_MODULE, REC_UNIT, REC_ERROR, REC_EXPR };

static long init(int pass)
{
  if( pass == 0)
    scanIoInit( &dummy_ioscanpvt);

  return 0;
}

  
static long init_record(struct stringinRecord *pmwstringin)
{
  struct mwstringinPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pmwstringin->dpvt = calloc( 1, sizeof(struct mwstringinPvt));
  dpvt = pmwstringin->dpvt;
  dpvt->invalid_flag = 1;
  //  dpvt->io_flag = 0;

  //  printf("%s\n", pmwstringin->inp.value.instio.string);

  mw100_parse_link( strdup(pmwstringin->inp.value.instio.string),
                    &dpvt->devname, &cmd, &arg);

  //  return 1;
  if( !strcmp("IP_ADDR", cmd))
    { 
      dpvt->rec_type = REC_IPADDR;
      dpvt->sub_type = 0;
      dpvt->channel = 0;
    }
  else if( !strcmp("MODULE_CODE", cmd) || !strcmp("MODULE_STRING", cmd))
    { 
      dpvt->rec_type = REC_MODULE;

      if( !strcmp("MODULE_CODE", cmd) )
        dpvt->sub_type = MODULE_CODE;
      else
        dpvt->sub_type = MODULE_STRING;

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
  else if( !strcmp("ERROR", cmd))
    { 
      dpvt->rec_type = REC_ERROR;
      dpvt->sub_type = 0;

      if( arg == NULL)
        return 1;
      i = atoi(arg);
      if( (i <= 0) || ( i > 3) )
        return 1;
      dpvt->channel = i;
    }
  else if( !strcmp("UNIT", cmd))
    {
      dpvt->rec_type = REC_UNIT;

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

        default:
          return 1;
        }
    }
  else if( !strcmp("EXPR", cmd))
    {
      dpvt->rec_type = REC_EXPR;
      dpvt->sub_type = ADDR_MATH; // not really needed
      if(arg == NULL)
        return 1;
      if( arg[0] != 'A')
        return 1;
      i = atoi( arg+1);
      if( (i <= 0) || (i > 300) )
        return 1;
      dpvt->channel = i;
    }
  else 
    return 1;

  dpvt->dq = mw100_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  if( (dpvt->rec_type == REC_UNIT) && (dpvt->sub_type == ADDR_SIGNAL) &&
      mw100_test_signal( dpvt->dq, dpvt->channel) )
    return 1;

  if( (dpvt->rec_type == REC_MODULE) && (dpvt->sub_type != MODULE_STRING) &&
      mw100_test_module( dpvt->dq, dpvt->channel) )
    return 1;

  // if it made it this far, it's okay
  dpvt->invalid_flag = 0;
  
  return(0);
}

static long get_ioint_info( int cmd, struct stringinRecord *pmwstringin, 
                            IOSCANPVT *ppvt)
{
  struct mwstringinPvt *dpvt;

  dpvt = (struct mwstringinPvt *) pmwstringin->dpvt;
  //  dpvt->io_flag = !cmd;

  if( dpvt->invalid_flag)
    *ppvt = dummy_ioscanpvt;
  else
    {
      // return interrupt handler for queue
      if( dpvt->rec_type == REC_ERROR)
        *ppvt = mw100_error_io_handler(dpvt->dq);
      else
        *ppvt = mw100_info_io_handler(dpvt->dq);
    }

  return(0);
}



static long read_stringin(struct stringinRecord *pmwstringin)
{
  struct mwstringinPvt *dpvt;

  dpvt = (struct mwstringinPvt *) pmwstringin->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  switch( dpvt->rec_type)
    {
    case REC_IPADDR:
      mw100_system_info( dpvt->dq, 0, (char *) &pmwstringin->val);
      break;
    case REC_MODULE:
      mw100_module_info( dpvt->dq, dpvt->sub_type, dpvt->channel, NULL,
                         (char *) &pmwstringin->val);
      break;
    case REC_UNIT:
      mw100_channel_get_egu( dpvt->dq, dpvt->sub_type, dpvt->channel, 
                             (char *) &pmwstringin->val);
      break;
    case REC_ERROR:
      mw100_get_error( dpvt->dq, dpvt->channel, (char *) &pmwstringin->val);
      break;
    case REC_EXPR:
      mw100_channel_get_expr( dpvt->dq, dpvt->channel, 
                              (char *) &pmwstringin->val);
      break;
    }
  pmwstringin->udf = FALSE;

  return 0;
}

/* static long special_linconv(struct stringinRecord *pmwstringin, int after) */
/* { */
/*   return 0; */
/* } */
