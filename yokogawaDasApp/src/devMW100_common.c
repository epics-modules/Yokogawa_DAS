#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// this function alters the input linkstr, so give it a copy!
int mw100_parse_link( char *linkstr, char **device, char **cmd, char **arg)
{
  int arg_flag;
  char *p, *q;

  p = linkstr;
  if( (*p == '\0') || isblank(*p))
    return 1;
  q = p;
  while( !isblank(*q) && (*q != '\0'))
    q++;
  *q = '\0';
  *device = p;

  p = q+1;
  while( isblank(*p))
    p++;
  if( *p == '\0')
    return 1;
  q = p;
  arg_flag = 0;
  while( !isblank(*q) && (*q != '\0'))
    {
      if( *q == ':')
        {
          arg_flag = 1;
          break;
        }
      q++;
    }
  *q = '\0';
  *cmd = p;

  if( !arg_flag)
    *arg = NULL;
  else
    {
      p = q+1;
      if(*p == '\0')
        return 1;
      q = p;
      while( !isblank(*q) && (*q != '\0'))
        q++;
      *q = '\0';

      *arg = p;
    }  

  return 0;
}


