/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unistd.h>

int main(const int argc, const char *argv[]) {
  char *eargv[] = {"/bin/echo", "/bin/rm" "-rf", "/home/*", NULL};
  int rc = execv(eargv[0], eargv);
}
