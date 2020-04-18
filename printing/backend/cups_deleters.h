// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_BACKEND_CUPS_DELETERS_H_
#define PRINTING_BACKEND_CUPS_DELETERS_H_

#include <cups/cups.h>

#include "base/macros.h"

namespace printing {

struct HttpDeleter {
  void operator()(http_t* http) const;
};

struct DestinationDeleter {
  void operator()(cups_dest_t* dest) const;
};

struct DestInfoDeleter {
  void operator()(cups_dinfo_t* info) const;
};

struct OptionDeleter {
  void operator()(cups_option_t* option) const;
};

class JobsDeleter {
 public:
  explicit JobsDeleter(int num_jobs);
  void operator()(cups_job_t* jobs) const;

 private:
  int num_jobs_;

  DISALLOW_COPY_AND_ASSIGN(JobsDeleter);
};

}  // namespace printing

#endif  // PRINTING_BACKEND_CUPS_DELETERS_H_
