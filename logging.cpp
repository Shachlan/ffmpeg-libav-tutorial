// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

void log(const string fmt, const string log_level, ...) {
  va_list args;
  fprintf(stdout, "{\"level\": \"%s\",", log_level.c_str());
  ptime t = microsec_clock::universal_time();
  fprintf(stdout, "\"time\": %sZ, ", to_iso_extended_string(t).c_str());
  fprintf(stdout, "\"message\": \"");
  va_start(args, log_level);
  vfprintf(stdout, fmt.c_str(), args);
  va_end(args);
  fprintf(stdout, "\"}\n");
}