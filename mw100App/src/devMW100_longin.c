/* devMW100_longin.c */

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

#include "drvMW100.h"
#include "devMW100_common.h"

/*Create the dset for devMW100_longin */
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
} devMW100_longin = 
  {
    5,
    NULL,
    init,
    init_record,
    get_ioint_info,
    read_longin
  };
epicsExportAddress(dset,devMW100_longin);

static IOSCANPVT dummy_ioscanpvt;

struct mwlonginPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  //  int io_flag; not really needed, just run everything, no async here

  int rec_type;
  int sub_type;
  int channel;
};

enum { REC_MODULE };


static long init(int pass)
{
  if( pass == 0)
    scanIoInit( &dummy_ioscanpvt);

  return 0;
}


static long init_record(struct longinRecord *pmwlongin)
{
  struct mwlonginPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pmwlongin->dpvt = calloc( 1, sizeof(struct mwlonginPvt));
  dpvt = pmwlongin->dpvt;
  dpvt->invalid_flag = 1;
  //  dpvt->io_flag = 0;

  //  printf("%s\n", pmwlongin->inp.value.instio.string);

  parse_MW100_link( strdup(pmwlongin->inp.value.instio.string),
                    &dpvt->devname, &cmd, &arg);

  if( !strcmp("MODULE_MODEL", cmd) )
    {
      dpvt->rec_type = REC_MODULE;
      dpvt->sub_type = MODULE_MODEL;
    }
  else if( !strcmp("MODULE_NUMBER", cmd) )
    {
      dpvt->rec_type = REC_MODULE;
      dpvt->sub_type = MODULE_NUMBER;
    }
  else
    return 1;

  if( arg == NULL)
    return 1;
  
  //  i = strtol( arg, &p, 10);
  i = atoi(arg);
  if( (i < 0) || ( i > 5) )
    return 1;
  dpvt->channel = i;


  dpvt->dq = mw100_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  // if module doesn't exist, disable it and set to passive
  if( (dpvt->rec_type == REC_MODULE) &&
      mw100_test_module( dpvt->dq, dpvt->channel) )
    return 1;

  // if it made it this far, it's okay
  dpvt->invalid_flag = 0;
  
  return 0;
}

static long get_ioint_info( int cmd, struct longinRecord *pmwlongin, 
                            IOSCANPVT *ppvt)
{
  struct mwlonginPvt *dpvt;

  dpvt = (struct mwlonginPvt *) pmwlongin->dpvt;
  //  dpvt->io_flag = !cmd;

  if( dpvt->invalid_flag)
    *ppvt = dummy_ioscanpvt;
  else
    {
      // only one case right now
      
      /* switch( dpvt->rec_type) */
      /*   { */
      /*   case REC_MODULE: */
      *ppvt = mw100_info_io_handler(dpvt->dq);
        /*   break; */
        /* } */
    }

  return(0);
}



static long read_longin(struct longinRecord *pmwlongin)
{
  struct mwlonginPvt *dpvt;
  int value;


  dpvt = (struct mwlonginPvt *) pmwlongin->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  switch( dpvt->rec_type)
    {
    case REC_MODULE:
      mw100_module_info( dpvt->dq, dpvt->sub_type, dpvt->channel, &value,
                         NULL);
      break;
    }

  pmwlongin->val = value;
  pmwlongin->udf = FALSE;

  return 0;
}

