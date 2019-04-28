#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void logging(const string fmt, ...) {
  va_list args;
  fprintf(stderr, "LOG: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt.c_str(), args);
  va_end(args);
  fprintf(stderr, "\n");
}