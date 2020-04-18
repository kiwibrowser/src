// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#ifndef COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
#define COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
#include "cronet_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef const char* Cronet_String;
typedef void* Cronet_RawDataPtr;
typedef void* Cronet_ClientContext;

// Forward declare interfaces.
typedef struct Cronet_Buffer Cronet_Buffer;
typedef struct Cronet_Buffer* Cronet_BufferPtr;
typedef struct Cronet_BufferCallback Cronet_BufferCallback;
typedef struct Cronet_BufferCallback* Cronet_BufferCallbackPtr;
typedef struct Cronet_Runnable Cronet_Runnable;
typedef struct Cronet_Runnable* Cronet_RunnablePtr;
typedef struct Cronet_Executor Cronet_Executor;
typedef struct Cronet_Executor* Cronet_ExecutorPtr;
typedef struct Cronet_Engine Cronet_Engine;
typedef struct Cronet_Engine* Cronet_EnginePtr;
typedef struct Cronet_UrlRequestStatusListener Cronet_UrlRequestStatusListener;
typedef struct Cronet_UrlRequestStatusListener*
    Cronet_UrlRequestStatusListenerPtr;
typedef struct Cronet_UrlRequestCallback Cronet_UrlRequestCallback;
typedef struct Cronet_UrlRequestCallback* Cronet_UrlRequestCallbackPtr;
typedef struct Cronet_UploadDataSink Cronet_UploadDataSink;
typedef struct Cronet_UploadDataSink* Cronet_UploadDataSinkPtr;
typedef struct Cronet_UploadDataProvider Cronet_UploadDataProvider;
typedef struct Cronet_UploadDataProvider* Cronet_UploadDataProviderPtr;
typedef struct Cronet_UrlRequest Cronet_UrlRequest;
typedef struct Cronet_UrlRequest* Cronet_UrlRequestPtr;

// Forward declare structs.
typedef struct Cronet_Error Cronet_Error;
typedef struct Cronet_Error* Cronet_ErrorPtr;
typedef struct Cronet_QuicHint Cronet_QuicHint;
typedef struct Cronet_QuicHint* Cronet_QuicHintPtr;
typedef struct Cronet_PublicKeyPins Cronet_PublicKeyPins;
typedef struct Cronet_PublicKeyPins* Cronet_PublicKeyPinsPtr;
typedef struct Cronet_EngineParams Cronet_EngineParams;
typedef struct Cronet_EngineParams* Cronet_EngineParamsPtr;
typedef struct Cronet_HttpHeader Cronet_HttpHeader;
typedef struct Cronet_HttpHeader* Cronet_HttpHeaderPtr;
typedef struct Cronet_UrlResponseInfo Cronet_UrlResponseInfo;
typedef struct Cronet_UrlResponseInfo* Cronet_UrlResponseInfoPtr;
typedef struct Cronet_UrlRequestParams Cronet_UrlRequestParams;
typedef struct Cronet_UrlRequestParams* Cronet_UrlRequestParamsPtr;
typedef struct Cronet_RequestFinishedInfo Cronet_RequestFinishedInfo;
typedef struct Cronet_RequestFinishedInfo* Cronet_RequestFinishedInfoPtr;

// Declare enums
typedef enum Cronet_RESULT {
  Cronet_RESULT_SUCCESS = 0,
  Cronet_RESULT_ILLEGAL_ARGUMENT = -100,
  Cronet_RESULT_ILLEGAL_ARGUMENT_STORAGE_PATH_MUST_EXIST = -101,
  Cronet_RESULT_ILLEGAL_ARGUMENT_INVALID_PIN = -102,
  Cronet_RESULT_ILLEGAL_ARGUMENT_INVALID_HOSTNAME = -103,
  Cronet_RESULT_ILLEGAL_ARGUMENT_INVALID_HTTP_METHOD = -104,
  Cronet_RESULT_ILLEGAL_ARGUMENT_INVALID_HTTP_HEADER = -105,
  Cronet_RESULT_ILLEGAL_STATE = -200,
  Cronet_RESULT_ILLEGAL_STATE_STORAGE_PATH_IN_USE = -201,
  Cronet_RESULT_ILLEGAL_STATE_CANNOT_SHUTDOWN_ENGINE_FROM_NETWORK_THREAD = -202,
  Cronet_RESULT_ILLEGAL_STATE_ENGINE_ALREADY_STARTED = -203,
  Cronet_RESULT_ILLEGAL_STATE_REQUEST_ALREADY_STARTED = -204,
  Cronet_RESULT_ILLEGAL_STATE_REQUEST_NOT_INITIALIZED = -205,
  Cronet_RESULT_ILLEGAL_STATE_REQUEST_NOT_STARTED = -206,
  Cronet_RESULT_ILLEGAL_STATE_UNEXPECTED_REDIRECT = -207,
  Cronet_RESULT_ILLEGAL_STATE_UNEXPECTED_READ = -208,
  Cronet_RESULT_ILLEGAL_STATE_READ_FAILED = -209,
  Cronet_RESULT_NULL_POINTER = -300,
  Cronet_RESULT_NULL_POINTER_HOSTNAME = -301,
  Cronet_RESULT_NULL_POINTER_SHA256_PINS = -302,
  Cronet_RESULT_NULL_POINTER_EXPIRATION_DATE = -303,
  Cronet_RESULT_NULL_POINTER_ENGINE = -304,
  Cronet_RESULT_NULL_POINTER_URL = -305,
  Cronet_RESULT_NULL_POINTER_CALLBACK = -306,
  Cronet_RESULT_NULL_POINTER_EXECUTOR = -307,
  Cronet_RESULT_NULL_POINTER_METHOD = -308,
  Cronet_RESULT_NULL_POINTER_HEADER_NAME = -309,
  Cronet_RESULT_NULL_POINTER_HEADER_VALUE = -310,
  Cronet_RESULT_NULL_POINTER_PARAMS = -311,
} Cronet_RESULT;

typedef enum Cronet_Error_ERROR_CODE {
  Cronet_Error_ERROR_CODE_ERROR_CALLBACK = 0,
  Cronet_Error_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED = 1,
  Cronet_Error_ERROR_CODE_ERROR_INTERNET_DISCONNECTED = 2,
  Cronet_Error_ERROR_CODE_ERROR_NETWORK_CHANGED = 3,
  Cronet_Error_ERROR_CODE_ERROR_TIMED_OUT = 4,
  Cronet_Error_ERROR_CODE_ERROR_CONNECTION_CLOSED = 5,
  Cronet_Error_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT = 6,
  Cronet_Error_ERROR_CODE_ERROR_CONNECTION_REFUSED = 7,
  Cronet_Error_ERROR_CODE_ERROR_CONNECTION_RESET = 8,
  Cronet_Error_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE = 9,
  Cronet_Error_ERROR_CODE_ERROR_QUIC_PROTOCOL_FAILED = 10,
  Cronet_Error_ERROR_CODE_ERROR_OTHER = 11,
} Cronet_Error_ERROR_CODE;

typedef enum Cronet_EngineParams_HTTP_CACHE_MODE {
  Cronet_EngineParams_HTTP_CACHE_MODE_DISABLED = 0,
  Cronet_EngineParams_HTTP_CACHE_MODE_IN_MEMORY = 1,
  Cronet_EngineParams_HTTP_CACHE_MODE_DISK_NO_HTTP = 2,
  Cronet_EngineParams_HTTP_CACHE_MODE_DISK = 3,
} Cronet_EngineParams_HTTP_CACHE_MODE;

typedef enum Cronet_UrlRequestParams_REQUEST_PRIORITY {
  Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_IDLE = 0,
  Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOWEST = 1,
  Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOW = 2,
  Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM = 3,
  Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_HIGHEST = 4,
} Cronet_UrlRequestParams_REQUEST_PRIORITY;

typedef enum Cronet_RequestFinishedInfo_FINISHED_REASON {
  Cronet_RequestFinishedInfo_FINISHED_REASON_SUCCEEDED = 0,
  Cronet_RequestFinishedInfo_FINISHED_REASON_FAILED = 1,
  Cronet_RequestFinishedInfo_FINISHED_REASON_CANCELED = 2,
} Cronet_RequestFinishedInfo_FINISHED_REASON;

typedef enum Cronet_UrlRequestStatusListener_Status {
  Cronet_UrlRequestStatusListener_Status_INVALID = -1,
  Cronet_UrlRequestStatusListener_Status_IDLE = 0,
  Cronet_UrlRequestStatusListener_Status_WAITING_FOR_STALLED_SOCKET_POOL = 1,
  Cronet_UrlRequestStatusListener_Status_WAITING_FOR_AVAILABLE_SOCKET = 2,
  Cronet_UrlRequestStatusListener_Status_WAITING_FOR_DELEGATE = 3,
  Cronet_UrlRequestStatusListener_Status_WAITING_FOR_CACHE = 4,
  Cronet_UrlRequestStatusListener_Status_DOWNLOADING_PAC_FILE = 5,
  Cronet_UrlRequestStatusListener_Status_RESOLVING_PROXY_FOR_URL = 6,
  Cronet_UrlRequestStatusListener_Status_RESOLVING_HOST_IN_PAC_FILE = 7,
  Cronet_UrlRequestStatusListener_Status_ESTABLISHING_PROXY_TUNNEL = 8,
  Cronet_UrlRequestStatusListener_Status_RESOLVING_HOST = 9,
  Cronet_UrlRequestStatusListener_Status_CONNECTING = 10,
  Cronet_UrlRequestStatusListener_Status_SSL_HANDSHAKE = 11,
  Cronet_UrlRequestStatusListener_Status_SENDING_REQUEST = 12,
  Cronet_UrlRequestStatusListener_Status_WAITING_FOR_RESPONSE = 13,
  Cronet_UrlRequestStatusListener_Status_READING_RESPONSE = 14,
} Cronet_UrlRequestStatusListener_Status;

///////////////////////
// Concrete interface Cronet_Buffer.

// Create an instance of Cronet_Buffer.
CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_Create();
// Destroy an instance of Cronet_Buffer.
CRONET_EXPORT void Cronet_Buffer_Destroy(Cronet_BufferPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_Buffer_SetClientContext(
    Cronet_BufferPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_Buffer_GetClientContext(Cronet_BufferPtr self);
// Concrete methods of Cronet_Buffer implemented by Cronet.
// The app calls them to manipulate Cronet_Buffer.
CRONET_EXPORT
void Cronet_Buffer_InitWithDataAndCallback(Cronet_BufferPtr self,
                                           Cronet_RawDataPtr data,
                                           uint64_t size,
                                           Cronet_BufferCallbackPtr callback);
CRONET_EXPORT
void Cronet_Buffer_InitWithAlloc(Cronet_BufferPtr self, uint64_t size);
CRONET_EXPORT
uint64_t Cronet_Buffer_GetSize(Cronet_BufferPtr self);
CRONET_EXPORT
Cronet_RawDataPtr Cronet_Buffer_GetData(Cronet_BufferPtr self);
// Concrete interface Cronet_Buffer is implemented by Cronet.
// The app can implement these for testing / mocking.
typedef void (*Cronet_Buffer_InitWithDataAndCallbackFunc)(
    Cronet_BufferPtr self,
    Cronet_RawDataPtr data,
    uint64_t size,
    Cronet_BufferCallbackPtr callback);
typedef void (*Cronet_Buffer_InitWithAllocFunc)(Cronet_BufferPtr self,
                                                uint64_t size);
typedef uint64_t (*Cronet_Buffer_GetSizeFunc)(Cronet_BufferPtr self);
typedef Cronet_RawDataPtr (*Cronet_Buffer_GetDataFunc)(Cronet_BufferPtr self);
// Concrete interface Cronet_Buffer is implemented by Cronet.
// The app can use this for testing / mocking.
CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_CreateWith(
    Cronet_Buffer_InitWithDataAndCallbackFunc InitWithDataAndCallbackFunc,
    Cronet_Buffer_InitWithAllocFunc InitWithAllocFunc,
    Cronet_Buffer_GetSizeFunc GetSizeFunc,
    Cronet_Buffer_GetDataFunc GetDataFunc);

///////////////////////
// Abstract interface Cronet_BufferCallback is implemented by the app.

// There is no method to create a concrete implementation.

// Destroy an instance of Cronet_BufferCallback.
CRONET_EXPORT void Cronet_BufferCallback_Destroy(Cronet_BufferCallbackPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_BufferCallback_SetClientContext(
    Cronet_BufferCallbackPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_BufferCallback_GetClientContext(Cronet_BufferCallbackPtr self);
// Abstract interface Cronet_BufferCallback is implemented by the app.
// The following concrete methods forward call to app implementation.
// The app doesn't normally call them.
CRONET_EXPORT
void Cronet_BufferCallback_OnDestroy(Cronet_BufferCallbackPtr self,
                                     Cronet_BufferPtr buffer);
// The app implements abstract interface Cronet_BufferCallback by defining
// custom functions for each method.
typedef void (*Cronet_BufferCallback_OnDestroyFunc)(
    Cronet_BufferCallbackPtr self,
    Cronet_BufferPtr buffer);
// The app creates an instance of Cronet_BufferCallback by providing custom
// functions for each method.
CRONET_EXPORT Cronet_BufferCallbackPtr Cronet_BufferCallback_CreateWith(
    Cronet_BufferCallback_OnDestroyFunc OnDestroyFunc);

///////////////////////
// Abstract interface Cronet_Runnable is implemented by the app.

// There is no method to create a concrete implementation.

// Destroy an instance of Cronet_Runnable.
CRONET_EXPORT void Cronet_Runnable_Destroy(Cronet_RunnablePtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_Runnable_SetClientContext(
    Cronet_RunnablePtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_Runnable_GetClientContext(Cronet_RunnablePtr self);
// Abstract interface Cronet_Runnable is implemented by the app.
// The following concrete methods forward call to app implementation.
// The app doesn't normally call them.
CRONET_EXPORT
void Cronet_Runnable_Run(Cronet_RunnablePtr self);
// The app implements abstract interface Cronet_Runnable by defining custom
// functions for each method.
typedef void (*Cronet_Runnable_RunFunc)(Cronet_RunnablePtr self);
// The app creates an instance of Cronet_Runnable by providing custom functions
// for each method.
CRONET_EXPORT Cronet_RunnablePtr
Cronet_Runnable_CreateWith(Cronet_Runnable_RunFunc RunFunc);

///////////////////////
// Abstract interface Cronet_Executor is implemented by the app.

// There is no method to create a concrete implementation.

// Destroy an instance of Cronet_Executor.
CRONET_EXPORT void Cronet_Executor_Destroy(Cronet_ExecutorPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_Executor_SetClientContext(
    Cronet_ExecutorPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_Executor_GetClientContext(Cronet_ExecutorPtr self);
// Abstract interface Cronet_Executor is implemented by the app.
// The following concrete methods forward call to app implementation.
// The app doesn't normally call them.
CRONET_EXPORT
void Cronet_Executor_Execute(Cronet_ExecutorPtr self,
                             Cronet_RunnablePtr command);
// The app implements abstract interface Cronet_Executor by defining custom
// functions for each method.
typedef void (*Cronet_Executor_ExecuteFunc)(Cronet_ExecutorPtr self,
                                            Cronet_RunnablePtr command);
// The app creates an instance of Cronet_Executor by providing custom functions
// for each method.
CRONET_EXPORT Cronet_ExecutorPtr
Cronet_Executor_CreateWith(Cronet_Executor_ExecuteFunc ExecuteFunc);

///////////////////////
// Concrete interface Cronet_Engine.

// Create an instance of Cronet_Engine.
CRONET_EXPORT Cronet_EnginePtr Cronet_Engine_Create();
// Destroy an instance of Cronet_Engine.
CRONET_EXPORT void Cronet_Engine_Destroy(Cronet_EnginePtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_Engine_SetClientContext(
    Cronet_EnginePtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_Engine_GetClientContext(Cronet_EnginePtr self);
// Concrete methods of Cronet_Engine implemented by Cronet.
// The app calls them to manipulate Cronet_Engine.
CRONET_EXPORT
Cronet_RESULT Cronet_Engine_StartWithParams(Cronet_EnginePtr self,
                                            Cronet_EngineParamsPtr params);
CRONET_EXPORT
bool Cronet_Engine_StartNetLogToFile(Cronet_EnginePtr self,
                                     Cronet_String file_name,
                                     bool log_all);
CRONET_EXPORT
void Cronet_Engine_StopNetLog(Cronet_EnginePtr self);
CRONET_EXPORT
Cronet_RESULT Cronet_Engine_Shutdown(Cronet_EnginePtr self);
CRONET_EXPORT
Cronet_String Cronet_Engine_GetVersionString(Cronet_EnginePtr self);
CRONET_EXPORT
Cronet_String Cronet_Engine_GetDefaultUserAgent(Cronet_EnginePtr self);
// Concrete interface Cronet_Engine is implemented by Cronet.
// The app can implement these for testing / mocking.
typedef Cronet_RESULT (*Cronet_Engine_StartWithParamsFunc)(
    Cronet_EnginePtr self,
    Cronet_EngineParamsPtr params);
typedef bool (*Cronet_Engine_StartNetLogToFileFunc)(Cronet_EnginePtr self,
                                                    Cronet_String file_name,
                                                    bool log_all);
typedef void (*Cronet_Engine_StopNetLogFunc)(Cronet_EnginePtr self);
typedef Cronet_RESULT (*Cronet_Engine_ShutdownFunc)(Cronet_EnginePtr self);
typedef Cronet_String (*Cronet_Engine_GetVersionStringFunc)(
    Cronet_EnginePtr self);
typedef Cronet_String (*Cronet_Engine_GetDefaultUserAgentFunc)(
    Cronet_EnginePtr self);
// Concrete interface Cronet_Engine is implemented by Cronet.
// The app can use this for testing / mocking.
CRONET_EXPORT Cronet_EnginePtr Cronet_Engine_CreateWith(
    Cronet_Engine_StartWithParamsFunc StartWithParamsFunc,
    Cronet_Engine_StartNetLogToFileFunc StartNetLogToFileFunc,
    Cronet_Engine_StopNetLogFunc StopNetLogFunc,
    Cronet_Engine_ShutdownFunc ShutdownFunc,
    Cronet_Engine_GetVersionStringFunc GetVersionStringFunc,
    Cronet_Engine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc);

///////////////////////
// Abstract interface Cronet_UrlRequestStatusListener is implemented by the app.

// There is no method to create a concrete implementation.

// Destroy an instance of Cronet_UrlRequestStatusListener.
CRONET_EXPORT void Cronet_UrlRequestStatusListener_Destroy(
    Cronet_UrlRequestStatusListenerPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_UrlRequestStatusListener_SetClientContext(
    Cronet_UrlRequestStatusListenerPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_UrlRequestStatusListener_GetClientContext(
    Cronet_UrlRequestStatusListenerPtr self);
// Abstract interface Cronet_UrlRequestStatusListener is implemented by the app.
// The following concrete methods forward call to app implementation.
// The app doesn't normally call them.
CRONET_EXPORT
void Cronet_UrlRequestStatusListener_OnStatus(
    Cronet_UrlRequestStatusListenerPtr self,
    Cronet_UrlRequestStatusListener_Status status);
// The app implements abstract interface Cronet_UrlRequestStatusListener by
// defining custom functions for each method.
typedef void (*Cronet_UrlRequestStatusListener_OnStatusFunc)(
    Cronet_UrlRequestStatusListenerPtr self,
    Cronet_UrlRequestStatusListener_Status status);
// The app creates an instance of Cronet_UrlRequestStatusListener by providing
// custom functions for each method.
CRONET_EXPORT Cronet_UrlRequestStatusListenerPtr
Cronet_UrlRequestStatusListener_CreateWith(
    Cronet_UrlRequestStatusListener_OnStatusFunc OnStatusFunc);

///////////////////////
// Abstract interface Cronet_UrlRequestCallback is implemented by the app.

// There is no method to create a concrete implementation.

// Destroy an instance of Cronet_UrlRequestCallback.
CRONET_EXPORT void Cronet_UrlRequestCallback_Destroy(
    Cronet_UrlRequestCallbackPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_UrlRequestCallback_SetClientContext(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_UrlRequestCallback_GetClientContext(Cronet_UrlRequestCallbackPtr self);
// Abstract interface Cronet_UrlRequestCallback is implemented by the app.
// The following concrete methods forward call to app implementation.
// The app doesn't normally call them.
CRONET_EXPORT
void Cronet_UrlRequestCallback_OnRedirectReceived(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_String new_location_url);
CRONET_EXPORT
void Cronet_UrlRequestCallback_OnResponseStarted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);
CRONET_EXPORT
void Cronet_UrlRequestCallback_OnReadCompleted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_BufferPtr buffer,
    uint64_t bytes_read);
CRONET_EXPORT
void Cronet_UrlRequestCallback_OnSucceeded(Cronet_UrlRequestCallbackPtr self,
                                           Cronet_UrlRequestPtr request,
                                           Cronet_UrlResponseInfoPtr info);
CRONET_EXPORT
void Cronet_UrlRequestCallback_OnFailed(Cronet_UrlRequestCallbackPtr self,
                                        Cronet_UrlRequestPtr request,
                                        Cronet_UrlResponseInfoPtr info,
                                        Cronet_ErrorPtr error);
CRONET_EXPORT
void Cronet_UrlRequestCallback_OnCanceled(Cronet_UrlRequestCallbackPtr self,
                                          Cronet_UrlRequestPtr request,
                                          Cronet_UrlResponseInfoPtr info);
// The app implements abstract interface Cronet_UrlRequestCallback by defining
// custom functions for each method.
typedef void (*Cronet_UrlRequestCallback_OnRedirectReceivedFunc)(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_String new_location_url);
typedef void (*Cronet_UrlRequestCallback_OnResponseStartedFunc)(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);
typedef void (*Cronet_UrlRequestCallback_OnReadCompletedFunc)(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_BufferPtr buffer,
    uint64_t bytes_read);
typedef void (*Cronet_UrlRequestCallback_OnSucceededFunc)(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);
typedef void (*Cronet_UrlRequestCallback_OnFailedFunc)(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_ErrorPtr error);
typedef void (*Cronet_UrlRequestCallback_OnCanceledFunc)(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);
// The app creates an instance of Cronet_UrlRequestCallback by providing custom
// functions for each method.
CRONET_EXPORT Cronet_UrlRequestCallbackPtr Cronet_UrlRequestCallback_CreateWith(
    Cronet_UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
    Cronet_UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
    Cronet_UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
    Cronet_UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
    Cronet_UrlRequestCallback_OnFailedFunc OnFailedFunc,
    Cronet_UrlRequestCallback_OnCanceledFunc OnCanceledFunc);

///////////////////////
// Concrete interface Cronet_UploadDataSink.

// Create an instance of Cronet_UploadDataSink.
CRONET_EXPORT Cronet_UploadDataSinkPtr Cronet_UploadDataSink_Create();
// Destroy an instance of Cronet_UploadDataSink.
CRONET_EXPORT void Cronet_UploadDataSink_Destroy(Cronet_UploadDataSinkPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_UploadDataSink_SetClientContext(
    Cronet_UploadDataSinkPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_UploadDataSink_GetClientContext(Cronet_UploadDataSinkPtr self);
// Concrete methods of Cronet_UploadDataSink implemented by Cronet.
// The app calls them to manipulate Cronet_UploadDataSink.
CRONET_EXPORT
void Cronet_UploadDataSink_OnReadSucceeded(Cronet_UploadDataSinkPtr self,
                                           bool final_chunk);
CRONET_EXPORT
void Cronet_UploadDataSink_OnReadError(Cronet_UploadDataSinkPtr self,
                                       Cronet_ErrorPtr error);
CRONET_EXPORT
void Cronet_UploadDataSink_OnRewindSucceded(Cronet_UploadDataSinkPtr self);
CRONET_EXPORT
void Cronet_UploadDataSink_OnRewindError(Cronet_UploadDataSinkPtr self,
                                         Cronet_ErrorPtr error);
// Concrete interface Cronet_UploadDataSink is implemented by Cronet.
// The app can implement these for testing / mocking.
typedef void (*Cronet_UploadDataSink_OnReadSucceededFunc)(
    Cronet_UploadDataSinkPtr self,
    bool final_chunk);
typedef void (*Cronet_UploadDataSink_OnReadErrorFunc)(
    Cronet_UploadDataSinkPtr self,
    Cronet_ErrorPtr error);
typedef void (*Cronet_UploadDataSink_OnRewindSuccededFunc)(
    Cronet_UploadDataSinkPtr self);
typedef void (*Cronet_UploadDataSink_OnRewindErrorFunc)(
    Cronet_UploadDataSinkPtr self,
    Cronet_ErrorPtr error);
// Concrete interface Cronet_UploadDataSink is implemented by Cronet.
// The app can use this for testing / mocking.
CRONET_EXPORT Cronet_UploadDataSinkPtr Cronet_UploadDataSink_CreateWith(
    Cronet_UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
    Cronet_UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
    Cronet_UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
    Cronet_UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc);

///////////////////////
// Abstract interface Cronet_UploadDataProvider is implemented by the app.

// There is no method to create a concrete implementation.

// Destroy an instance of Cronet_UploadDataProvider.
CRONET_EXPORT void Cronet_UploadDataProvider_Destroy(
    Cronet_UploadDataProviderPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_UploadDataProvider_SetClientContext(
    Cronet_UploadDataProviderPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_UploadDataProvider_GetClientContext(Cronet_UploadDataProviderPtr self);
// Abstract interface Cronet_UploadDataProvider is implemented by the app.
// The following concrete methods forward call to app implementation.
// The app doesn't normally call them.
CRONET_EXPORT
int64_t Cronet_UploadDataProvider_GetLength(Cronet_UploadDataProviderPtr self);
CRONET_EXPORT
void Cronet_UploadDataProvider_Read(Cronet_UploadDataProviderPtr self,
                                    Cronet_UploadDataSinkPtr upload_data_sink,
                                    Cronet_BufferPtr buffer);
CRONET_EXPORT
void Cronet_UploadDataProvider_Rewind(
    Cronet_UploadDataProviderPtr self,
    Cronet_UploadDataSinkPtr upload_data_sink);
CRONET_EXPORT
void Cronet_UploadDataProvider_Close(Cronet_UploadDataProviderPtr self);
// The app implements abstract interface Cronet_UploadDataProvider by defining
// custom functions for each method.
typedef int64_t (*Cronet_UploadDataProvider_GetLengthFunc)(
    Cronet_UploadDataProviderPtr self);
typedef void (*Cronet_UploadDataProvider_ReadFunc)(
    Cronet_UploadDataProviderPtr self,
    Cronet_UploadDataSinkPtr upload_data_sink,
    Cronet_BufferPtr buffer);
typedef void (*Cronet_UploadDataProvider_RewindFunc)(
    Cronet_UploadDataProviderPtr self,
    Cronet_UploadDataSinkPtr upload_data_sink);
typedef void (*Cronet_UploadDataProvider_CloseFunc)(
    Cronet_UploadDataProviderPtr self);
// The app creates an instance of Cronet_UploadDataProvider by providing custom
// functions for each method.
CRONET_EXPORT Cronet_UploadDataProviderPtr Cronet_UploadDataProvider_CreateWith(
    Cronet_UploadDataProvider_GetLengthFunc GetLengthFunc,
    Cronet_UploadDataProvider_ReadFunc ReadFunc,
    Cronet_UploadDataProvider_RewindFunc RewindFunc,
    Cronet_UploadDataProvider_CloseFunc CloseFunc);

///////////////////////
// Concrete interface Cronet_UrlRequest.

// Create an instance of Cronet_UrlRequest.
CRONET_EXPORT Cronet_UrlRequestPtr Cronet_UrlRequest_Create();
// Destroy an instance of Cronet_UrlRequest.
CRONET_EXPORT void Cronet_UrlRequest_Destroy(Cronet_UrlRequestPtr self);
// Set and get app-specific Cronet_ClientContext.
CRONET_EXPORT void Cronet_UrlRequest_SetClientContext(
    Cronet_UrlRequestPtr self,
    Cronet_ClientContext client_context);
CRONET_EXPORT Cronet_ClientContext
Cronet_UrlRequest_GetClientContext(Cronet_UrlRequestPtr self);
// Concrete methods of Cronet_UrlRequest implemented by Cronet.
// The app calls them to manipulate Cronet_UrlRequest.
CRONET_EXPORT
Cronet_RESULT Cronet_UrlRequest_InitWithParams(
    Cronet_UrlRequestPtr self,
    Cronet_EnginePtr engine,
    Cronet_String url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback,
    Cronet_ExecutorPtr executor);
CRONET_EXPORT
Cronet_RESULT Cronet_UrlRequest_Start(Cronet_UrlRequestPtr self);
CRONET_EXPORT
Cronet_RESULT Cronet_UrlRequest_FollowRedirect(Cronet_UrlRequestPtr self);
CRONET_EXPORT
Cronet_RESULT Cronet_UrlRequest_Read(Cronet_UrlRequestPtr self,
                                     Cronet_BufferPtr buffer);
CRONET_EXPORT
void Cronet_UrlRequest_Cancel(Cronet_UrlRequestPtr self);
CRONET_EXPORT
bool Cronet_UrlRequest_IsDone(Cronet_UrlRequestPtr self);
CRONET_EXPORT
void Cronet_UrlRequest_GetStatus(Cronet_UrlRequestPtr self,
                                 Cronet_UrlRequestStatusListenerPtr listener);
// Concrete interface Cronet_UrlRequest is implemented by Cronet.
// The app can implement these for testing / mocking.
typedef Cronet_RESULT (*Cronet_UrlRequest_InitWithParamsFunc)(
    Cronet_UrlRequestPtr self,
    Cronet_EnginePtr engine,
    Cronet_String url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback,
    Cronet_ExecutorPtr executor);
typedef Cronet_RESULT (*Cronet_UrlRequest_StartFunc)(Cronet_UrlRequestPtr self);
typedef Cronet_RESULT (*Cronet_UrlRequest_FollowRedirectFunc)(
    Cronet_UrlRequestPtr self);
typedef Cronet_RESULT (*Cronet_UrlRequest_ReadFunc)(Cronet_UrlRequestPtr self,
                                                    Cronet_BufferPtr buffer);
typedef void (*Cronet_UrlRequest_CancelFunc)(Cronet_UrlRequestPtr self);
typedef bool (*Cronet_UrlRequest_IsDoneFunc)(Cronet_UrlRequestPtr self);
typedef void (*Cronet_UrlRequest_GetStatusFunc)(
    Cronet_UrlRequestPtr self,
    Cronet_UrlRequestStatusListenerPtr listener);
// Concrete interface Cronet_UrlRequest is implemented by Cronet.
// The app can use this for testing / mocking.
CRONET_EXPORT Cronet_UrlRequestPtr Cronet_UrlRequest_CreateWith(
    Cronet_UrlRequest_InitWithParamsFunc InitWithParamsFunc,
    Cronet_UrlRequest_StartFunc StartFunc,
    Cronet_UrlRequest_FollowRedirectFunc FollowRedirectFunc,
    Cronet_UrlRequest_ReadFunc ReadFunc,
    Cronet_UrlRequest_CancelFunc CancelFunc,
    Cronet_UrlRequest_IsDoneFunc IsDoneFunc,
    Cronet_UrlRequest_GetStatusFunc GetStatusFunc);

///////////////////////
// Struct Cronet_Error.
CRONET_EXPORT Cronet_ErrorPtr Cronet_Error_Create();
CRONET_EXPORT void Cronet_Error_Destroy(Cronet_ErrorPtr self);
// Cronet_Error setters.
CRONET_EXPORT
void Cronet_Error_error_code_set(Cronet_ErrorPtr self,
                                 Cronet_Error_ERROR_CODE error_code);
CRONET_EXPORT
void Cronet_Error_message_set(Cronet_ErrorPtr self, Cronet_String message);
CRONET_EXPORT
void Cronet_Error_internal_error_code_set(Cronet_ErrorPtr self,
                                          int32_t internal_error_code);
CRONET_EXPORT
void Cronet_Error_immediately_retryable_set(Cronet_ErrorPtr self,
                                            bool immediately_retryable);
CRONET_EXPORT
void Cronet_Error_quic_detailed_error_code_set(
    Cronet_ErrorPtr self,
    int32_t quic_detailed_error_code);
// Cronet_Error getters.
CRONET_EXPORT
Cronet_Error_ERROR_CODE Cronet_Error_error_code_get(Cronet_ErrorPtr self);
CRONET_EXPORT
Cronet_String Cronet_Error_message_get(Cronet_ErrorPtr self);
CRONET_EXPORT
int32_t Cronet_Error_internal_error_code_get(Cronet_ErrorPtr self);
CRONET_EXPORT
bool Cronet_Error_immediately_retryable_get(Cronet_ErrorPtr self);
CRONET_EXPORT
int32_t Cronet_Error_quic_detailed_error_code_get(Cronet_ErrorPtr self);

///////////////////////
// Struct Cronet_QuicHint.
CRONET_EXPORT Cronet_QuicHintPtr Cronet_QuicHint_Create();
CRONET_EXPORT void Cronet_QuicHint_Destroy(Cronet_QuicHintPtr self);
// Cronet_QuicHint setters.
CRONET_EXPORT
void Cronet_QuicHint_host_set(Cronet_QuicHintPtr self, Cronet_String host);
CRONET_EXPORT
void Cronet_QuicHint_port_set(Cronet_QuicHintPtr self, int32_t port);
CRONET_EXPORT
void Cronet_QuicHint_alternate_port_set(Cronet_QuicHintPtr self,
                                        int32_t alternate_port);
// Cronet_QuicHint getters.
CRONET_EXPORT
Cronet_String Cronet_QuicHint_host_get(Cronet_QuicHintPtr self);
CRONET_EXPORT
int32_t Cronet_QuicHint_port_get(Cronet_QuicHintPtr self);
CRONET_EXPORT
int32_t Cronet_QuicHint_alternate_port_get(Cronet_QuicHintPtr self);

///////////////////////
// Struct Cronet_PublicKeyPins.
CRONET_EXPORT Cronet_PublicKeyPinsPtr Cronet_PublicKeyPins_Create();
CRONET_EXPORT void Cronet_PublicKeyPins_Destroy(Cronet_PublicKeyPinsPtr self);
// Cronet_PublicKeyPins setters.
CRONET_EXPORT
void Cronet_PublicKeyPins_host_set(Cronet_PublicKeyPinsPtr self,
                                   Cronet_String host);
CRONET_EXPORT
void Cronet_PublicKeyPins_pins_sha256_add(Cronet_PublicKeyPinsPtr self,
                                          Cronet_String element);
CRONET_EXPORT
void Cronet_PublicKeyPins_include_subdomains_set(Cronet_PublicKeyPinsPtr self,
                                                 bool include_subdomains);
CRONET_EXPORT
void Cronet_PublicKeyPins_expiration_date_set(Cronet_PublicKeyPinsPtr self,
                                              int64_t expiration_date);
// Cronet_PublicKeyPins getters.
CRONET_EXPORT
Cronet_String Cronet_PublicKeyPins_host_get(Cronet_PublicKeyPinsPtr self);
CRONET_EXPORT
uint32_t Cronet_PublicKeyPins_pins_sha256_size(Cronet_PublicKeyPinsPtr self);
CRONET_EXPORT
Cronet_String Cronet_PublicKeyPins_pins_sha256_at(Cronet_PublicKeyPinsPtr self,
                                                  uint32_t index);
CRONET_EXPORT
void Cronet_PublicKeyPins_pins_sha256_clear(Cronet_PublicKeyPinsPtr self);
CRONET_EXPORT
bool Cronet_PublicKeyPins_include_subdomains_get(Cronet_PublicKeyPinsPtr self);
CRONET_EXPORT
int64_t Cronet_PublicKeyPins_expiration_date_get(Cronet_PublicKeyPinsPtr self);

///////////////////////
// Struct Cronet_EngineParams.
CRONET_EXPORT Cronet_EngineParamsPtr Cronet_EngineParams_Create();
CRONET_EXPORT void Cronet_EngineParams_Destroy(Cronet_EngineParamsPtr self);
// Cronet_EngineParams setters.
CRONET_EXPORT
void Cronet_EngineParams_enable_check_result_set(Cronet_EngineParamsPtr self,
                                                 bool enable_check_result);
CRONET_EXPORT
void Cronet_EngineParams_user_agent_set(Cronet_EngineParamsPtr self,
                                        Cronet_String user_agent);
CRONET_EXPORT
void Cronet_EngineParams_accept_language_set(Cronet_EngineParamsPtr self,
                                             Cronet_String accept_language);
CRONET_EXPORT
void Cronet_EngineParams_storage_path_set(Cronet_EngineParamsPtr self,
                                          Cronet_String storage_path);
CRONET_EXPORT
void Cronet_EngineParams_enable_quic_set(Cronet_EngineParamsPtr self,
                                         bool enable_quic);
CRONET_EXPORT
void Cronet_EngineParams_enable_http2_set(Cronet_EngineParamsPtr self,
                                          bool enable_http2);
CRONET_EXPORT
void Cronet_EngineParams_enable_brotli_set(Cronet_EngineParamsPtr self,
                                           bool enable_brotli);
CRONET_EXPORT
void Cronet_EngineParams_http_cache_mode_set(
    Cronet_EngineParamsPtr self,
    Cronet_EngineParams_HTTP_CACHE_MODE http_cache_mode);
CRONET_EXPORT
void Cronet_EngineParams_http_cache_max_size_set(Cronet_EngineParamsPtr self,
                                                 int64_t http_cache_max_size);
CRONET_EXPORT
void Cronet_EngineParams_quic_hints_add(Cronet_EngineParamsPtr self,
                                        Cronet_QuicHintPtr element);
CRONET_EXPORT
void Cronet_EngineParams_public_key_pins_add(Cronet_EngineParamsPtr self,
                                             Cronet_PublicKeyPinsPtr element);
CRONET_EXPORT
void Cronet_EngineParams_enable_public_key_pinning_bypass_for_local_trust_anchors_set(
    Cronet_EngineParamsPtr self,
    bool enable_public_key_pinning_bypass_for_local_trust_anchors);
CRONET_EXPORT
void Cronet_EngineParams_experimental_options_set(
    Cronet_EngineParamsPtr self,
    Cronet_String experimental_options);
// Cronet_EngineParams getters.
CRONET_EXPORT
bool Cronet_EngineParams_enable_check_result_get(Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_String Cronet_EngineParams_user_agent_get(Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_String Cronet_EngineParams_accept_language_get(
    Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_String Cronet_EngineParams_storage_path_get(Cronet_EngineParamsPtr self);
CRONET_EXPORT
bool Cronet_EngineParams_enable_quic_get(Cronet_EngineParamsPtr self);
CRONET_EXPORT
bool Cronet_EngineParams_enable_http2_get(Cronet_EngineParamsPtr self);
CRONET_EXPORT
bool Cronet_EngineParams_enable_brotli_get(Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_EngineParams_HTTP_CACHE_MODE Cronet_EngineParams_http_cache_mode_get(
    Cronet_EngineParamsPtr self);
CRONET_EXPORT
int64_t Cronet_EngineParams_http_cache_max_size_get(
    Cronet_EngineParamsPtr self);
CRONET_EXPORT
uint32_t Cronet_EngineParams_quic_hints_size(Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_QuicHintPtr Cronet_EngineParams_quic_hints_at(
    Cronet_EngineParamsPtr self,
    uint32_t index);
CRONET_EXPORT
void Cronet_EngineParams_quic_hints_clear(Cronet_EngineParamsPtr self);
CRONET_EXPORT
uint32_t Cronet_EngineParams_public_key_pins_size(Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_PublicKeyPinsPtr Cronet_EngineParams_public_key_pins_at(
    Cronet_EngineParamsPtr self,
    uint32_t index);
CRONET_EXPORT
void Cronet_EngineParams_public_key_pins_clear(Cronet_EngineParamsPtr self);
CRONET_EXPORT
bool Cronet_EngineParams_enable_public_key_pinning_bypass_for_local_trust_anchors_get(
    Cronet_EngineParamsPtr self);
CRONET_EXPORT
Cronet_String Cronet_EngineParams_experimental_options_get(
    Cronet_EngineParamsPtr self);

///////////////////////
// Struct Cronet_HttpHeader.
CRONET_EXPORT Cronet_HttpHeaderPtr Cronet_HttpHeader_Create();
CRONET_EXPORT void Cronet_HttpHeader_Destroy(Cronet_HttpHeaderPtr self);
// Cronet_HttpHeader setters.
CRONET_EXPORT
void Cronet_HttpHeader_name_set(Cronet_HttpHeaderPtr self, Cronet_String name);
CRONET_EXPORT
void Cronet_HttpHeader_value_set(Cronet_HttpHeaderPtr self,
                                 Cronet_String value);
// Cronet_HttpHeader getters.
CRONET_EXPORT
Cronet_String Cronet_HttpHeader_name_get(Cronet_HttpHeaderPtr self);
CRONET_EXPORT
Cronet_String Cronet_HttpHeader_value_get(Cronet_HttpHeaderPtr self);

///////////////////////
// Struct Cronet_UrlResponseInfo.
CRONET_EXPORT Cronet_UrlResponseInfoPtr Cronet_UrlResponseInfo_Create();
CRONET_EXPORT void Cronet_UrlResponseInfo_Destroy(
    Cronet_UrlResponseInfoPtr self);
// Cronet_UrlResponseInfo setters.
CRONET_EXPORT
void Cronet_UrlResponseInfo_url_set(Cronet_UrlResponseInfoPtr self,
                                    Cronet_String url);
CRONET_EXPORT
void Cronet_UrlResponseInfo_url_chain_add(Cronet_UrlResponseInfoPtr self,
                                          Cronet_String element);
CRONET_EXPORT
void Cronet_UrlResponseInfo_http_status_code_set(Cronet_UrlResponseInfoPtr self,
                                                 int32_t http_status_code);
CRONET_EXPORT
void Cronet_UrlResponseInfo_http_status_text_set(
    Cronet_UrlResponseInfoPtr self,
    Cronet_String http_status_text);
CRONET_EXPORT
void Cronet_UrlResponseInfo_all_headers_list_add(Cronet_UrlResponseInfoPtr self,
                                                 Cronet_HttpHeaderPtr element);
CRONET_EXPORT
void Cronet_UrlResponseInfo_was_cached_set(Cronet_UrlResponseInfoPtr self,
                                           bool was_cached);
CRONET_EXPORT
void Cronet_UrlResponseInfo_negotiated_protocol_set(
    Cronet_UrlResponseInfoPtr self,
    Cronet_String negotiated_protocol);
CRONET_EXPORT
void Cronet_UrlResponseInfo_proxy_server_set(Cronet_UrlResponseInfoPtr self,
                                             Cronet_String proxy_server);
CRONET_EXPORT
void Cronet_UrlResponseInfo_received_byte_count_set(
    Cronet_UrlResponseInfoPtr self,
    int64_t received_byte_count);
// Cronet_UrlResponseInfo getters.
CRONET_EXPORT
Cronet_String Cronet_UrlResponseInfo_url_get(Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
uint32_t Cronet_UrlResponseInfo_url_chain_size(Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
Cronet_String Cronet_UrlResponseInfo_url_chain_at(
    Cronet_UrlResponseInfoPtr self,
    uint32_t index);
CRONET_EXPORT
void Cronet_UrlResponseInfo_url_chain_clear(Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
int32_t Cronet_UrlResponseInfo_http_status_code_get(
    Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
Cronet_String Cronet_UrlResponseInfo_http_status_text_get(
    Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
uint32_t Cronet_UrlResponseInfo_all_headers_list_size(
    Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
Cronet_HttpHeaderPtr Cronet_UrlResponseInfo_all_headers_list_at(
    Cronet_UrlResponseInfoPtr self,
    uint32_t index);
CRONET_EXPORT
void Cronet_UrlResponseInfo_all_headers_list_clear(
    Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
bool Cronet_UrlResponseInfo_was_cached_get(Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
Cronet_String Cronet_UrlResponseInfo_negotiated_protocol_get(
    Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
Cronet_String Cronet_UrlResponseInfo_proxy_server_get(
    Cronet_UrlResponseInfoPtr self);
CRONET_EXPORT
int64_t Cronet_UrlResponseInfo_received_byte_count_get(
    Cronet_UrlResponseInfoPtr self);

///////////////////////
// Struct Cronet_UrlRequestParams.
CRONET_EXPORT Cronet_UrlRequestParamsPtr Cronet_UrlRequestParams_Create();
CRONET_EXPORT void Cronet_UrlRequestParams_Destroy(
    Cronet_UrlRequestParamsPtr self);
// Cronet_UrlRequestParams setters.
CRONET_EXPORT
void Cronet_UrlRequestParams_http_method_set(Cronet_UrlRequestParamsPtr self,
                                             Cronet_String http_method);
CRONET_EXPORT
void Cronet_UrlRequestParams_request_headers_add(
    Cronet_UrlRequestParamsPtr self,
    Cronet_HttpHeaderPtr element);
CRONET_EXPORT
void Cronet_UrlRequestParams_disable_cache_set(Cronet_UrlRequestParamsPtr self,
                                               bool disable_cache);
CRONET_EXPORT
void Cronet_UrlRequestParams_priority_set(
    Cronet_UrlRequestParamsPtr self,
    Cronet_UrlRequestParams_REQUEST_PRIORITY priority);
CRONET_EXPORT
void Cronet_UrlRequestParams_upload_data_provider_set(
    Cronet_UrlRequestParamsPtr self,
    Cronet_UploadDataProviderPtr upload_data_provider);
CRONET_EXPORT
void Cronet_UrlRequestParams_upload_data_provider_executor_set(
    Cronet_UrlRequestParamsPtr self,
    Cronet_ExecutorPtr upload_data_provider_executor);
CRONET_EXPORT
void Cronet_UrlRequestParams_allow_direct_executor_set(
    Cronet_UrlRequestParamsPtr self,
    bool allow_direct_executor);
CRONET_EXPORT
void Cronet_UrlRequestParams_annotations_add(Cronet_UrlRequestParamsPtr self,
                                             Cronet_RawDataPtr element);
// Cronet_UrlRequestParams getters.
CRONET_EXPORT
Cronet_String Cronet_UrlRequestParams_http_method_get(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
uint32_t Cronet_UrlRequestParams_request_headers_size(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
Cronet_HttpHeaderPtr Cronet_UrlRequestParams_request_headers_at(
    Cronet_UrlRequestParamsPtr self,
    uint32_t index);
CRONET_EXPORT
void Cronet_UrlRequestParams_request_headers_clear(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
bool Cronet_UrlRequestParams_disable_cache_get(Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
Cronet_UrlRequestParams_REQUEST_PRIORITY Cronet_UrlRequestParams_priority_get(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
Cronet_UploadDataProviderPtr Cronet_UrlRequestParams_upload_data_provider_get(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
Cronet_ExecutorPtr Cronet_UrlRequestParams_upload_data_provider_executor_get(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
bool Cronet_UrlRequestParams_allow_direct_executor_get(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
uint32_t Cronet_UrlRequestParams_annotations_size(
    Cronet_UrlRequestParamsPtr self);
CRONET_EXPORT
Cronet_RawDataPtr Cronet_UrlRequestParams_annotations_at(
    Cronet_UrlRequestParamsPtr self,
    uint32_t index);
CRONET_EXPORT
void Cronet_UrlRequestParams_annotations_clear(Cronet_UrlRequestParamsPtr self);

///////////////////////
// Struct Cronet_RequestFinishedInfo.
CRONET_EXPORT Cronet_RequestFinishedInfoPtr Cronet_RequestFinishedInfo_Create();
CRONET_EXPORT void Cronet_RequestFinishedInfo_Destroy(
    Cronet_RequestFinishedInfoPtr self);
// Cronet_RequestFinishedInfo setters.
// Cronet_RequestFinishedInfo getters.

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
