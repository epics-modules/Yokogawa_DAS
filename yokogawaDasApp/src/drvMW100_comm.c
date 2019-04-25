

#include "drvMW100_comm.h"

#include <unistd.h>
#include <string.h>

#include <epicsThread.h>




int mw100_socket_connect(struct mw100_comm *comm)
{
  int result;

  result = connect(comm->sockfd, (struct sockaddr *)&(comm->r_addr), 
                   sizeof(comm->r_addr) );
  // change this!
  if( result == -1)
    return 1;

  return 0;
}



int mw100_ok_error_reader( struct mw100_comm *comm)
{
  int len, addlen;
  int flag;

  // TODO: add timeout for returning 1
  
  flag = 1;
  len = 2;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 
                         MW100_INBUFLEN - len - 1)) > 0)
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

int mw100_ascii_reader( struct mw100_comm *comm)
{
  int len, addlen;
  int flag;

  // TODO: add timeout for returning 1
  
  flag = 1;
  len = 2;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 
                         MW100_INBUFLEN - len - 1)) > 0)
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

int mw100_binary_reader( struct mw100_comm *comm)
{  
  int len, addlen;
  uint32_t datalen, *dp;

  len = 2;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 8 - len)) > 0)
    len += addlen;
  if( len != 8)
    return 1;

  dp = (uint32_t *)(comm->inbuffer + 4);
  datalen = *dp;
  while( (addlen = read( comm->sockfd, comm->inbuffer + len, 
                         8 + datalen - len)) > 0)
    len += addlen;
  if( len != (8 + datalen) )
    return 1;

  comm->read_length = len;

  return 0;
}


int mw100_simple_writer( struct mw100_comm *comm, char *string)
{
  epicsThreadSleep(.02);
  return write( comm->sockfd, string, strlen(string) );
}

int mw100_writer( struct mw100_comm *comm)
{
  epicsThreadSleep(.02);
  return write( comm->sockfd, comm->outbuffer, strlen(comm->outbuffer) );
}





int mw100_response_reader( struct mw100_comm *comm)
{
  if( read( comm->sockfd, comm->inbuffer, 1) <= 0)
    return 1;
  if( comm->inbuffer[0] != 'E')
    {
      //  read all in network buffer and throw away!

      // THIS MIGHT TIMEOUT!!!
      read( comm->sockfd, comm->inbuffer, MW100_INBUFLEN);
      return RESPONSE_INVALID;
    }

  if( read( comm->sockfd, comm->inbuffer + 1, 1) <= 0)
    return 1;
  //  printf("%s", comm->inbuffer); fflush(stdout);
  switch( comm->inbuffer[1])
    {
    case '0':
      if( mw100_ok_error_reader( comm) )
        return RESPONSE_INVALID;
      return RESPONSE_OK;
      break;

    case '1':
      if( mw100_ok_error_reader( comm) )
        return RESPONSE_INVALID;
      return RESPONSE_ERROR;
      break;

    case '2': // possibly handle correctly in future, only MW100 uses
      if( mw100_ok_error_reader( comm) )
        return RESPONSE_INVALID;

      return RESPONSE_CHAIN_ERRORS;
      break;

    case 'A':
      if( mw100_ascii_reader(comm) )
        return RESPONSE_INVALID;
      else
        return RESPONSE_ASCII;
      break;

    case 'B':
      if( mw100_binary_reader(comm) )
        return RESPONSE_INVALID;
      else
        return RESPONSE_BINARY;
      break;
    }

  // default;
  return RESPONSE_INVALID;
}
