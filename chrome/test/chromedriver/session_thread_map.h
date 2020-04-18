// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_SESSION_THREAD_MAP_H_
#define CHROME_TEST_CHROMEDRIVER_SESSION_THREAD_MAP_H_

#include <map>
#include <string>

#include "base/memory/linked_ptr.h"
#include "base/threading/thread.h"

typedef std::map<std::string, linked_ptr<base::Thread> > SessionThreadMap;

#endif  // CHROME_TEST_CHROMEDRIVER_SESSION_THREAD_MAP_H_
