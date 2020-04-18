/**
 * \file errors.c
 * Mesa debugging and error handling functions.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "errors.h"

#include "imports.h"
#include "context.h"
#include "dispatch.h"
#include "hash.h"
#include "mtypes.h"
#include "version.h"


#define MAXSTRING MAX_DEBUG_MESSAGE_LENGTH


struct gl_client_severity
{
   struct simple_node link;
   GLuint ID;
};

static char out_of_memory[] = "Debugging error: out of memory";

#define enum_is(e, kind1, kind2) \
   ((e) == GL_DEBUG_##kind1##_##kind2##_ARB || (e) == GL_DONT_CARE)
#define severity_is(sev, kind) enum_is(sev, SEVERITY, kind)
#define source_is(s, kind) enum_is(s, SOURCE, kind)
#define type_is(t, kind) enum_is(t, TYPE, kind)

/* Prevent define collision on Windows */
#undef ERROR

enum {
   SOURCE_APPLICATION,
   SOURCE_THIRD_PARTY,

   SOURCE_COUNT,
   SOURCE_ANY = -1
};

enum {
   TYPE_ERROR,
   TYPE_DEPRECATED,
   TYPE_UNDEFINED,
   TYPE_PORTABILITY,
   TYPE_PERFORMANCE,
   TYPE_OTHER,

   TYPE_COUNT,
   TYPE_ANY = -1
};

enum {
   SEVERITY_LOW,
   SEVERITY_MEDIUM,
   SEVERITY_HIGH,

   SEVERITY_COUNT,
   SEVERITY_ANY = -1
};

static int
enum_to_index(GLenum e)
{
   switch (e) {
   case GL_DEBUG_SOURCE_APPLICATION_ARB:
      return (int)SOURCE_APPLICATION;
   case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
      return (int)SOURCE_THIRD_PARTY;

   case GL_DEBUG_TYPE_ERROR_ARB:
      return (int)TYPE_ERROR;
   case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
      return (int)TYPE_DEPRECATED;
   case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
      return (int)TYPE_UNDEFINED;
   case GL_DEBUG_TYPE_PERFORMANCE_ARB:
      return (int)TYPE_PERFORMANCE;
   case GL_DEBUG_TYPE_PORTABILITY_ARB:
      return (int)TYPE_PORTABILITY;
   case GL_DEBUG_TYPE_OTHER_ARB:
      return (int)TYPE_OTHER;

   case GL_DEBUG_SEVERITY_LOW_ARB:
      return (int)SEVERITY_LOW;
   case GL_DEBUG_SEVERITY_MEDIUM_ARB:
      return (int)SEVERITY_MEDIUM;
   case GL_DEBUG_SEVERITY_HIGH_ARB:
      return (int)SEVERITY_HIGH;

   case GL_DONT_CARE:
      return (int)TYPE_ANY;

   default:
      assert(0 && "unreachable");
      return -2;
   };
}


/*
 * We store a bitfield in the hash table, with five possible values total.
 *
 * The ENABLED_BIT's purpose is self-explanatory.
 *
 * The FOUND_BIT is needed to differentiate the value of DISABLED from
 * the value returned by HashTableLookup() when it can't find the given key.
 *
 * The KNOWN_SEVERITY bit is a bit complicated:
 *
 * A client may call Control() with an array of IDs, then call Control()
 * on all message IDs of a certain severity, then Insert() one of the
 * previously specified IDs, giving us a known severity level, then call
 * Control() on all message IDs of a certain severity level again.
 *
 * After the first call, those IDs will have a FOUND_BIT, but will not
 * exist in any severity-specific list, so the second call will not
 * impact them. This is undesirable but unavoidable given the API:
 * The only entrypoint that gives a severity for a client-defined ID
 * is the Insert() call.
 *
 * For the sake of Control(), we want to maintain the invariant
 * that an ID will either appear in none of the three severity lists,
 * or appear once, to minimize pointless duplication and potential surprises.
 *
 * Because Insert() is the only place that will learn an ID's severity,
 * it should insert an ID into the appropriate list, but only if the ID
 * doesn't exist in it or any other list yet. Because searching all three
 * lists at O(n) is needlessly expensive, we store KNOWN_SEVERITY.
 */
enum {
   FOUND_BIT = 1 << 0,
   ENABLED_BIT = 1 << 1,
   KNOWN_SEVERITY = 1 << 2,

   /* HashTable reserves zero as a return value meaning 'not found' */
   NOT_FOUND = 0,
   DISABLED = FOUND_BIT,
   ENABLED = ENABLED_BIT | FOUND_BIT
};

/**
 * Returns the state of the given message ID in a client-controlled
 * namespace.
 * 'source', 'type', and 'severity' are array indices like TYPE_ERROR,
 * not GL enums.
 */
static GLboolean
get_message_state(struct gl_context *ctx, int source, int type,
                  GLuint id, int severity)
{
   struct gl_client_namespace *nspace =
         &ctx->Debug.ClientIDs.Namespaces[source][type];
   uintptr_t state;

   /* In addition to not being able to store zero as a value, HashTable also
      can't use zero as a key. */
   if (id)
      state = (uintptr_t)_mesa_HashLookup(nspace->IDs, id);
   else
      state = nspace->ZeroID;

   /* Only do this once for each ID. This makes sure the ID exists in,
      at most, one list, and does not pointlessly appear multiple times. */
   if (!(state & KNOWN_SEVERITY)) {
      struct gl_client_severity *entry;

      if (state == NOT_FOUND) {
         if (ctx->Debug.ClientIDs.Defaults[severity][source][type])
            state = ENABLED;
         else
            state = DISABLED;
      }

      entry = malloc(sizeof *entry);
      if (!entry)
         goto out;

      state |= KNOWN_SEVERITY;

      if (id)
         _mesa_HashInsert(nspace->IDs, id, (void*)state);
      else
         nspace->ZeroID = state;

      entry->ID = id;
      insert_at_tail(&nspace->Severity[severity], &entry->link);
   }

out:
   return !!(state & ENABLED_BIT);
}

/**
 * Sets the state of the given message ID in a client-controlled
 * namespace.
 * 'source' and 'type' are array indices like TYPE_ERROR, not GL enums.
 */
static void
set_message_state(struct gl_context *ctx, int source, int type,
                  GLuint id, GLboolean enabled)
{
   struct gl_client_namespace *nspace =
         &ctx->Debug.ClientIDs.Namespaces[source][type];
   uintptr_t state;

   /* In addition to not being able to store zero as a value, HashTable also
      can't use zero as a key. */
   if (id)
      state = (uintptr_t)_mesa_HashLookup(nspace->IDs, id);
   else
      state = nspace->ZeroID;

   if (state == NOT_FOUND)
      state = enabled ? ENABLED : DISABLED;
   else {
      if (enabled)
         state |= ENABLED_BIT;
      else
         state &= ~ENABLED_BIT;
   }

   if (id)
      _mesa_HashInsert(nspace->IDs, id, (void*)state);
   else
      nspace->ZeroID = state;
}

/**
 * Whether a debugging message should be logged or not.
 * For implementation-controlled namespaces, we keep an array
 * of booleans per namespace, per context, recording whether
 * each individual message is enabled or not. The message ID
 * is an index into the namespace's array.
 */
static GLboolean
should_log(struct gl_context *ctx, GLenum source, GLenum type,
           GLuint id, GLenum severity)
{
   if (source == GL_DEBUG_SOURCE_APPLICATION_ARB ||
       source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB) {
      int s, t, sev;
      s = enum_to_index(source);
      t = enum_to_index(type);
      sev = enum_to_index(severity);

      return get_message_state(ctx, s, t, sev, id);
   }

   if (type_is(type, ERROR)) {
      if (source_is(source, API))
         return ctx->Debug.ApiErrors[id];
      if (source_is(source, WINDOW_SYSTEM))
         return ctx->Debug.WinsysErrors[id];
      if (source_is(source, SHADER_COMPILER))
         return ctx->Debug.ShaderErrors[id];
      if (source_is(source, OTHER))
         return ctx->Debug.OtherErrors[id];
   }

   return (severity != GL_DEBUG_SEVERITY_LOW_ARB);
}

/**
 * 'buf' is not necessarily a null-terminated string. When logging, copy
 * 'len' characters from it, store them in a new, null-terminated string,
 * and remember the number of bytes used by that string, *including*
 * the null terminator this time.
 */
static void
_mesa_log_msg(struct gl_context *ctx, GLenum source, GLenum type,
              GLuint id, GLenum severity, GLint len, const char *buf)
{
   GLint nextEmpty;
   struct gl_debug_msg *emptySlot;

   assert(len >= 0 && len < MAX_DEBUG_MESSAGE_LENGTH);

   if (!should_log(ctx, source, type, id, severity))
      return;

   if (ctx->Debug.Callback) {
      ctx->Debug.Callback(source, type, id, severity,
                          len, buf, ctx->Debug.CallbackData);
      return;
   }

   if (ctx->Debug.NumMessages == MAX_DEBUG_LOGGED_MESSAGES)
      return;

   nextEmpty = (ctx->Debug.NextMsg + ctx->Debug.NumMessages)
                          % MAX_DEBUG_LOGGED_MESSAGES;
   emptySlot = &ctx->Debug.Log[nextEmpty];

   assert(!emptySlot->message && !emptySlot->length);

   emptySlot->message = MALLOC(len+1);
   if (emptySlot->message) {
      (void) strncpy(emptySlot->message, buf, (size_t)len);
      emptySlot->message[len] = '\0';

      emptySlot->length = len+1;
      emptySlot->source = source;
      emptySlot->type = type;
      emptySlot->id = id;
      emptySlot->severity = severity;
   } else {
      /* malloc failed! */
      emptySlot->message = out_of_memory;
      emptySlot->length = strlen(out_of_memory)+1;
      emptySlot->source = GL_DEBUG_SOURCE_OTHER_ARB;
      emptySlot->type = GL_DEBUG_TYPE_ERROR_ARB;
      emptySlot->id = OTHER_ERROR_OUT_OF_MEMORY;
      emptySlot->severity = GL_DEBUG_SEVERITY_HIGH_ARB;
   }

   if (ctx->Debug.NumMessages == 0)
      ctx->Debug.NextMsgLength = ctx->Debug.Log[ctx->Debug.NextMsg].length;

   ctx->Debug.NumMessages++;
}

/**
 * Pop the oldest debug message out of the log.
 * Writes the message string, including the null terminator, into 'buf',
 * using up to 'bufSize' bytes. If 'bufSize' is too small, or
 * if 'buf' is NULL, nothing is written.
 *
 * Returns the number of bytes written on success, or when 'buf' is NULL,
 * the number that would have been written. A return value of 0
 * indicates failure.
 */
static GLsizei
_mesa_get_msg(struct gl_context *ctx, GLenum *source, GLenum *type,
              GLuint *id, GLenum *severity, GLsizei bufSize, char *buf)
{
   struct gl_debug_msg *msg;
   GLsizei length;

   if (ctx->Debug.NumMessages == 0)
      return 0;

   msg = &ctx->Debug.Log[ctx->Debug.NextMsg];
   length = msg->length;

   assert(length > 0 && length == ctx->Debug.NextMsgLength);

   if (bufSize < length && buf != NULL)
      return 0;

   if (severity)
      *severity = msg->severity;
   if (source)
      *source = msg->source;
   if (type)
      *type = msg->type;
   if (id)
      *id = msg->id;

   if (buf) {
      assert(msg->message[length-1] == '\0');
      (void) strncpy(buf, msg->message, (size_t)length);
   }

   if (msg->message != (char*)out_of_memory)
      FREE(msg->message);
   msg->message = NULL;
   msg->length = 0;

   ctx->Debug.NumMessages--;
   ctx->Debug.NextMsg++;
   ctx->Debug.NextMsg %= MAX_DEBUG_LOGGED_MESSAGES;
   ctx->Debug.NextMsgLength = ctx->Debug.Log[ctx->Debug.NextMsg].length;

   return length;
}

/**
 * Verify that source, type, and severity are valid enums.
 * glDebugMessageInsertARB only accepts two values for 'source',
 * and glDebugMessageControlARB will additionally accept GL_DONT_CARE
 * in any parameter, so handle those cases specially.
 */
static GLboolean
validate_params(struct gl_context *ctx, unsigned caller,
                GLenum source, GLenum type, GLenum severity)
{
#define INSERT 1
#define CONTROL 2
   switch(source) {
   case GL_DEBUG_SOURCE_APPLICATION_ARB:
   case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
      break;
   case GL_DEBUG_SOURCE_API_ARB:
   case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
   case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
   case GL_DEBUG_SOURCE_OTHER_ARB:
      if (caller != INSERT)
         break;
   case GL_DONT_CARE:
      if (caller == CONTROL)
         break;
   default:
      goto error;
   }

   switch(type) {
   case GL_DEBUG_TYPE_ERROR_ARB:
   case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
   case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
   case GL_DEBUG_TYPE_PERFORMANCE_ARB:
   case GL_DEBUG_TYPE_PORTABILITY_ARB:
   case GL_DEBUG_TYPE_OTHER_ARB:
      break;
   case GL_DONT_CARE:
      if (caller == CONTROL)
         break;
   default:
      goto error;
   }

   switch(severity) {
   case GL_DEBUG_SEVERITY_HIGH_ARB:
   case GL_DEBUG_SEVERITY_MEDIUM_ARB:
   case GL_DEBUG_SEVERITY_LOW_ARB:
      break;
   case GL_DONT_CARE:
      if (caller == CONTROL)
         break;
   default:
      goto error;
   }
   return GL_TRUE;

error:
   {
      const char *callerstr;
      if (caller == INSERT)
         callerstr = "glDebugMessageInsertARB";
      else if (caller == CONTROL)
         callerstr = "glDebugMessageControlARB";
      else
         return GL_FALSE;

      _mesa_error( ctx, GL_INVALID_ENUM, "bad values passed to %s"
                  "(source=0x%x, type=0x%x, severity=0x%x)", callerstr,
                  source, type, severity);
   }
   return GL_FALSE;
}

static void GLAPIENTRY
_mesa_DebugMessageInsertARB(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLint length,
                            const GLcharARB* buf)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!validate_params(ctx, INSERT, source, type, severity))
      return; /* GL_INVALID_ENUM */

   if (length < 0)
      length = strlen(buf);

   if (length >= MAX_DEBUG_MESSAGE_LENGTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDebugMessageInsertARB"
                 "(length=%d, which is not less than "
                 "GL_MAX_DEBUG_MESSAGE_LENGTH_ARB=%d)", length,
                 MAX_DEBUG_MESSAGE_LENGTH);
      return;
   }

   _mesa_log_msg(ctx, source, type, id, severity, length, buf);
}

static GLuint GLAPIENTRY
_mesa_GetDebugMessageLogARB(GLuint count, GLsizei logSize, GLenum* sources,
                            GLenum* types, GLenum* ids, GLenum* severities,
                            GLsizei* lengths, GLcharARB* messageLog)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint ret;

   if (!messageLog)
      logSize = 0;

   if (logSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetDebugMessageLogARB"
                 "(logSize=%d : logSize must not be negative)", logSize);
      return 0;
   }

   for (ret = 0; ret < count; ret++) {
      GLsizei written = _mesa_get_msg(ctx, sources, types, ids, severities,
                                      logSize, messageLog);
      if (!written)
         break;

      if (messageLog) {
         messageLog += written;
         logSize -= written;
      }
      if (lengths) {
         *lengths = written;
         lengths++;
      }

      if (severities)
         severities++;
      if (sources)
         sources++;
      if (types)
         types++;
      if (ids)
         ids++;
   }

   return ret;
}

/**
 * 'array' is an array representing a particular debugging-message namespace.
 * I.e., the set of all API errors, or the set of all Shader Compiler errors.
 * 'size' is the size of 'array'. 'count' is the size of 'ids', an array
 * of indices into 'array'. All the elements of 'array' at the indices
 * listed in 'ids' will be overwritten with the value of 'enabled'.
 *
 * If 'count' is zero, all elements in 'array' are overwritten with the
 * value of 'enabled'.
 */
static void
control_messages(GLboolean *array, GLuint size,
                 GLsizei count, const GLuint *ids, GLboolean enabled)
{
   GLsizei i;

   if (!count) {
      GLuint id;
      for (id = 0; id < size; id++) {
         array[id] = enabled;
      }
      return;
   }

   for (i = 0; i < count; i++) {
      if (ids[i] >= size) {
         /* XXX: The spec doesn't say what to do with a non-existent ID. */
         continue;
      }
      array[ids[i]] = enabled;
   }
}

/**
 * Set the state of all message IDs found in the given intersection
 * of 'source', 'type', and 'severity'. Note that all three of these
 * parameters are array indices, not the corresponding GL enums.
 *
 * This requires both setting the state of all previously seen message
 * IDs in the hash table, and setting the default state for all
 * applicable combinations of source/type/severity, so that all the
 * yet-unknown message IDs that may be used in the future will be
 * impacted as if they were already known.
 */
static void
control_app_messages_by_group(struct gl_context *ctx, int source, int type,
                              int severity, GLboolean enabled)
{
   struct gl_client_debug *ClientIDs = &ctx->Debug.ClientIDs;
   int s, t, sev, smax, tmax, sevmax;

   if (source == SOURCE_ANY) {
      source = 0;
      smax = SOURCE_COUNT;
   } else {
      smax = source+1;
   }

   if (type == TYPE_ANY) {
      type = 0;
      tmax = TYPE_COUNT;
   } else {
      tmax = type+1;
   }

   if (severity == SEVERITY_ANY) {
      severity = 0;
      sevmax = SEVERITY_COUNT;
   } else {
      sevmax = severity+1;
   }

   for (sev = severity; sev < sevmax; sev++)
      for (s = source; s < smax; s++)
         for (t = type; t < tmax; t++) {
            struct simple_node *node;
            struct gl_client_severity *entry;

            /* change the default for IDs we've never seen before. */
            ClientIDs->Defaults[sev][s][t] = enabled;

            /* Now change the state of IDs we *have* seen... */
            foreach(node, &ClientIDs->Namespaces[s][t].Severity[sev]) {
               entry = (struct gl_client_severity *)node;
               set_message_state(ctx, s, t, entry->ID, enabled);
            }
         }
}

/**
 * Debugging-message namespaces with the source APPLICATION or THIRD_PARTY
 * require special handling, since the IDs in them are controlled by clients,
 * not the OpenGL implementation.
 *
 * 'count' is the length of the array 'ids'. If 'count' is nonzero, all
 * the given IDs in the namespace defined by 'esource' and 'etype'
 * will be affected.
 *
 * If 'count' is zero, this sets the state of all IDs that match
 * the combination of 'esource', 'etype', and 'eseverity'.
 */
static void
control_app_messages(struct gl_context *ctx, GLenum esource, GLenum etype,
                     GLenum eseverity, GLsizei count, const GLuint *ids,
                     GLboolean enabled)
{
   int source, type, severity;
   GLsizei i;

   source = enum_to_index(esource);
   type = enum_to_index(etype);
   severity = enum_to_index(eseverity);

   if (count)
      assert(severity == SEVERITY_ANY && type != TYPE_ANY
             && source != SOURCE_ANY);

   for (i = 0; i < count; i++)
      set_message_state(ctx, source, type, ids[i], enabled);

   if (count)
      return;

   control_app_messages_by_group(ctx, source, type, severity, enabled);
}

static void GLAPIENTRY
_mesa_DebugMessageControlARB(GLenum source, GLenum type, GLenum severity,
                             GLsizei count, const GLuint *ids,
                             GLboolean enabled)
{
   GET_CURRENT_CONTEXT(ctx);

   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDebugMessageControlARB"
                 "(count=%d : count must not be negative)", count);
      return;
   }

   if (!validate_params(ctx, CONTROL, source, type, severity))
      return; /* GL_INVALID_ENUM */

   if (count && (severity != GL_DONT_CARE || type == GL_DONT_CARE
                 || source == GL_DONT_CARE)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDebugMessageControlARB"
                 "(When passing an array of ids, severity must be"
         " GL_DONT_CARE, and source and type must not be GL_DONT_CARE.");
      return;
   }

   if (source_is(source, APPLICATION) || source_is(source, THIRD_PARTY))
      control_app_messages(ctx, source, type, severity, count, ids, enabled);

   if (severity_is(severity, HIGH)) {
      if (type_is(type, ERROR)) {
         if (source_is(source, API))
            control_messages(ctx->Debug.ApiErrors, API_ERROR_COUNT,
                             count, ids, enabled);
         if (source_is(source, WINDOW_SYSTEM))
            control_messages(ctx->Debug.WinsysErrors, WINSYS_ERROR_COUNT,
                             count, ids, enabled);
         if (source_is(source, SHADER_COMPILER))
            control_messages(ctx->Debug.ShaderErrors, SHADER_ERROR_COUNT,
                             count, ids, enabled);
         if (source_is(source, OTHER))
            control_messages(ctx->Debug.OtherErrors, OTHER_ERROR_COUNT,
                             count, ids, enabled);
      }
   }
}

static void GLAPIENTRY
_mesa_DebugMessageCallbackARB(GLDEBUGPROCARB callback, const GLvoid *userParam)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Debug.Callback = callback;
   ctx->Debug.CallbackData = (void *) userParam;
}

void
_mesa_init_errors_dispatch(struct _glapi_table *disp)
{
   SET_DebugMessageCallbackARB(disp, _mesa_DebugMessageCallbackARB);
   SET_DebugMessageControlARB(disp, _mesa_DebugMessageControlARB);
   SET_DebugMessageInsertARB(disp, _mesa_DebugMessageInsertARB);
   SET_GetDebugMessageLogARB(disp, _mesa_GetDebugMessageLogARB);
}

void
_mesa_init_errors(struct gl_context *ctx)
{
   int s, t, sev;
   struct gl_client_debug *ClientIDs = &ctx->Debug.ClientIDs;

   ctx->Debug.Callback = NULL;
   ctx->Debug.SyncOutput = GL_FALSE;
   ctx->Debug.Log[0].length = 0;
   ctx->Debug.NumMessages = 0;
   ctx->Debug.NextMsg = 0;
   ctx->Debug.NextMsgLength = 0;

   /* Enable all the messages with severity HIGH or MEDIUM by default. */
   memset(ctx->Debug.ApiErrors, GL_TRUE, sizeof ctx->Debug.ApiErrors);
   memset(ctx->Debug.WinsysErrors, GL_TRUE, sizeof ctx->Debug.WinsysErrors);
   memset(ctx->Debug.ShaderErrors, GL_TRUE, sizeof ctx->Debug.ShaderErrors);
   memset(ctx->Debug.OtherErrors, GL_TRUE, sizeof ctx->Debug.OtherErrors);
   memset(ClientIDs->Defaults[SEVERITY_HIGH], GL_TRUE,
          sizeof ClientIDs->Defaults[SEVERITY_HIGH]);
   memset(ClientIDs->Defaults[SEVERITY_MEDIUM], GL_TRUE,
          sizeof ClientIDs->Defaults[SEVERITY_MEDIUM]);
   memset(ClientIDs->Defaults[SEVERITY_LOW], GL_FALSE,
          sizeof ClientIDs->Defaults[SEVERITY_LOW]);

   /* Initialize state for filtering client-provided debug messages. */
   for (s = 0; s < SOURCE_COUNT; s++)
      for (t = 0; t < TYPE_COUNT; t++) {
         ClientIDs->Namespaces[s][t].IDs = _mesa_NewHashTable();
         assert(ClientIDs->Namespaces[s][t].IDs);

         for (sev = 0; sev < SEVERITY_COUNT; sev++)
            make_empty_list(&ClientIDs->Namespaces[s][t].Severity[sev]);
      }
}

void
_mesa_free_errors_data(struct gl_context *ctx)
{
   int s, t, sev;
   struct gl_client_debug *ClientIDs = &ctx->Debug.ClientIDs;

   /* Tear down state for filtering client-provided debug messages. */
   for (s = 0; s < SOURCE_COUNT; s++)
      for (t = 0; t < TYPE_COUNT; t++) {
         _mesa_DeleteHashTable(ClientIDs->Namespaces[s][t].IDs);
         for (sev = 0; sev < SEVERITY_COUNT; sev++) {
            struct simple_node *node, *tmp;
            struct gl_client_severity *entry;

            foreach_s(node, tmp, &ClientIDs->Namespaces[s][t].Severity[sev]) {
               entry = (struct gl_client_severity *)node;
               FREE(entry);
            }
         }
      }
}

/**********************************************************************/
/** \name Diagnostics */
/*@{*/

static void
output_if_debug(const char *prefixString, const char *outputString,
                GLboolean newline)
{
   static int debug = -1;
   static FILE *fout = NULL;

   /* Init the local 'debug' var once.
    * Note: the _mesa_init_debug() function should have been called
    * by now so MESA_DEBUG_FLAGS will be initialized.
    */
   if (debug == -1) {
      /* If MESA_LOG_FILE env var is set, log Mesa errors, warnings,
       * etc to the named file.  Otherwise, output to stderr.
       */
      const char *logFile = _mesa_getenv("MESA_LOG_FILE");
      if (logFile)
         fout = fopen(logFile, "w");
      if (!fout)
         fout = stderr;
#ifdef DEBUG
      /* in debug builds, print messages unless MESA_DEBUG="silent" */
      if (MESA_DEBUG_FLAGS & DEBUG_SILENT)
         debug = 0;
      else
         debug = 1;
#else
      /* in release builds, be silent unless MESA_DEBUG is set */
      debug = _mesa_getenv("MESA_DEBUG") != NULL;
#endif
   }

   /* Now only print the string if we're required to do so. */
   if (debug) {
      fprintf(fout, "%s: %s", prefixString, outputString);
      if (newline)
         fprintf(fout, "\n");
      fflush(fout);

#if defined(_WIN32) && !defined(_WIN32_WCE)
      /* stderr from windows applications without console is not usually 
       * visible, so communicate with the debugger instead */ 
      {
         char buf[4096];
         _mesa_snprintf(buf, sizeof(buf), "%s: %s%s", prefixString, outputString, newline ? "\n" : "");
         OutputDebugStringA(buf);
      }
#endif
   }
}


/**
 * Return string version of GL error code.
 */
static const char *
error_string( GLenum error )
{
   switch (error) {
   case GL_NO_ERROR:
      return "GL_NO_ERROR";
   case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
   case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
   case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
   case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
   case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
   case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
   case GL_TABLE_TOO_LARGE:
      return "GL_TABLE_TOO_LARGE";
   case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
   default:
      return "unknown";
   }
}


/**
 * When a new type of error is recorded, print a message describing
 * previous errors which were accumulated.
 */
static void
flush_delayed_errors( struct gl_context *ctx )
{
   char s[MAXSTRING];

   if (ctx->ErrorDebugCount) {
      _mesa_snprintf(s, MAXSTRING, "%d similar %s errors", 
                     ctx->ErrorDebugCount,
                     error_string(ctx->ErrorValue));

      output_if_debug("Mesa", s, GL_TRUE);

      ctx->ErrorDebugCount = 0;
   }
}


/**
 * Report a warning (a recoverable error condition) to stderr if
 * either DEBUG is defined or the MESA_DEBUG env var is set.
 *
 * \param ctx GL context.
 * \param fmtString printf()-like format string.
 */
void
_mesa_warning( struct gl_context *ctx, const char *fmtString, ... )
{
   char str[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   (void) _mesa_vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );
   
   if (ctx)
      flush_delayed_errors( ctx );

   output_if_debug("Mesa warning", str, GL_TRUE);
}


/**
 * Report an internal implementation problem.
 * Prints the message to stderr via fprintf().
 *
 * \param ctx GL context.
 * \param fmtString problem description string.
 */
void
_mesa_problem( const struct gl_context *ctx, const char *fmtString, ... )
{
   va_list args;
   char str[MAXSTRING];
   static int numCalls = 0;

   (void) ctx;

   if (numCalls < 50) {
      numCalls++;

      va_start( args, fmtString );  
      _mesa_vsnprintf( str, MAXSTRING, fmtString, args );
      va_end( args );
      fprintf(stderr, "Mesa %s implementation error: %s\n",
              MESA_VERSION_STRING, str);
      fprintf(stderr, "Please report at bugs.freedesktop.org\n");
   }
}

static GLboolean
should_output(struct gl_context *ctx, GLenum error, const char *fmtString)
{
   static GLint debug = -1;

   /* Check debug environment variable only once:
    */
   if (debug == -1) {
      const char *debugEnv = _mesa_getenv("MESA_DEBUG");

#ifdef DEBUG
      if (debugEnv && strstr(debugEnv, "silent"))
         debug = GL_FALSE;
      else
         debug = GL_TRUE;
#else
      if (debugEnv)
         debug = GL_TRUE;
      else
         debug = GL_FALSE;
#endif
   }

   if (debug) {
      if (ctx->ErrorValue != error ||
          ctx->ErrorDebugFmtString != fmtString) {
         flush_delayed_errors( ctx );
         ctx->ErrorDebugFmtString = fmtString;
         ctx->ErrorDebugCount = 0;
         return GL_TRUE;
      }
      ctx->ErrorDebugCount++;
   }
   return GL_FALSE;
}


/**
 * Record an OpenGL state error.  These usually occur when the user
 * passes invalid parameters to a GL function.
 *
 * If debugging is enabled (either at compile-time via the DEBUG macro, or
 * run-time via the MESA_DEBUG environment variable), report the error with
 * _mesa_debug().
 * 
 * \param ctx the GL context.
 * \param error the error value.
 * \param fmtString printf() style format string, followed by optional args
 */
void
_mesa_error( struct gl_context *ctx, GLenum error, const char *fmtString, ... )
{
   GLboolean do_output, do_log;

   do_output = should_output(ctx, error, fmtString);
   do_log = should_log(ctx, GL_DEBUG_SOURCE_API_ARB, GL_DEBUG_TYPE_ERROR_ARB,
                       API_ERROR_UNKNOWN, GL_DEBUG_SEVERITY_HIGH_ARB);

   if (do_output || do_log) {
      char s[MAXSTRING], s2[MAXSTRING];
      int len;
      va_list args;

      va_start(args, fmtString);
      len = _mesa_vsnprintf(s, MAXSTRING, fmtString, args);
      va_end(args);

      if (len >= MAXSTRING) {
         /* Too long error message. Whoever calls _mesa_error should use
          * shorter strings. */
         ASSERT(0);
         return;
      }

      len = _mesa_snprintf(s2, MAXSTRING, "%s in %s", error_string(error), s);
      if (len >= MAXSTRING) {
         /* Same as above. */
         ASSERT(0);
         return;
      }

      /* Print the error to stderr if needed. */
      if (do_output) {
         output_if_debug("Mesa: User error", s2, GL_TRUE);
      }

      /* Log the error via ARB_debug_output if needed.*/
      if (do_log) {
         _mesa_log_msg(ctx, GL_DEBUG_SOURCE_API_ARB, GL_DEBUG_TYPE_ERROR_ARB,
                       API_ERROR_UNKNOWN, GL_DEBUG_SEVERITY_HIGH_ARB, len, s2);
      }
   }

   /* Set the GL context error state for glGetError. */
   _mesa_record_error(ctx, error);
}


/**
 * Report debug information.  Print error message to stderr via fprintf().
 * No-op if DEBUG mode not enabled.
 * 
 * \param ctx GL context.
 * \param fmtString printf()-style format string, followed by optional args.
 */
void
_mesa_debug( const struct gl_context *ctx, const char *fmtString, ... )
{
#ifdef DEBUG
   char s[MAXSTRING];
   va_list args;
   va_start(args, fmtString);
   _mesa_vsnprintf(s, MAXSTRING, fmtString, args);
   va_end(args);
   output_if_debug("Mesa", s, GL_FALSE);
#endif /* DEBUG */
   (void) ctx;
   (void) fmtString;
}


/**
 * Report debug information from the shader compiler via GL_ARB_debug_output.
 *
 * \param ctx GL context.
 * \param type The namespace to which this message belongs.
 * \param id The message ID within the given namespace.
 * \param msg The message to output. Need not be null-terminated.
 * \param len The length of 'msg'. If negative, 'msg' must be null-terminated.
 */
void
_mesa_shader_debug( struct gl_context *ctx, GLenum type, GLuint id,
                    const char *msg, int len )
{
   GLenum source = GL_DEBUG_SOURCE_SHADER_COMPILER_ARB,
          severity;

   switch (type) {
   case GL_DEBUG_TYPE_ERROR_ARB:
      assert(id < SHADER_ERROR_COUNT);
      severity = GL_DEBUG_SEVERITY_HIGH_ARB;
      break;
   case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
   case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
   case GL_DEBUG_TYPE_PORTABILITY_ARB:
   case GL_DEBUG_TYPE_PERFORMANCE_ARB:
   case GL_DEBUG_TYPE_OTHER_ARB:
      assert(0 && "other categories not implemented yet");
   default:
      _mesa_problem(ctx, "bad enum in _mesa_shader_debug()");
      return;
   }

   if (len < 0)
      len = strlen(msg);

   /* Truncate the message if necessary. */
   if (len >= MAX_DEBUG_MESSAGE_LENGTH)
      len = MAX_DEBUG_MESSAGE_LENGTH - 1;

   _mesa_log_msg(ctx, source, type, id, severity, len, msg);
}

/*@}*/
