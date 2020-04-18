/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/irt/irt_pnacl_translator_common.h"

static const char kInputVar[] = "NACL_IRT_PNACL_TRANSLATOR_LINK_INPUT";
static const char kOutputVar[] = "NACL_IRT_PNACL_TRANSLATOR_LINK_OUTPUT";

static void serve_link_request(int (*func)(int nexe_fd,
                                           const int *obj_file_fds,
                                           int obj_file_fd_count)) {
  const char *output_filename = getenv(kOutputVar);
  if (output_filename == NULL)
    NaClLog(LOG_FATAL, "serve_link_request: Env var %s not set\n", kOutputVar);

  /* Open output file. */
  int nexe_fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (nexe_fd < 0) {
    NaClLog(LOG_FATAL,
            "serve_link_request: Failed to open output file \"%s\": %s\n",
            output_filename, strerror(errno));
  }

  /* Open input files. */
  size_t inputs_count = env_var_prefix_match_count(kInputVar);
  int input_fds[inputs_count];
  char **env;
  size_t i = 0;
  for (env = environ; *env != NULL; env++) {
    char *input_filename = env_var_prefix_match(*env, kInputVar);
    if (input_filename != NULL) {
      int input_fd = open(input_filename, O_RDONLY);
      if (input_fd < 0) {
        NaClLog(LOG_FATAL,
                "serve_link_request: Failed to open input file \"%s\": %s\n",
                input_filename, strerror(errno));
      }
      assert(i < inputs_count);
      input_fds[i++] = input_fd;
    }
  }
  assert(i == inputs_count);

  func(nexe_fd, input_fds, inputs_count);
}

const struct nacl_irt_private_pnacl_translator_link
    nacl_irt_private_pnacl_translator_link = {
  serve_link_request,
};
