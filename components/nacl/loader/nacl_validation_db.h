// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_LOADER_NACL_VALIDATION_DB_H_
#define COMPONENTS_NACL_LOADER_NACL_VALIDATION_DB_H_

#include <string>

#include "base/macros.h"

class NaClValidationDB {
 public:
  NaClValidationDB() {}
  virtual ~NaClValidationDB() {}

  virtual bool QueryKnownToValidate(const std::string& signature) = 0;
  virtual void SetKnownToValidate(const std::string& signature) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(NaClValidationDB);
};

#endif  // COMPONENTS_NACL_LOADER_NACL_VALIDATION_DB_H_
