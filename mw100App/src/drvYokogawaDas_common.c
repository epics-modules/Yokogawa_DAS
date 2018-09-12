

#include "drvYokogawaDas_common.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


#include <recSup.h>
#include <dbScan.h>

#include <epicsThread.h>

int yDas_socket_connect(struct yDas_comm *comm)
{
  int result;

  result = connect(comm->sockfd, (struct sockaddr *)&(comm->r_addr), 
                   sizeof(comm->r_addr) );
  // change this!
  if( result == -1)
    return 1;

  return 0;
}



int yDas_ok_error_reader( struct yDas_comm *comm)
{
  int len, addlen;
  int flag;

  // TODO: add timeout for returning 1
  
  flag = 1;
  len = 2;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 
                         YDAS_INBUFLEN - len - 1)) > 0)
    {
      len += addlen;
      if( (len > 1) && 
          (comm->inbuffer[len-2] == '\r') && (comm->inbuffer[len-1] == '\n') )
        {
          flag = 0;
          break;
        }
    }
  comm->inbuffer[len] = '\0';

  comm->read_length = len;

  return flag;
}

int yDas_ascii_reader( struct yDas_comm *comm)
{
  int len, addlen;
  int flag;

  // TODO: add timeout for returning 1
  
  flag = 1;
  len = 2;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 
                         YDAS_INBUFLEN - len - 1)) > 0)
    {
      len += addlen;

      //      printf("%s", comm->inbuffer); fflush(stdout);

      if( (len > 4) && 
          (comm->inbuffer[len-4] == 'E') && (comm->inbuffer[len-3] == 'N') &&
          (comm->inbuffer[len-2] == '\r') && (comm->inbuffer[len-1] == '\n') )
        {
          flag = 0;
          break;
        }
    }
  comm->inbuffer[len] = '\0';

  comm->read_length = len;

  return flag;
}

int yDas_binary_reader( struct yDas_comm *comm)
{  
  int len, addlen;
  uint32_t *datalen;

  len = 2;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 8 - len)) > 0)
    len += addlen;
  if( len != 8)
    return 1;
  datalen = (uint32_t *)(comm->inbuffer + 4);
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 
                         8 + *datalen - len)) > 0)
    len += addlen;
  if( len != (8 + *datalen) )
    return 1;

  comm->read_length = len;

  return 0;
}


int yDas_simple_writer( struct yDas_comm *comm, char *string)
{
  epicsThreadSleep(.02);
  return write( comm->sockfd, string, strlen(string) );
}

int yDas_writer( struct yDas_comm *comm)
{
  epicsThreadSleep(.02);
  return write( comm->sockfd, comm->outbuffer, strlen(comm->outbuffer) );
}





int yDas_response_reader( struct yDas_comm *comm)
{
  if( read( comm->sockfd, comm->inbuffer, 1) <= 0)
    return 1;
  if( comm->inbuffer[0] != 'E')
    {
      //  read all in network buffer and throw away!

      // THIS MIGHT TIMEOUT!!!
      read( comm->sockfd, comm->inbuffer, YDAS_INBUFLEN);
      return RESPONSE_INVALID;
    }

  if( read( comm->sockfd, comm->inbuffer + 1, 1) <= 0)
    return 1;
  //  printf("%s", comm->inbuffer); fflush(stdout);
  switch( comm->inbuffer[1])
    {
    case '0':
      if( yDas_ok_error_reader( comm) )
        return RESPONSE_INVALID;
      return RESPONSE_OK;
      break;

    case '1':
      if( yDas_ok_error_reader( comm) )
        return RESPONSE_INVALID;
      return RESPONSE_ERROR;
      break;

    case '2': // possibly handle correctly in future, only MW100 uses
      if( yDas_ok_error_reader( comm) )
        return RESPONSE_INVALID;

      return RESPONSE_CHAIN_ERRORS;
      break;

    case 'A':
      if( yDas_ascii_reader(comm) )
        return RESPONSE_INVALID;
      else
        return RESPONSE_ASCII;
      break;

    case 'B':
      if( yDas_binary_reader(comm) )
        return RESPONSE_INVALID;
      else
        return RESPONSE_BINARY;
      break;
    }

  // default;
  return RESPONSE_INVALID;
}



////////////////////


union datum datum_empty( void)
{
  union datum dt;
  dt.int_d = 0;
  return dt;
}
union datum datum_int( int val)
{
  union datum dt;
  dt.int_d = val;
  return dt;
}
union datum datum_float( double val)
{
  union datum dt;
  dt.flt_d = val;
  return dt;
}

// nothing uses this yet
// the string needs to be free'd
union datum datum_string( char *val)
{
  union datum dt;
  dt.str_d = strdup(val);
  return dt;
}



struct req_pkt
{
  // this allows queue to serve different device types
  // passes it to command functions as is.
  void *arb_device;
  
  struct dbCommon *precord;
  int cmd_id;
  int channel;

  // a length value would allow arrays, and a length value over one
  // means it an allocated array, making it easier to free for strings...

  union datum value;
    
  struct req_pkt *next;
};

struct queue_processor_struct
{
  struct yDas_queue *queue;
  int (* processor)( void *, int, int, union datum);
};

static void *queue_processor(void *arg)
{
  struct queue_processor_struct *yqp = arg;
  
  struct yDas_queue *queue;
  int (* processor)( void *, int, int, union datum);

  struct req_pkt *pkt;
  struct rset *prset;

  queue = yqp->queue;
  processor = yqp->processor;
  free(arg);
  
  while(1)
    {
      pthread_mutex_lock( &(queue->lock));
      if( queue->list == NULL)
        pthread_cond_wait(&(queue->cond), &(queue->lock));

      pkt = queue->list;
      queue->list = pkt->next;
      pthread_mutex_unlock( &(queue->lock));

      //      if( !error)
      processor( pkt->arb_device, pkt->cmd_id, pkt->channel, pkt->value);

      // this causes the record to process again to get the value
      if( pkt->precord != NULL)
        {
          prset = (struct rset *) (pkt->precord->rset);
          dbScanLock(pkt->precord);
          (*prset->process)(pkt->precord);
          dbScanUnlock(pkt->precord);
        }

      
      free( pkt);
      
      //      printf("%s", pkt->recv);
    }

  return NULL;
}

void yDas_start_queue( struct yDas_queue *queue,
                       int (* processor)( void *, int, int, union datum) )
{
  pthread_t thread;
  pthread_attr_t attr;

  struct queue_processor_struct *yqp;
   
  yqp = malloc( sizeof( struct queue_processor_struct) );
  yqp->queue = queue;
  yqp->processor = processor;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread, &attr, queue_processor, (void *) yqp );
  pthread_attr_destroy( &attr);
}


int yDas_enqueue_cmd( struct yDas_queue *queue, void *device,
                 dbCommon *precord, int cmd, int channel, union datum dt)
{
  struct req_pkt *pkt, *plp;


  pkt = malloc( sizeof( struct req_pkt) );

  pkt->arb_device = device;
  pkt->cmd_id = cmd;
  pkt->channel = channel;
  pkt->precord = precord;
  pkt->value = dt;
  pkt->next = NULL;

  pthread_mutex_lock( &(queue->lock));
  if( queue->list == NULL)
    queue->list = pkt;
  else
    {
      plp = queue->list;
      while( plp->next != NULL)
        plp = plp->next;
      plp->next = pkt;
    }
  pthread_cond_signal( &(queue->cond));
  pthread_mutex_unlock( &(queue->lock));

  return 0;
}
