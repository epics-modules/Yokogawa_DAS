#ifndef drvGM10_comm_h
#define drvGM10_comm_h

#include <netinet/in.h>
#include <arpa/inet.h>

#define GM10_INBUFLEN (65536) 

#define GM10_OUTBUFLEN (128) 


enum ResponseType { RESPONSE_OK, RESPONSE_ERROR, RESPONSE_CHAIN_ERRORS,
                    RESPONSE_ASCII, RESPONSE_BINARY, RESPONSE_INVALID };

struct gm10_comm
{
  int sockfd;
  struct sockaddr_in r_addr;

  char inbuffer[GM10_INBUFLEN];
  char outbuffer[GM10_OUTBUFLEN];
  int read_length;
};


int gm10_socket_connect(struct gm10_comm *comm);

int gm10_ok_error_reader( struct gm10_comm *comm);
int gm10_ascii_reader( struct gm10_comm *comm);
int gm10_binary_reader( struct gm10_comm *comm);

int gm10_simple_writer( struct gm10_comm *comm, char *string);
int gm10_writer( struct gm10_comm *comm);

int gm10_response_reader( struct gm10_comm *comm);

#endif

