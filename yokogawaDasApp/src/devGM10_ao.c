/* devGM10_ao.c */

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


#include "aoRecord.h"

#include "drvGM10.h"
#include "devGM10_common.h"

/*Create the dset for devGM10_ao */
static long init();
static long init_record();
static long write_ao();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	write_ao;
  DEVSUPFUN	special_linconv;
} devGM10_ao = 
  {
    6,
    NULL,
    init,
    init_record,
    NULL,
    write_ao,
    NULL /* special_linconv */
  };
epicsExportAddress(dset,devGM10_ao);

enum { REC_VAL };

struct gmaoPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  int rec_type;
  int sub_type;
  int channel;
};


static long init(int pass)
{

 return 0;
}

  
static long init_record(struct aoRecord *pgmao)
{
  struct gmaoPvt *dpvt;
 
  char *cmd, *arg;

  int i;

  pgmao->dpvt = calloc( 1, sizeof(struct gmaoPvt));
  dpvt = pgmao->dpvt;
  dpvt->invalid_flag = 1;

  gm10_parse_link( strdup(pgmao->out.value.instio.string),
                    &dpvt->devname, &cmd, &arg);

  dpvt->dq = gm10_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

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
        case 'C':
          dpvt->sub_type = ADDR_COMM;
          i = atoi( arg+1);
          if( (i <= 0) || (i > MAX_COMM) )
            return 1;
          dpvt->channel = i;
        break;
        case 'K':
          dpvt->sub_type = ADDR_CONST;
          i = atoi( arg+1);
          if( (i <= 0) || (i > MAX_CONST) )
            return 1;
          dpvt->channel = i;
        break;
        case 'W':
          dpvt->sub_type = ADDR_VARCONST;
          i = atoi( arg+1);
          if( (i <= 0) || (i > MAX_VARCONST) )
            return 1;
          dpvt->channel = i;
        break;
        default:
          return 1;
        }
    }
  else 
    return 1;


  // if doesn't exist on device, disable it and set to passive
  if( (dpvt->rec_type == REC_VAL) && (dpvt->sub_type == ADDR_SIGNAL) && 
      gm10_test_output_analog_signal( dpvt->dq, dpvt->channel) )
    return 1;

  // if it made it this far, it's okay
  dpvt->invalid_flag = 0;

  if( dpvt->rec_type == REC_VAL)
    {
      gm10_analog_get( dpvt->dq, dpvt->sub_type, dpvt->channel, &pgmao->val);
      pgmao->udf = FALSE;
    }

  return(2);
}


static long write_ao(struct aoRecord *pgmao)
{
  struct gmaoPvt *dpvt;

  dpvt = (struct gmaoPvt *) pgmao->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  if( pgmao->pact == TRUE)
    {
      pgmao->udf = FALSE;
      
      return 0;
    }

  gm10_analog_set( dpvt->dq, (dbCommon *) pgmao, dpvt->sub_type, 
                   dpvt->channel, (double) pgmao->val);
  pgmao->pact = TRUE;

  return 0;
}

/* static long special_linconv(struct aoRecord *pgmao, int after) */
/* { */
/*   return 0; */
/* } */
