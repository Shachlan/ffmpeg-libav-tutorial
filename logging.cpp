#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void log(const string fmt, const string log_level, ...) {
  va_list args;
  fprintf(stdout, "{\"level\": \"%s\",", log_level.c_str());
  time_t t;
  time(&t);
  tm *lt = localtime(&t);
  fprintf(stdout, "\"time\": %02d/%02d/%04d %02d:%02d:%02d, ", lt->tm_mday, lt->tm_mon + 1,
          1900 + lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec);
  fprintf(stdout, "\"message\": \"");
  va_start(args, log_level);
  vfprintf(stdout, fmt.c_str(), args);
  va_end(args);
  fprintf(stdout, "\"}\n");
}