/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010 LunarG, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * Logging facility for debug/info messages.
 * _EGL_FATAL messages are printed to stderr
 * The EGL_LOG_LEVEL var controls the output of other warning/info/debug msgs.
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "egllog.h"
#include "eglstring.h"
#include "eglmutex.h"

#define MAXSTRING 1000
#define FALLBACK_LOG_LEVEL _EGL_WARNING


static struct {
   _EGLMutex mutex;

   EGLBoolean initialized;
   EGLint level;
   _EGLLogProc logger;
   EGLint num_messages;
} logging = {
   _EGL_MUTEX_INITIALIZER,
   EGL_FALSE,
   FALLBACK_LOG_LEVEL,
   NULL,
   0
};

static const char *level_strings[] = {
   /* the order is important */
   "fatal",
   "warning",
   "info",
   "debug",
   NULL
};


/**
 * Set the function to be called when there is a message to log.
 * Note that the function will be called with an internal lock held.
 * Recursive logging is not allowed.
 */
void
_eglSetLogProc(_EGLLogProc logger)
{
   EGLint num_messages = 0;

   _eglLockMutex(&logging.mutex);

   if (logging.logger != logger) {
      logging.logger = logger;

      num_messages = logging.num_messages;
      logging.num_messages = 0;
   }

   _eglUnlockMutex(&logging.mutex);

   if (num_messages)
      _eglLog(_EGL_DEBUG,
              "New logger installed. "
              "Messages before the new logger might not be available.");
}


/**
 * Set the log reporting level.
 */
void
_eglSetLogLevel(EGLint level)
{
   switch (level) {
   case _EGL_FATAL:
   case _EGL_WARNING:
   case _EGL_INFO:
   case _EGL_DEBUG:
      _eglLockMutex(&logging.mutex);
      logging.level = level;
      _eglUnlockMutex(&logging.mutex);
      break;
   default:
      break;
   }
}


/**
 * The default logger.  It prints the message to stderr.
 */
static void
_eglDefaultLogger(EGLint level, const char *msg)
{
   fprintf(stderr, "libEGL %s: %s\n", level_strings[level], msg);
}


/**
 * Initialize the logging facility.
 */
static void
_eglInitLogger(void)
{
   const char *log_env;
   EGLint i, level = -1;

   if (logging.initialized)
      return;

   log_env = getenv("EGL_LOG_LEVEL");
   if (log_env) {
      for (i = 0; level_strings[i]; i++) {
         if (_eglstrcasecmp(log_env, level_strings[i]) == 0) {
            level = i;
            break;
         }
      }
   }
   else {
      level = FALLBACK_LOG_LEVEL;
   }

   logging.logger = _eglDefaultLogger;
   logging.level = (level >= 0) ? level : FALLBACK_LOG_LEVEL;
   logging.initialized = EGL_TRUE;

   /* it is fine to call _eglLog now */
   if (log_env && level < 0) {
      _eglLog(_EGL_WARNING,
              "Unrecognized EGL_LOG_LEVEL environment variable value. "
              "Expected one of \"fatal\", \"warning\", \"info\", \"debug\". "
              "Got \"%s\". Falling back to \"%s\".",
              log_env, level_strings[FALLBACK_LOG_LEVEL]);
   }
}


/**
 * Log a message with message logger.
 * \param level one of _EGL_FATAL, _EGL_WARNING, _EGL_INFO, _EGL_DEBUG.
 */
void
_eglLog(EGLint level, const char *fmtStr, ...)
{
   va_list args;
   char msg[MAXSTRING];
   int ret;

   /* one-time initialization; a little race here is fine */
   if (!logging.initialized)
      _eglInitLogger();
   if (level > logging.level || level < 0)
      return;

   _eglLockMutex(&logging.mutex);

   if (logging.logger) {
      va_start(args, fmtStr);
      ret = vsnprintf(msg, MAXSTRING, fmtStr, args);
      if (ret < 0 || ret >= MAXSTRING)
         strcpy(msg, "<message truncated>");
      va_end(args);

      logging.logger(level, msg);
      logging.num_messages++;
   }

   _eglUnlockMutex(&logging.mutex);

   if (level == _EGL_FATAL)
      exit(1); /* or abort()? */
}
