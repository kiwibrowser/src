// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_NET_SERVER_CONNECTION_MANAGER_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_NET_SERVER_CONNECTION_MANAGER_H_

#include <stdint.h>

#include <iosfwd>
#include <memory>
#include <string>

#include "base/atomicops.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "components/sync/base/cancelation_observer.h"
#include "components/sync/syncable/syncable_id.h"

namespace syncer {

class CancelationSignal;

static const int32_t kUnsetResponseCode = -1;
static const int32_t kUnsetContentLength = -1;
static const int32_t kUnsetPayloadLength = -1;

// HttpResponse gathers the relevant output properties of an HTTP request.
// Depending on the value of the server_status code, response_code, and
// content_length may not be valid.
struct HttpResponse {
  enum ServerConnectionCode {
    // For uninitialized state.
    NONE,

    // CONNECTION_UNAVAILABLE is returned when InternetConnect() fails.
    CONNECTION_UNAVAILABLE,

    // IO_ERROR is returned when reading/writing to a buffer has failed.
    IO_ERROR,

    // SYNC_SERVER_ERROR is returned when the HTTP status code indicates that
    // a non-auth error has occured.
    SYNC_SERVER_ERROR,

    // SYNC_AUTH_ERROR is returned when the HTTP status code indicates that an
    // auth error has occured (i.e. a 401 or sync-specific AUTH_INVALID
    // response)
    // TODO(tim): Caring about AUTH_INVALID is a layering violation. But
    // this app-specific logic is being added as a stable branch hotfix so
    // minimal changes prevail for the moment.  Fix this! Bug 35060.
    SYNC_AUTH_ERROR,

    // SERVER_CONNECTION_OK is returned when request was handled correctly.
    SERVER_CONNECTION_OK,

    // RETRY is returned when a Commit request fails with a RETRY response from
    // the server.
    //
    // TODO(idana): the server no longer returns RETRY so we should remove this
    // value.
    RETRY,
  };

  // The HTTP Status code.
  int64_t response_code;

  // The value of the Content-length header.
  int64_t content_length;

  // The size of a download request's payload.
  int64_t payload_length;

  // Identifies the type of failure, if any.
  ServerConnectionCode server_status;

  HttpResponse();

  static const char* GetServerConnectionCodeString(ServerConnectionCode code);
};

struct ServerConnectionEvent {
  HttpResponse::ServerConnectionCode connection_code;
  explicit ServerConnectionEvent(HttpResponse::ServerConnectionCode code)
      : connection_code(code) {}
};

class ServerConnectionEventListener {
 public:
  virtual void OnServerConnectionEvent(const ServerConnectionEvent& event) = 0;

 protected:
  virtual ~ServerConnectionEventListener() {}
};

// Use this class to interact with the sync server.
// The ServerConnectionManager currently supports POSTing protocol buffers.
//
class ServerConnectionManager {
 public:
  // buffer_in - will be POSTed
  // buffer_out - string will be overwritten with response
  struct PostBufferParams {
    std::string buffer_in;
    std::string buffer_out;
    HttpResponse response;
  };

  // Abstract class providing network-layer functionality to the
  // ServerConnectionManager. Subclasses implement this using an HTTP stack of
  // their choice.
  class Connection {
   public:
    explicit Connection(ServerConnectionManager* scm);
    virtual ~Connection();

    // Called to initialize and perform an HTTP POST.
    virtual bool Init(const char* path,
                      const std::string& auth_token,
                      const std::string& payload,
                      HttpResponse* response) = 0;

    // Immediately abandons a pending HTTP POST request and unblocks caller
    // in Init.
    virtual void Abort() = 0;

    bool ReadBufferResponse(std::string* buffer_out,
                            HttpResponse* response,
                            bool require_response);
    bool ReadDownloadResponse(HttpResponse* response, std::string* buffer_out);

   protected:
    std::string MakeConnectionURL(const std::string& sync_server,
                                  const std::string& path,
                                  bool use_ssl) const;

    void GetServerParams(std::string* server,
                         int* server_port,
                         bool* use_ssl) const {
      server->assign(scm_->sync_server_);
      *server_port = scm_->sync_server_port_;
      *use_ssl = scm_->use_ssl_;
    }

    std::string buffer_;

   private:
    int ReadResponse(std::string* buffer, int length);

    ServerConnectionManager* scm_;
  };

  ServerConnectionManager(const std::string& server,
                          int port,
                          bool use_ssl,
                          CancelationSignal* cancelation_signal);

  virtual ~ServerConnectionManager();

  // POSTS buffer_in and reads a response into buffer_out. Uses our currently
  // set auth token in our headers.
  //
  // Returns true if executed successfully.
  virtual bool PostBufferWithCachedAuth(PostBufferParams* params);

  void AddListener(ServerConnectionEventListener* listener);
  void RemoveListener(ServerConnectionEventListener* listener);

  inline HttpResponse::ServerConnectionCode server_status() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return server_status_;
  }

  const std::string client_id() const { return client_id_; }

  // Factory method to create an Connection object we can use for
  // communication with the server.
  virtual std::unique_ptr<Connection> MakeConnection();

  void set_client_id(const std::string& client_id) {
    DCHECK(thread_checker_.CalledOnValidThread());
    DCHECK(client_id_.empty());
    client_id_.assign(client_id);
  }

  // Sets a new auth token. If |auth_token| is empty, the current token is
  // invalidated and cleared. Returns false if the server is in authentication
  // error state.
  bool SetAuthToken(const std::string& auth_token);

  bool HasInvalidAuthToken() { return auth_token_.empty(); }

  const std::string auth_token() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return auth_token_;
  }

 protected:
  inline std::string proto_sync_path() const { return proto_sync_path_; }

  // Updates server_status_ and notifies listeners if server_status_ changed
  void SetServerStatus(HttpResponse::ServerConnectionCode server_status);

  // NOTE: Tests rely on this protected function being virtual.
  //
  // Internal PostBuffer base function.
  virtual bool PostBufferToPath(PostBufferParams*,
                                const std::string& path,
                                const std::string& auth_token);

  // An internal helper to clear our auth_token_ and cache the old version
  // in |previously_invalidated_token_| to shelter us from retrying with a
  // known bad token.
  void InvalidateAndClearAuthToken();

  // Helper to check terminated flags and build a Connection object. If this
  // ServerConnectionManager has been terminated, this will return null.
  std::unique_ptr<Connection> MakeActiveConnection();

 private:
  void NotifyStatusChanged();

  // The sync_server_ is the server that requests will be made to.
  std::string sync_server_;

  // The sync_server_port_ is the port that HTTP requests will be made on.
  int sync_server_port_;

  // The unique id of the user's client.
  std::string client_id_;

  // Indicates whether or not requests should be made using HTTPS.
  bool use_ssl_;

  // The paths we post to.
  std::string proto_sync_path_;

  // The auth token to use in authenticated requests.
  std::string auth_token_;

  // The previous auth token that is invalid now.
  std::string previously_invalidated_token;

  base::ObserverList<ServerConnectionEventListener> listeners_;

  HttpResponse::ServerConnectionCode server_status_;

  base::ThreadChecker thread_checker_;

  CancelationSignal* const cancelation_signal_;

  DISALLOW_COPY_AND_ASSIGN(ServerConnectionManager);
};

std::ostream& operator<<(std::ostream& s, const struct HttpResponse& hr);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_NET_SERVER_CONNECTION_MANAGER_H_
