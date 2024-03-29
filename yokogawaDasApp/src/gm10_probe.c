#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>


enum ModType 
  { MOD_TYPE_UNKNOWN, MOD_TYPE_INPUT_ANALOG, MOD_TYPE_INPUT_DIGITAL, 
    MOD_TYPE_INPUT_PULSE, MOD_TYPE_OUTPUT_ANALOG, MOD_TYPE_OUTPUT_DIGITAL, 
    MOD_TYPE_INPUT_OUTPUT_DIGITAL, MOD_TYPE_PID };

struct module
{
  char module_string[14];

  // broken-out information
  int use_flag;

  enum ModType type; 
  int channel_number[2];  // input, output
  
};


char *location = "$(YOKOGAWA_DAS)/yokogawaDasApp/Db";;

char *ip_address = NULL;
char *prefix = NULL;
char *dau = NULL;
char *handle = NULL;

  
void print_sf(FILE *fptr_subst, FILE *fptr_req, struct module *modules)
{
  int i, m;
 
  fprintf( fptr_subst, "# this file was autogenerated by gm10_probe\n\n");
  fprintf( fptr_req, "# this file was autogenerated by gm10_probe\n\n");
  
  fprintf( fptr_subst,
           "file \"%s/GM10_system.db\"\n"
           "{\n"
           "pattern\n"
           "{ P, DAU, HANDLE }\n"
           "{ \"%s\", \"%s\", \"%s\" }\n"
           "}\n"
           "\n",
           location, prefix, dau, handle);
  
  
  fprintf( fptr_subst,
           "file \"%s/GM10_communication_channel.db\"\n"
           "{\n"
           "pattern\n"
           "{ P, DAU, HANDLE, ADDRESS }\n", location);
  for( i = 1; i <= 50; i++)
    {
      fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"C%03d\" }\n",
               prefix, dau, handle, i);
      fprintf( fptr_req, "%s%s:C%03d:Label\n", prefix, dau, i);
    }
  fprintf( fptr_subst, "}\n\n");

  fprintf( fptr_subst,
           "file \"%s/GM10_constant_channel.db\"\n"
           "{\n"
           "pattern\n"
           "{ P, DAU, HANDLE, ADDRESS }\n", location);
  for( i = 1; i <= 50; i++)
    {
      fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"K%03d\" }\n",
               prefix, dau, handle, i);
      fprintf( fptr_req, "%s%s:K%03d:Label\n", prefix, dau, i);
    }
  fprintf( fptr_subst, "}\n\n");

  fprintf( fptr_subst,
           "file \"%s/GM10_varconstant_channel.db\"\n"
           "{\n"
           "pattern\n"
           "{ P, DAU, HANDLE, ADDRESS }\n", location);
  for( i = 1; i <= 50; i++)
    {
      fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"W%03d\" }\n",
               prefix, dau, handle, i);
      fprintf( fptr_req, "%s%s:W%03d:Label\n", prefix, dau, i);
    }
  fprintf( fptr_subst, "}\n\n");

  fprintf( fptr_subst,
           "file \"%s/GM10_calculation_channel.db\"\n"
           "{\n"
           "pattern\n"
           "{ P, DAU, HANDLE, ADDRESS }\n", location);
  for( i = 1; i <= 50; i++)
    {
      fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"A%03d\" }\n",
               prefix, dau, handle, i);
      fprintf( fptr_req, "%s%s:A%03d:Label\n", prefix, dau, i);
    }
  fprintf( fptr_subst, "}\n\n");


  for( m = 0; m < 10; m++)
    {
      if( !modules[m].use_flag)
        continue;
      if( (modules[m].type == MOD_TYPE_UNKNOWN) || 
          (modules[m].type == MOD_TYPE_PID) )
        continue;

      switch( modules[m].type)
        {
        case MOD_TYPE_INPUT_ANALOG:
          fprintf( fptr_subst,
                   "file \"%s/GM10_analog_input_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location);
          for( i = 1; i <= modules[m].channel_number[0]; i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          break;
        case MOD_TYPE_INPUT_DIGITAL:
          fprintf( fptr_subst,
                   "file \"%s/GM10_digital_input_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location);
          for( i = 1; i <= modules[m].channel_number[0]; i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          break;
        case MOD_TYPE_INPUT_PULSE:
          fprintf( fptr_subst,
                   "file \"%s/GM10_pulse_input_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location);
          for( i = 1; i <= modules[m].channel_number[0]; i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          break;
        case MOD_TYPE_OUTPUT_ANALOG:
          fprintf( fptr_subst,
                   "file \"%s/GM10_analog_output_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location);
          for( i = 1; i <= modules[m].channel_number[1]; i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          break;
        case MOD_TYPE_OUTPUT_DIGITAL:
          fprintf( fptr_subst,
                   "file \"%s/GM10_digital_output_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location );
          for( i = 1; i <= modules[m].channel_number[1]; i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          break;
        case MOD_TYPE_INPUT_OUTPUT_DIGITAL:
          fprintf( fptr_subst,
                   "file \"%s/GM10_digital_input_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location);
          for( i = 1; i <= modules[m].channel_number[0]; i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          fprintf( fptr_subst,
                   "file \"%s/GM10_digital_output_channel.db\"\n"
                   "{\n"
                   "pattern\n"
                   "{ P, DAU, HANDLE, ADDRESS }\n", 
                   location);
          for( i = modules[m].channel_number[0] + 1; 
               i <= (modules[m].channel_number[0] + 
                     modules[m].channel_number[1]); i++)
            {
              fprintf( fptr_subst, "{ \"%s\", \"%s\", \"%s\", \"%04d\" }\n",
                       prefix, dau, handle, i + m*100);
              fprintf( fptr_req, "%s%s:%04d:Label\n", prefix, dau, i + m*100);
            }
          fprintf( fptr_subst, "}\n\n");
          break;
        default:
          continue;
        }
    }

  
}



static int socket_connect(int *sockfd, char *address)
{
  int result;
  int count;
  
  int sfd;
  struct sockaddr_in r_addr;
  char buffer[1024];

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero( &r_addr, sizeof(r_addr) );
  r_addr.sin_family = AF_INET;
  r_addr.sin_port = htons(34434);
  if( !inet_aton( address, &(r_addr.sin_addr)) )
    {
      return 1;
    }
  
  result = connect(sfd, (struct sockaddr *)&r_addr, 
                   sizeof(r_addr) );
  // change this!
  if( result == -1)
    return 1;

  if( (count = read( sfd, buffer, 1023)) <= 0)
    return 1;
  buffer[count] = '\0';

  
  // initial E0
  if( strcmp( buffer, "E0\r\n"))
    {
      puts("Can't read.");
      return 1;
    }

  *sockfd = sfd;
  
  return 0;
}


static int ascii_reader( int sockfd, char *cmd, char *buffer, int bufflen )
{
  int len, addlen;
  int flag;

  int init_flag;
  
  init_flag = 1;
  
  flag = 1;
  len = 0;

  write( sockfd, cmd, strlen(cmd) );
  while( (addlen = read( sockfd, buffer + len, bufflen - len - 1)) > 0)
    {
      len += addlen;

      //      printf("%s", dq->inbuffer); fflush(stdout);
      if( len > 4)
        {
          if( init_flag)
            {
              if( strncmp( buffer, "EA\r\n", 4) )
                break;
              init_flag = 0;
            }
          if( (buffer[len-4] == 'E') && (buffer[len-3] == 'N') &&
              (buffer[len-2] == '\r') && (buffer[len-1] == '\n') )
            {
              flag = 0;
              break;
            }
        }
    }
  buffer[len] = '\0';

  return flag;
}


#define MAX_MODULES (10)


int get_modules( int sockfd, struct module *modules)
{
  char set_message[MAX_MODULES][17];
  char status_message[MAX_MODULES][17];
  char error_message[MAX_MODULES][17];

  int check[MAX_MODULES];

  char *ptr;

  int which;

  char *c;
  int i;

  char buffer[1024];

  if( ascii_reader( sockfd, "FSysConf\r\n", buffer, 1024) )
    return 1;

  ptr = buffer;
  ptr += 4;


 // We are not allowing expandable IO for now (which has different format)
  if( strncmp( ptr, "Unit:00", 7) )
    return 1;
  ptr += 9;

  for( i = 0; i < MAX_MODULES; i++)
    {
      check[i] = 0;
    }
  while( *ptr != 'E')
    {
      which = 10 * (*(ptr++) - '0');
      which += (*(ptr++) - '0');
      ptr++;

      check[which] = 1;

      c = set_message[which];
      for( i = 0; *ptr != ' '; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += (17 - i);

      c = status_message[which];
      for( i = 0; *ptr != ' '; i++)
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += (17 - i);

      c = error_message[which];
      while( *ptr != '\r')
        *(c++) = *(ptr++);
      *c = '\0';
      ptr += 2;
    }


  for( i = 0; i < MAX_MODULES; i++)
    {
      modules[i].use_flag = 0;
      modules[i].module_string[0] = '\0';
    }
  for( i = 0; i < MAX_MODULES; i++)
    {
      if( !check[i])
        continue;

      // the strings have to match
      if( strcmp( set_message[i], status_message[i] ) )
        continue;
      // need no errors
      if( strcmp( error_message[i], "----------------") )
        continue;
      if( !strcmp( set_message[i], "----------------") )
        continue;

      strcpy( modules[i].module_string, set_message[i] );

      ptr = modules[i].module_string;
      if( strncmp( ptr, "GX90", 4) )  // use only GX90 modules
        continue;
      ptr+=4;

      modules[i].type = MOD_TYPE_UNKNOWN;
      switch( *(ptr++) )
        {
        case 'U':
          if( *(ptr++) == 'T')
            modules[i].type = MOD_TYPE_PID;
          break;
        case 'X':
          switch( *(ptr++) )
            {
            case 'A':
              modules[i].type = MOD_TYPE_INPUT_ANALOG;
              break;
            case 'D':
              modules[i].type = MOD_TYPE_INPUT_DIGITAL;
              break;
            case 'P':
              modules[i].type = MOD_TYPE_INPUT_PULSE;
              break;
            }
          break;
        case 'Y':
          switch( *(ptr++) )
            {
            case 'A':
              modules[i].type = MOD_TYPE_OUTPUT_ANALOG;
              break;
            case 'D':
              modules[i].type = MOD_TYPE_OUTPUT_DIGITAL;
              break;
            }
          break;
        case 'W':
          if( *(ptr++) == 'D')
            modules[i].type = MOD_TYPE_INPUT_OUTPUT_DIGITAL;
          else
            continue;
          break;
        }
      if( *(ptr++) != '-')
        continue;

      switch( modules[i].type)
        {
        case MOD_TYPE_INPUT_ANALOG:
        case MOD_TYPE_INPUT_DIGITAL:
        case MOD_TYPE_INPUT_PULSE:
          modules[i].channel_number[0] = atoi(ptr);
          modules[i].channel_number[1] = 0;
          break;
        case MOD_TYPE_OUTPUT_ANALOG:
        case MOD_TYPE_OUTPUT_DIGITAL:
          modules[i].channel_number[0] = 0;
          modules[i].channel_number[1] = atoi(ptr);
          break;
        case MOD_TYPE_INPUT_OUTPUT_DIGITAL:
          {
            int v;
            v = atoi(ptr);
            modules[i].channel_number[0] = v/100;
            modules[i].channel_number[1] = v%100;
          }
          break;
          ////////////////  Need to figure out what to do with PID!
        case MOD_TYPE_PID: 
        case MOD_TYPE_UNKNOWN: 
          modules[i].channel_number[0] = 0;
          modules[i].channel_number[1] = 0;
          continue;
        }

      modules[i].use_flag = 1;
    }
   
  return 0;
}



int main( int argc, char *argv[])
{
  FILE *fptr_subst, *fptr_req;

  struct module modules[MAX_MODULES];

  int sockfd;

  int i;
  
  if( argc != 5)
    {
      puts("gm10_probe (IP address) (P) (DAU) (HANDLE)");
      puts("Probes a Yokogawa GM10 at supplied IP address, and generates EPICS files.\n");

      puts("It will create a substitutions file called \"auto_gm10.substitutions\" and");
      puts("an autosave file called \"auto_gm10.req\" in the current directory.");
      puts("The P, DAU, and HANDLE values correspond to macro values in the GM10 database");
      puts("files, and are used in the generated substitutions and autosave files.  The");
      puts("locations to the database files are given as a relative path from an");
      puts("environment variable, called YOKOGAWA_DAS (up to user to make sure it exists).");
      return 0;
    }

  sockfd = -1;

  ip_address = strdup(argv[1]);
  prefix = strdup(argv[2]);
  dau = strdup(argv[3]);
  handle = strdup(argv[4]);
  
  if( socket_connect( &sockfd, ip_address) )
    {
      printf("Can't connect to the GM10 \"%s\".\n", ip_address);
      return 1;
    }

  if( get_modules( sockfd, modules) )
    {
      printf("Did not find GM10 at that IP address \"%s\".\n", ip_address);
      return 1;
    }
  
  shutdown( sockfd, SHUT_RDWR);
  close( sockfd);

  printf("GM10 unit found at %s\n", ip_address);
  for( i = 0; i < 10; i++)
    {
      if( modules[i].use_flag)
        {
          printf("  position %d: %s\n", i, modules[i].module_string);
        }
      else
        {
          if( modules[i].module_string[0] != '\0')
            printf("  position %d: (unused) %s", i, modules[i].module_string);
          else
            printf("  position %d: empty\n", i);
        }
    }

  fptr_subst = fopen( "auto_gm10.substitutions", "w");
  fptr_req = fopen( "auto_gm10.req", "w");
  if( (fptr_subst == NULL) || (fptr_req == NULL) )
    {
      printf("\nERROR: Can't create output files.\n");
      fclose(fptr_subst);
      fclose(fptr_req);
      return 1;
    }
  print_sf( fptr_subst, fptr_req, modules);

  fclose(fptr_subst);
  fclose(fptr_req);

  return 0;
}

