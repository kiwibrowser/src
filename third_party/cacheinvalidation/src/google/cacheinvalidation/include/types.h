// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Types used by the invalidation client library and its applications.

#ifndef GOOGLE_CACHEINVALIDATION_INCLUDE_TYPES_H_
#define GOOGLE_CACHEINVALIDATION_INCLUDE_TYPES_H_

#include <stdint.h>

#include <string>

#include "google/cacheinvalidation/deps/logging.h"
#include "google/cacheinvalidation/deps/stl-namespace.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::string;

/* Represents an opaque handle that can be used to acknowledge an invalidation
 * event by calling InvalidationClient::Acknowledge(AckHandle) to indicate that
 * the client has successfully handled the event.
 */
class AckHandle {
 public:
  /* Creates a new ack handle from the serialized handle_data representation. */
  explicit AckHandle(const string& handle_data) : handle_data_(handle_data) {}

  const string& handle_data() const {
    return handle_data_;
  }

  bool operator==(const AckHandle& ack_handle) const {
    return handle_data() == ack_handle.handle_data();
  }

  bool IsNoOp() const {
    return handle_data_.empty();
  }

 private:
  /* The serialized representation of the handle. */
  string handle_data_;
};

/* An identifier for application clients in an application-defined way. I.e., a
 * client name in an application naming scheme. This is not interpreted by the
 * invalidation system - however, it is used opaquely to squelch invalidations
 * for the cient causing an update, e.g., if a client C whose app client id is
 * C.appClientId changes object X and the backend store informs the backend
 * invalidation sytsem that X was modified by X.appClientId, the invalidation to
 * C can then be squelched by the invalidation system.
 */
class ApplicationClientId {
 public:
  /* Creates an application id for the given client_Name. */
  explicit ApplicationClientId(const string& client_name)
      : client_name_(client_name) {}

  const string& client_name() const {
    return client_name_;
  }

  bool operator==(const ApplicationClientId& app_client_id) const {
    return client_name() == app_client_id.client_name();
  }

 private:
  string client_name_;
};

/* Possible reasons for error in InvalidationListener::InformError. The
 * application writer must NOT assume that this is complete list since error
 * codes may be added later. That is, for error codes that it cannot handle,
 * it should not necessarily just crash the code. It may want to present a
 * dialog box to the user (say). For each ErrorReason, the ErrorInfo object
 * has a context object. We describe the type and meaning of the context for
 * each enum value below.
 */
class ErrorReason {
 public:
  /* The provided authentication/authorization token is not valid for use. */
  static const int AUTH_FAILURE = 1;

  /* An unknown failure - more human-readable information is in the error
   * message.
   */
  static const int UNKNOWN_FAILURE = -1;
};

/* Extra information about the error - cast to appropriate subtype as specified
 * for the reason.
 */
class ErrorContext {
 public:
  virtual ~ErrorContext() {}
};

/* A context with numeric data. */
class NumberContext : public ErrorContext {
 public:
  explicit NumberContext(int number) : number_(number) {}

  virtual ~NumberContext() {}

  int number() {
    return number_;
  }

 private:
  int number_;
};

/* Information about an error given to the application. */
class ErrorInfo {
 public:
  /* Constructs an ErrorInfo object given the reason for the error, whether it
   * is transient or permanent, and a helpful message describing the error.
   */
  ErrorInfo(int error_reason, bool is_transient,
            const string& error_message, const ErrorContext& context)
      : error_reason_(error_reason),
        is_transient_(is_transient),
        error_message_(error_message),
        context_(context) {}

  int error_reason() const {
    return error_reason_;
  }

  bool is_transient() const {
    return is_transient_;
  }

  const string& error_message() const {
    return error_message_;
  }

  const ErrorContext& context() const {
    return context_;
  }

 private:
  /* The cause of the failure. */
  int error_reason_;

  /* Is the error transient or permanent. See discussion in Status::Code for
   * permanent and transient failure handling.
   */
  bool is_transient_;

  /* Human-readable description of the error. */
  string error_message_;

  /* Extra information about the error - cast to appropriate object as specified
   * for the reason.
   */
  ErrorContext context_;
};

/* A class to represent a unique object id that an application can register or
 * unregister for.
 */
class ObjectId {
 public:
  ObjectId() : is_initialized_(false) {}

  /* Creates an object id for the given source and name (the name is copied). */
  ObjectId(int source, const string& name)
      : is_initialized_(true), source_(source), name_(name) {}

  void Init(int source, const string& name) {
    is_initialized_ = true;
    source_ = source;
    name_ = name;
  }

  int source() const {
    CHECK(is_initialized_);
    return source_;
  }

  const string& name() const {
    CHECK(is_initialized_);
    return name_;
  }

  bool operator==(const ObjectId& object_id) const {
    CHECK(is_initialized_);
    CHECK(object_id.is_initialized_);
    return (source() == object_id.source()) && (name() == object_id.name());
  }

 private:
  /* Whether the object id has been initialized. */
  bool is_initialized_;

  /* The invalidation source type. */
  int source_;

  /* The name/unique id for the object. */
  string name_;
};

/* A class to represent an invalidation for a given object/version and an
 * optional payload.
 */
class Invalidation {
 public:
  Invalidation() : is_initialized_(false) {}

  /* Creates a restarted invalidation for the given object and version. */
  Invalidation(const ObjectId& object_id, int64_t version) {
    Init(object_id, version, true);
  }

  /* Creates an invalidation for the given object, version, and payload. */
  Invalidation(const ObjectId& object_id, int64_t version,
               const string& payload) {
    Init(object_id, version, payload, true);
  }

  /*
   * Creates an invalidation for the given object, version, payload,
   * and restarted flag.
   */
  Invalidation(const ObjectId& object_id, int64_t version, const string& payload,
               bool is_trickle_restart) {
    Init(object_id, version, payload, is_trickle_restart);
  }


  void Init(const ObjectId& object_id, int64_t version, bool is_trickle_restart) {
    Init(object_id, version, false, "", is_trickle_restart);
  }

  void Init(const ObjectId& object_id, int64_t version, const string& payload,
            bool is_trickle_restart) {
    Init(object_id, version, true, payload, is_trickle_restart);
  }

  const ObjectId& object_id() const {
    return object_id_;
  }

  int64_t version() const {
    return version_;
  }

  bool has_payload() const {
    return has_payload_;
  }

  const string& payload() const {
    return payload_;
  }

  // This method is for internal use only.
  bool is_trickle_restart_for_internal_use() const {
    return is_trickle_restart_;
  }

  bool operator==(const Invalidation& invalidation) const {
    return (object_id() == invalidation.object_id()) &&
        (version() == invalidation.version()) &&
        (is_trickle_restart_for_internal_use() ==
            invalidation.is_trickle_restart_for_internal_use()) &&
        (has_payload() == invalidation.has_payload()) &&
        (payload() == invalidation.payload());
  }

 private:
  void Init(const ObjectId& object_id, int64_t version, bool has_payload,
            const string& payload, bool is_trickle_restart) {
    is_initialized_ = true;
    object_id_.Init(object_id.source(), object_id.name());
    version_ = version;
    has_payload_ = has_payload;
    payload_ = payload;
    is_trickle_restart_ = is_trickle_restart;
  }

  /* Whether this invalidation has been initialized. */
  bool is_initialized_;

  /* The object being invalidated/updated. */
  ObjectId object_id_;

  /* The new version of the object. */
  int64_t version_;

  /* Whether or not the invalidation includes a payload. */
  bool has_payload_;

  /* Optional payload for the client. */
  string payload_;

  /* Flag whether the trickle restarts at this invalidation. */
  bool is_trickle_restart_;
};

/* Information given to about a operation - success, temporary or permanent
 * failure.
 */
class Status {
 public:
  /* Actual status of the operation: Whether successful, transient or permanent
   * failure.
   */
  enum Code {
    /* Operation was successful. */
    SUCCESS,

    /* Operation had a transient failure. The application can retry the failed
     * operation later - if it chooses to do so, it must use a sensible backoff
     * policy such as exponential backoff.
     */
    TRANSIENT_FAILURE,

    /* Opration has a permanent failure. Application must not automatically
     * retry without fixing the situation (e.g., by presenting a dialog box to
     * the user).
     */
    PERMANENT_FAILURE
  };

  /* Creates a new Status object given the code and message. */
  Status(Code code, const string& message) : code_(code), message_(message) {}

  bool IsSuccess() const {
    return code_ == SUCCESS;
  }

  bool IsTransientFailure() const {
    return code_ == TRANSIENT_FAILURE;
  }

  bool IsPermanentFailure() const {
    return code_ == PERMANENT_FAILURE;
  }

  const string& message() const {
    return message_;
  }

  bool operator==(const Status& status) const {
    return (code_ == status.code_) && (message() == status.message());
  }

 private:
  /* Success or failure. */
  Code code_;

  /* A message describing why the state was unknown, for debugging. */
  string message_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_INCLUDE_TYPES_H_
