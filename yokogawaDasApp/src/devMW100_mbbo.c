/* devMW100_mbbo.c */

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


#include "mbboRecord.h"

#include "drvMW100.h"
#include "devMW100_common.h"


/*Create the dset for devMW100_mbbo */
static long init();
static long init_record();
/* static long get_ioint_info(); */
static long write_mbbo();
/* static long special_linconv(); */
struct 
{
  long		number;
  DEVSUPFUN	report;
  DEVSUPFUN	init;
  DEVSUPFUN	init_record;
  DEVSUPFUN	get_ioint_info;
  DEVSUPFUN	write_mbbo;
} devMW100_mbbo = 
  {
    5,
    NULL,
    init,
    init_record,
    NULL,
    write_mbbo
  };
epicsExportAddress(dset,devMW100_mbbo);

struct mwmbboPvt
{
  int invalid_flag;

  char *devname;
  devqueue *dq;

  int rec_type;
  int sub_type;
};

enum { REC_MODE };

static long init(int pass)
{

  return 0;
}

  
static long init_record(struct mbboRecord *pmwmbbo)
{
  struct mwmbboPvt *dpvt;
 
  char *cmd, *arg;


  pmwmbbo->dpvt = calloc( 1, sizeof(struct mwmbboPvt));
  dpvt = pmwmbbo->dpvt;
  dpvt->invalid_flag = 1;

  mw100_parse_link( strdup(pmwmbbo->out.value.instio.string),
                    &dpvt->devname, &cmd, &arg);
  
  dpvt->dq = mw100_connect( dpvt->devname);
  if( dpvt->dq == NULL)
    return 1;

  // nothing has an arg yet
  if(arg != NULL)
    return 1;

  if( !strcmp("COMPUTE_CMD", cmd))
    {
      dpvt->rec_type = REC_MODE;
      dpvt->sub_type = MODE_COMPUTE_CMD;
    }
  else 
    return 1;

 // if it made it this far, it's okay
  dpvt->invalid_flag = 0;
 
  return(0);
}

static long write_mbbo(struct mbboRecord *pmwmbbo)
{
  struct mwmbboPvt *dpvt;

  dpvt = (struct mwmbboPvt *) pmwmbbo->dpvt;
  if( dpvt->invalid_flag)
    return 0; // maybe a different value, no idea if anyone uses it

  if( pmwmbbo->pact == TRUE) 
    {
      pmwmbbo->udf = FALSE;

      return 0;
    }

  pmwmbbo->pact = TRUE;
  
  mw100_mode_set( dpvt->dq, (dbCommon *) pmwmbbo, dpvt->sub_type, 
                  pmwmbbo->val );

  return 0;
}

