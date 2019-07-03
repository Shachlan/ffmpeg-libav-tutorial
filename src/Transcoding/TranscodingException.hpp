// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

#include <stdio.h>

namespace WRETranscoding {

class TranscodingException : public std::exception {
public:
  TranscodingException(std::string format, ...) {
    format = "TranscodingException: " + format;
    va_list argList;
    va_start(argList, format);
    vasprintf(&full_description, format.c_str(), argList);
    va_end(argList);
  }
  /// Error message of the exception.
  const char* what() const noexcept {
    return full_description;
  }

  /// Description of the error that caused the exception.
  char* full_description;
};

}  // namespace WRETranscoding
