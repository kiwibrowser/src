/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub empty environment for porting support.
 */

static char *__env[1] = { 0 };
char **environ = __env;
