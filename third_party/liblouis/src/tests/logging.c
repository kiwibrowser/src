#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "louis.h"

static char log_buffer[1024];
static int log_buffer_pos = 0;

static void
log_to_buffer(int level, const char *message)
{
  switch(level) {
    case LOG_DEBUG:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[DEBUG] %s\n", message);
      break;
    case LOG_INFO:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[INFO] %s\n", message);
      break;
    case LOG_WARN:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[WARN] %s\n", message);
      break;
    case LOG_ERROR:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[ERROR] %s\n", message);
      break;
    case LOG_FATAL:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[FATAL] %s\n", message);
      break;  
  }
}

static int
assert_string_equals(char * expected, char * actual)
{
  if (strcmp(expected, actual))
    {
      printf("Expected \"%s\" but received \"%s\"\n", expected, actual);
      return 1;
    }
  return 0;
}
  
static int
assert_log_buffer_equals(char * expected)
{
  return assert_string_equals(expected, log_buffer);
}

int
main(int argc, char **argv)
{
  lou_registerLogCallback(log_to_buffer);
  log_buffer_pos = 0;
  lou_setLogLevel(LOG_WARN);
  logMessage(LOG_ERROR, "foo");
  logMessage(LOG_INFO, "bar");
  lou_setLogLevel(LOG_INFO);
  logMessage(LOG_INFO, "baz");
  return assert_log_buffer_equals("[ERROR] foo\n[INFO] baz\n");
}
