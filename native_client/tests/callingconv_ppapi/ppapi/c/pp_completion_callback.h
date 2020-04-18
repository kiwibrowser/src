/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From pp_completion_callback.idl modified Wed Oct  5 14:06:02 2011. */

#ifndef PPAPI_C_PP_COMPLETION_CALLBACK_H_
#define PPAPI_C_PP_COMPLETION_CALLBACK_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"

/**
 * @file
 * This file defines the API to create and run a callback.
 */


/**
 * @addtogroup Typedefs
 * @{
 */
/**
 * This typedef defines the signature that you implement to receive callbacks
 * on asynchronous completion of an operation.
 *
 * @param[in] user_data A pointer to user data passed to a callback function.
 * @param[in] result If result is 0 (PP_OK), the operation succeeded.  Negative
 * values (other than -1 or PP_OK_COMPLETE) indicate error and are specified
 * in pp_errors.h. Positive values for result usually indicate success and have
 * some operation-dependent meaning (such as bytes read).
 */
typedef void (*PP_CompletionCallback_Func)(void* user_data, int32_t result);
/**
 * @}
 */

/**
 * @addtogroup Enums
 * @{
 */
/**
 * This enumeration contains flags used to control how non-NULL callbacks are
 * scheduled by asynchronous methods.
 */
typedef enum {
  /**
   * By default any non-NULL callback will always invoked asynchronously,
   * on success or error, even if the operation could complete synchronously
   * without blocking.
   *
   * The method taking such callback will always return PP_OK_COMPLETIONPENDING.
   * The callback will be invoked on the main thread of PPAPI execution.
   */
  PP_COMPLETIONCALLBACK_FLAG_NONE = 0 << 0,
  /**
   * This flag allows any method taking such callback to complete synchronously
   * and not call the callback if the operation would not block. This is useful
   * when performance is an issue, and the operation bandwidth should not be
   * limited to the processing speed of the message loop.
   *
   * On synchronous method completion, the completion result will be returned
   * by the method itself. Otherwise, the method will return
   * PP_OK_COMPLETIONPENDING, and the callback will be invoked asynchronously on
   * the main thread of PPAPI execution.
   */
  PP_COMPLETIONCALLBACK_FLAG_OPTIONAL = 1 << 0
} PP_CompletionCallback_Flag;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_CompletionCallback_Flag, 4);
/**
 * @}
 */

/**
 * @addtogroup Structs
 * @{
 */
/**
 * Any method that takes a <code>PP_CompletionCallback</code> has the option of
 * completing asynchronously if the operation would block.  Such a method
 * should return <code>PP_OK_COMPLETIONPENDING</code> to indicate that the
 * method will complete asynchronously and notify the caller and will always be
 * invoked from the main thread of PPAPI execution.  If the completion callback
 * is NULL, then the operation will block if necessary to complete its work.
 * <code>PP_BlockUntilComplete()</code> provides a convenient way to specify
 * blocking behavior. Refer to <code>PP_BlockUntilComplete</code> for more
 * information.
 *
 * The result parameter passed to <code>func</code> is an int32_t that, if
 * negative indicates an error code whose meaning is specific to the calling
 * method (refer to <code>pp_error.h</code> for further information). A
 * positive or 0 value is a return result indicating success whose meaning
 * depends on the calling method (e.g. number of bytes read).
 */
struct PP_CompletionCallback {
  /**
   * This value is a callback function that will be called.
   */
  PP_CompletionCallback_Func func;
  /**
   * This value is a pointer to user data passed to a callback function.
   */
  void* user_data;
  /**
   * Flags used to control how non-NULL callbacks are scheduled by
   * asynchronous methods.
   */
  int32_t flags;
};
/**
 * @}
 */

#include <stdlib.h>

/**
 * @addtogroup Functions
 * @{
 */
/**
 * PP_MakeCompletionCallback() is used to create a
 * <code>PP_CompletionCallback</code>.
 *
 * <strong>Example:</strong>
 *
 * <code>
 * struct PP_CompletionCallback cc = PP_MakeCompletionCallback(Foo, NULL);
 * cc.flags = cc.flags | PP_COMPLETIONCALLBACK_FLAG_OPTIONAL;
 * </code>
 *
 * @param[in] func A <code>PP_CompletionCallback_Func</code> that will be
 * called.
 * @param[in] user_data A pointer to user data passed to your callback
 * function. This is optional and is typically used to help track state
 * when you may have multiple callbacks pending.
 *
 * @return A <code>PP_CompletionCallback</code> structure.
 */
PP_INLINE struct PP_CompletionCallback PP_MakeCompletionCallback(
    PP_CompletionCallback_Func func,
    void* user_data) {
  struct PP_CompletionCallback cc;
  cc.func = func;
  cc.user_data = user_data;
  cc.flags = PP_COMPLETIONCALLBACK_FLAG_NONE;
  return cc;
}

/**
 * PP_MakeOptionalCompletionCallback() is used to create a PP_CompletionCallback
 * with PP_COMPLETIONCALLBACK_FLAG_OPTIONAL set.
 *
 * @param[in] func A PP_CompletionCallback_Func to be called on completion.
 * @param[in] user_data A pointer to user data passed to be passed to the
 * callback function. This is optional and is typically used to help track state
 * in case of multiple pending callbacks.
 *
 * @return A PP_CompletionCallback structure.
 */
PP_INLINE struct PP_CompletionCallback PP_MakeOptionalCompletionCallback(
    PP_CompletionCallback_Func func,
    void* user_data) {
  struct PP_CompletionCallback cc = PP_MakeCompletionCallback(func, user_data);
  cc.flags = cc.flags | PP_COMPLETIONCALLBACK_FLAG_OPTIONAL;
  return cc;
}
/**
 * @}
 */

/**
 * @addtogroup Functions
 * @{
 */

/**
 * PP_RunCompletionCallback() is used to run a callback. It invokes
 * the callback function passing it user data specified on creation and
 * completion |result|.
 *
 * @param[in] cc A pointer to a <code>PP_CompletionCallback</code> that will be
 * run.
 * @param[in] result The result of the operation. Non-positive values correspond
 * to the error codes from pp_errors.h (excluding PP_OK_COMPLETIONPENDING).
 * Positive values indicate additional information such as bytes read.
 */
PP_INLINE void PP_RunCompletionCallback(struct PP_CompletionCallback* cc,
                                        int32_t result) {
  cc->func(cc->user_data, result);
}

/**
 * @}
 */

/**
 * @addtogroup Functions
 * @{
 */

 /**
 * PP_BlockUntilComplete() is used in place of an actual completion callback
 * to request blocking behavior. If specified, the calling thread will block
 * until the function completes. Blocking completion callbacks are only allowed
 * from background threads.
 *
 * @return A <code>PP_CompletionCallback</code> structure.
 */
PP_INLINE struct PP_CompletionCallback PP_BlockUntilComplete() {
  return PP_MakeCompletionCallback(NULL, NULL);
}

/**
 * PP_RunAndClearCompletionCallback() runs a callback and clears the reference
 * to that callback.
 *
 * This function is used when the null-ness of a completion callback is used as
 * a signal for whether a completion callback has been registered. In this
 * case, after the execution of the callback, it should be cleared. However,
 * this introduces a conflict if the completion callback wants to schedule more
 * work that involves the same completion callback again (for example, when
 * reading data from an URLLoader, one would typically queue up another read
 * callback). As a result, this function clears the pointer
 * before the provided callback is executed.
 */
PP_INLINE void PP_RunAndClearCompletionCallback(
    struct PP_CompletionCallback* cc,
    int32_t res) {
  struct PP_CompletionCallback temp = *cc;
  *cc = PP_BlockUntilComplete();
  PP_RunCompletionCallback(&temp, res);
}
/**
 * @}
 */

#endif  /* PPAPI_C_PP_COMPLETION_CALLBACK_H_ */

