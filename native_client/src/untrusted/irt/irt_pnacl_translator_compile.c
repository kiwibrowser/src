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

static const char kInputVar[] = "NACL_IRT_PNACL_TRANSLATOR_COMPILE_INPUT";
static const char kOutputVar[] = "NACL_IRT_PNACL_TRANSLATOR_COMPILE_OUTPUT";
static const char kArgVar[] = "NACL_IRT_PNACL_TRANSLATOR_COMPILE_ARG";
static const char kThreadsVar[] = "NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS";

static void serve_translate_request(
    const struct nacl_irt_pnacl_compile_funcs *funcs) {
  const char *input_filename = getenv(kInputVar);
  const char *threads_str = getenv(kThreadsVar);
  if (input_filename == NULL) {
    NaClLog(LOG_FATAL, "serve_translate_request: Env var %s not set\n",
            kInputVar);
  }
  if (threads_str == NULL) {
    NaClLog(LOG_FATAL, "serve_translate_request: Env var %s not set\n",
            kThreadsVar);
  }

  /* Open input file. */
  int input_fd = open(input_filename, O_RDONLY);
  if (input_fd < 0) {
    NaClLog(LOG_FATAL,
            "serve_translate_request: Failed to open input file \"%s\": %s\n",
            input_filename, strerror(errno));
  }

  /* Open output files. */
  size_t outputs_count = env_var_prefix_match_count(kOutputVar);
  int output_fds[outputs_count];
  char **env;
  size_t i = 0;
  for (env = environ; *env != NULL; ++env) {
    char *output_filename = env_var_prefix_match(*env, kOutputVar);
    if (output_filename != NULL) {
      int output_fd = creat(output_filename, 0666);
      if (output_fd < 0) {
        NaClLog(LOG_FATAL,
                "serve_translate_request: "
                "Failed to open output file \"%s\": %s\n",
                output_filename, strerror(errno));
      }
      assert(i < outputs_count);
      output_fds[i++] = output_fd;
    }
  }
  assert(i == outputs_count);

  /* Extract list of arguments from the environment variables. */
  size_t args_count = env_var_prefix_match_count(kArgVar);
  char *args[args_count + 1];
  i = 0;
  for (env = environ; *env != NULL; ++env) {
    char *arg = env_var_prefix_match(*env, kArgVar);
    if (arg != NULL) {
      assert(i < args_count);
      args[i++] = arg;
    }
  }
  assert(i == args_count);
  args[i] = NULL;

  int thread_count = atoi(threads_str);
  funcs->init_callback(thread_count, output_fds, outputs_count,
                       args, args_count);
  char buf[0x1000];
  for (;;) {
    ssize_t bytes_read = read(input_fd, buf, sizeof(buf));
    if (bytes_read < 0) {
      NaClLog(LOG_FATAL,
              "serve_translate_request: "
              "Error while reading input file \"%s\": %s\n",
              input_filename, strerror(errno));
    }
    if (bytes_read == 0)
      break;
    funcs->data_callback(buf, bytes_read);
  }
  int rc = close(input_fd);
  if (rc != 0) {
    NaClLog(LOG_FATAL, "serve_translate_request: close() failed: %s\n",
            strerror(errno));
  }
  funcs->end_callback();
}

const struct nacl_irt_private_pnacl_translator_compile
    nacl_irt_private_pnacl_translator_compile = {
  serve_translate_request
};
