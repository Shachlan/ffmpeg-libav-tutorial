#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void log(const string fmt, const string log_level, ...) {
  va_list args;
  fprintf(stdout, "{\"level\": \"%s\",", log_level.c_str());
  fprintf(stdout, "\"message\": \"");
  va_start(args, log_level);
  vfprintf(stdout, fmt.c_str(), args);
  va_end(args);
  fprintf(stdout, "\"}\n");
}