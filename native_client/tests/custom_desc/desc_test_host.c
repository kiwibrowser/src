/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/public/nacl_desc_custom.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/load_file.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


static struct NaClDesc *MakeExampleDesc(void);

static void *g_handle = (void *) 0x1234;
static int g_object_count = 0;
static struct NaClApp *g_nap;
static struct NaClDesc *g_expected_desc;

/*
 * In order to test RecvMsg() with multiple kinds of message, the test
 * guest will first do a SendMsg() to indicate which test message it
 * wants to receive.  We have some state to record this here.
 */
enum DescState {
  REPLY_UNEXPECTED,
  REPLY_WITH_DATA_ONLY,
  REPLY_WITH_DESCS
} g_desc_state = REPLY_UNEXPECTED;


static void ExampleDescDestroy(void *handle) {
  CHECK(handle == g_handle);
  CHECK(g_object_count > 0);
  g_object_count--;
}

static int MessageDataMatches(const struct NaClImcTypedMsgHdr *msg,
                              const char *string) {
  return (msg->iov_length == 1 &&
          msg->iov[0].length == strlen(string) &&
          memcmp(msg->iov[0].base, string, strlen(string)) == 0);
}

static ssize_t ExampleDescSendMsg(void *handle,
                                  const struct NaClImcTypedMsgHdr *msg,
                                  int flags) {
  int result_code = 100;
  CHECK(handle == g_handle);
  CHECK(msg->flags == 0);
  CHECK(flags == 0);

  if (MessageDataMatches(msg, "test_sending_data_only")) {
    CHECK(msg->ndesc_length == 0);
    /*
     * TODO(mseaborn): SendMsg() currently receives a pointer into
     * untrusted address space.  We might want to change things so
     * that SendMsg() receives a copy of the message data.
     */
    CHECK(msg->iov_length == 1);
    CHECK(NaClIsUserAddr(g_nap, (uintptr_t) msg->iov[0].base));
    result_code = 101;
  } else if (MessageDataMatches(msg, "test_sending_descs")) {
    uint32_t index;
    for (index = 0; index < msg->ndesc_length; index++) {
      CHECK(msg->ndescv[index] == g_expected_desc);
    }
    CHECK(msg->ndesc_length == 2);
    result_code = 102;
  } else if (MessageDataMatches(msg, "request_receiving_data_only")) {
    CHECK(msg->ndesc_length == 0);
    g_desc_state = REPLY_WITH_DATA_ONLY;
    result_code = 200;
  } else if (MessageDataMatches(msg, "request_receiving_descs")) {
    CHECK(msg->ndesc_length == 0);
    g_desc_state = REPLY_WITH_DESCS;
    result_code = 200;
  } else {
    NaClLog(LOG_FATAL, "Unexpected message\n");
  }
  /*
   * imc_sendmsg() is normally expected to return the number of bytes
   * that were sent, but the custom descriptor can assign whatever
   * meaning it wants to the return value.  Here, for testing, we use
   * the return value to indicate which message was matched.
   */
  return result_code;
}

static ssize_t WriteReplyData(struct NaClImcTypedMsgHdr *msg,
                              const char *data) {
  /*
   * A non-test implementation would truncate the data if the buffer
   * were too small, rather than producing a fatal error.
   */
  CHECK(msg->iov_length == 1);
  CHECK(msg->iov[0].length > strlen(data));
  memcpy(msg->iov[0].base, data, strlen(data));
  return strlen(data);
}

static ssize_t ExampleDescRecvMsg(void *handle,
                                  struct NaClImcTypedMsgHdr *msg,
                                  int flags) {
  ssize_t result = 0;
  uint32_t index;

  CHECK(handle == g_handle);
  CHECK(msg->flags == 0);
  CHECK(flags == 0);

  /*
   * ndesc_length tells us how large the buffer is.  *Currently*, the
   * syscall allocates the same size buffer regardless of the buffer
   * size that untrusted code provided.  Furthermore, the buffer is
   * filled with NULLs to guard against RecvMsg() forgetting to set
   * ndesc_length.  However, a RecvMsg() implementation should not
   * rely on either of these things.
   */
  CHECK(msg->ndesc_length == NACL_ABI_IMC_DESC_MAX);
  for (index = 0; index < msg->ndesc_length; index++) {
    CHECK(msg->ndescv[index] == NULL);
  }

  if (g_desc_state == REPLY_WITH_DATA_ONLY) {
    result = WriteReplyData(msg, "test_receiving_data_only");
    /*
     * TODO(mseaborn): RecvMsg() currently has to set ndesc_length to
     * indicate how many descriptors are in the message.  Since
     * ndesc_length is used as an input and output parameter, there is
     * no way for a caller of RecvMsg() to have an assertion to check
     * whether RecvMsg() has forgotten to set ndesc_length.  We might
     * want to change this so that there are separate input and output
     * parameters.
     */
    msg->ndesc_length = 0;
    /*
     * TODO(mseaborn): RecvMsg() currently receives a pointer into
     * untrusted address space.  We might want to change things so
     * that RecvMsg() does not write directly into untrusted address
     * space.
     */
    CHECK(msg->iov_length == 1);
    CHECK(NaClIsUserAddr(g_nap, (uintptr_t) msg->iov[0].base));
  } else if (g_desc_state == REPLY_WITH_DESCS) {
    result = WriteReplyData(msg, "test_receiving_descs");
    /*
     * A non-test implementation would truncate the descriptor array
     * if the buffer were too small, rather than producing a fatal
     * error.
     */
    CHECK(msg->ndesc_length >= 2);
    msg->ndescv[0] = MakeExampleDesc();
    msg->ndescv[1] = MakeExampleDesc();
    msg->ndesc_length = 2;
  } else {
    NaClLog(LOG_FATAL, "RecvMsg called while in an expected state\n");
  }
  g_desc_state = REPLY_UNEXPECTED;
  return result;
}

static struct NaClDesc *MakeExampleDesc(void) {
  struct NaClDescCustomFuncs funcs = NACL_DESC_CUSTOM_FUNCS_INITIALIZER;
  funcs.Destroy = ExampleDescDestroy;
  funcs.SendMsg = ExampleDescSendMsg;
  funcs.RecvMsg = ExampleDescRecvMsg;
  g_object_count++;
  return NaClDescMakeCustomDesc(g_handle, &funcs);
}


int main(int argc, char **argv) {
  struct NaClApp app;

  NaClHandleBootstrapArgs(&argc, &argv);

  if (argc != 2) {
    NaClLog(LOG_FATAL, "Expected 1 argument: executable filename\n");
  }

  NaClAllModulesInit();

  CHECK(NaClAppCtor(&app));
  CHECK(NaClAppLoadFileFromFilename(&app, argv[1]) == LOAD_OK);
  NaClAppInitialDescriptorHookup(&app);

  g_nap = &app;
  g_expected_desc = MakeExampleDesc();
  NaClAppSetDesc(&app, 10, g_expected_desc);

  CHECK(NaClCreateMainThread(&app, 0, NULL, NULL));
  CHECK(NaClWaitForMainThreadToExit(&app) == 0);

  /* Check for leaks. */
  CHECK(g_object_count == 0);

  /*
   * Avoid calling exit() because it runs process-global destructors
   * which might break code that is running in our unjoined threads.
   */
  NaClExit(0);
  return 0;
}
