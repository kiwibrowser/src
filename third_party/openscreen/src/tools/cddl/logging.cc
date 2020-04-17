// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cddl/logging.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

const char* Logger::MakePrintable(const std::string data) {
  return data.c_str();
}

void Logger::InitializeInstance() {
  this->is_initialized_ = true;

  this->WriteLog("CDDL GENERATION TOOL");
  this->WriteLog("---------------------------------------------\n");
}

void Logger::VerifyInitialized() {
  if (!this->is_initialized_) {
    this->InitializeInstance();
  }
}

Logger::Logger() {
  this->is_initialized_ = false;
  openscreen::platform::LogInit(nullptr);
}
// Static:
Logger* Logger::Get() {
  return Logger::singleton_;
}

// Static:
Logger* Logger::singleton_ = new Logger();
