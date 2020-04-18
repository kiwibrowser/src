// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REVERSE_H_
#define REVERSE_H_

/* Allocate a new string that is the reverse of the given string. */
typedef char* (*TYPE_reverse)(const char*);
extern "C" char* Reverse(const char*);

#endif /* REVERSE_H_ */
