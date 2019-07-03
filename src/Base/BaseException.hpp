// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

/// Defines an exception class that can be constructed with a message and arguments.
#define DefineException(name)                                \
  class name : public std::exception {                       \
  public:                                                    \
    name() {}                                                \
    name(std::string format, ...) {                          \
      std::string name_str = #name;                          \
      format = name_str + ": " + format;                     \
      va_list argList;                                       \
      va_start(argList, format);                             \
      vasprintf(&full_description, format.c_str(), argList); \
      va_end(argList);                                       \
    }                                                        \
                                                             \
    const char *what() const noexcept {                      \
      return full_description;                               \
    }                                                        \
                                                             \
    char *full_description;                                  \
  }
