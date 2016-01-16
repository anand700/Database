#include "dberror.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *RC_message;

/* print a message to standard out describing the error */
void 
printError (RC error)
{
  if (RC_message != NULL)
    printf("EC (%i), \"%s\"\n", error, RC_message);
  else
    printf("EC (%i)\n", error);
}

char *
errorMessage (RC error)
{
  char *message;

  if (RC_message != NULL)
    {
      message = (char *) malloc(strlen(RC_message) + 30);
      sprintf(message, "EC (%i), \"%s\"\n", error, RC_message);
    }
  else
    {
      message = (char *) malloc(30);
      sprintf(message, "EC (%i)\n", error);
    }

  return message;
}
