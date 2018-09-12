#ifndef drvYokogawaDas_common_h
#define drvYokogawaDas_common_h

#include <netinet/in.h>
#include <arpa/inet.h>

#include <dbAccess.h>

#define YDAS_INBUFLEN (16384) 
#define YDAS_OUTBUFLEN (128) 


enum ResponseType { RESPONSE_OK, RESPONSE_ERROR, RESPONSE_CHAIN_ERRORS,
                    RESPONSE_ASCII, RESPONSE_BINARY, RESPONSE_INVALID };

struct yDas_comm
{
  int sockfd;
  struct sockaddr_in r_addr;

  char inbuffer[YDAS_INBUFLEN];
  char outbuffer[YDAS_OUTBUFLEN];
  int read_length;
};


int yDas_socket_connect(struct yDas_comm *comm);

int yDas_ok_error_reader( struct yDas_comm *comm);
int yDas_ascii_reader( struct yDas_comm *comm);
int yDas_binary_reader( struct yDas_comm *comm);

int yDas_simple_writer( struct yDas_comm *comm, char *string);
int yDas_writer( struct yDas_comm *comm);

int yDas_response_reader( struct yDas_comm *comm);


/////////////////


union datum
  {
    int int_d;
    double flt_d;
    char *str_d;
  };

union datum datum_empty( void);
union datum datum_int( int val);
union datum datum_float( double val);
union datum datum_string( char *val);


struct yDas_queue
{
  pthread_mutex_t lock;
  pthread_cond_t cond;
  struct req_pkt *list;
};

void yDas_start_queue( struct yDas_queue *queue,
                       int (* processor)( void *, int, int, union datum) );
int yDas_enqueue_cmd( struct yDas_queue *queue, void *device,
                 dbCommon *precord, int cmd, int channel, union datum dt);



#endif

