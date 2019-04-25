#ifndef drvMW100_comm_h
#define drvMW100_comm_h

#include <netinet/in.h>
#include <arpa/inet.h>

#define MW100_INBUFLEN (16384) 
#define MW100_OUTBUFLEN (128) 

enum ResponseType { RESPONSE_OK, RESPONSE_ERROR, RESPONSE_CHAIN_ERRORS,
                    RESPONSE_ASCII, RESPONSE_BINARY, RESPONSE_INVALID };

struct mw100_comm
{
  int sockfd;
  struct sockaddr_in r_addr;

  char inbuffer[MW100_INBUFLEN];
  char outbuffer[MW100_OUTBUFLEN];
  int read_length;
};


int mw100_socket_connect(struct mw100_comm *comm);

int mw100_ok_error_reader( struct mw100_comm *comm);
int mw100_ascii_reader( struct mw100_comm *comm);
int mw100_binary_reader( struct mw100_comm *comm);

int mw100_simple_writer( struct mw100_comm *comm, char *string);
int mw100_writer( struct mw100_comm *comm);

int mw100_response_reader( struct mw100_comm *comm);

#endif

