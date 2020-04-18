// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/cloud_print_token_store.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace cloud_print {

// Keep the global CloudPrintTokenStore in a TLS slot so it is impossible to
// incorrectly from the wrong thread.
static base::LazyInstance<
    base::ThreadLocalPointer<CloudPrintTokenStore>>::DestructorAtExit lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

// static
CloudPrintTokenStore* CloudPrintTokenStore::current() {
  return lazy_tls.Pointer()->Get();
}

CloudPrintTokenStore::CloudPrintTokenStore() {
  lazy_tls.Pointer()->Set(this);
}

CloudPrintTokenStore::~CloudPrintTokenStore() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  lazy_tls.Pointer()->Set(NULL);
}

void CloudPrintTokenStore::SetToken(const std::string& token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  token_ = token;
}

}  // namespace cloud_print
