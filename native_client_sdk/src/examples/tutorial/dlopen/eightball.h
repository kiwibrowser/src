// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EIGHTBALL_H_
#define EIGHTBALL_H_

/* Return an answer. Question not required */
typedef char* (*TYPE_eightball)(void);
extern "C" const char* Magic8Ball();

#endif /* EIGHTBALL_H_ */
