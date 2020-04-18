// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/base/xmpp_connection.h"

#include <stddef.h>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_task_runner_handle.h"
#include "jingle/glue/chrome_async_socket.h"
#include "jingle/glue/task_pump.h"
#include "jingle/glue/xmpp_client_socket_factory.h"
#include "jingle/notifier/base/weak_xmpp_client.h"
#include "net/socket/client_socket_factory.h"
#include "net/ssl/ssl_config_service.h"
#include "net/url_request/url_request_context.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclientsettings.h"

namespace notifier {

XmppConnection::Delegate::~Delegate() {}

namespace {

buzz::AsyncSocket* CreateSocket(
    const buzz::XmppClientSettings& xmpp_client_settings,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  bool use_fake_ssl_client_socket =
      (xmpp_client_settings.protocol() == cricket::PROTO_SSLTCP);
  // The default SSLConfig is good enough for us for now.
  const net::SSLConfig ssl_config;
  // These numbers were taken from similar numbers in
  // XmppSocketAdapter.
  const size_t kReadBufSize = 64U * 1024U;
  const size_t kWriteBufSize = 64U * 1024U;
  jingle_glue::XmppClientSocketFactory* const client_socket_factory =
      new jingle_glue::XmppClientSocketFactory(
          net::ClientSocketFactory::GetDefaultFactory(),
          ssl_config,
          request_context_getter,
          use_fake_ssl_client_socket);

  return new jingle_glue::ChromeAsyncSocket(client_socket_factory, kReadBufSize,
                                            kWriteBufSize, traffic_annotation);
}

}  // namespace

XmppConnection::XmppConnection(
    const buzz::XmppClientSettings& xmpp_client_settings,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    Delegate* delegate,
    buzz::PreXmppAuth* pre_xmpp_auth,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : task_pump_(new jingle_glue::TaskPump()),
      on_connect_called_(false),
      delegate_(delegate) {
  DCHECK(delegate_);
  // Owned by |task_pump_|, but is guaranteed to live at least as long
  // as this function.
  WeakXmppClient* weak_xmpp_client = new WeakXmppClient(task_pump_.get());
  weak_xmpp_client->SignalStateChange.connect(
      this, &XmppConnection::OnStateChange);
  weak_xmpp_client->SignalLogInput.connect(
      this, &XmppConnection::OnInputLog);
  weak_xmpp_client->SignalLogOutput.connect(
      this, &XmppConnection::OnOutputLog);
  const char kLanguage[] = "en";
  buzz::XmppReturnStatus connect_status = weak_xmpp_client->Connect(
      xmpp_client_settings, kLanguage,
      CreateSocket(xmpp_client_settings, request_context_getter,
                   traffic_annotation),
      pre_xmpp_auth);
  // buzz::XmppClient::Connect() should never fail.
  DCHECK_EQ(connect_status, buzz::XMPP_RETURN_OK);
  weak_xmpp_client->Start();
  weak_xmpp_client_ = weak_xmpp_client->AsWeakPtr();
}

XmppConnection::~XmppConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ClearClient();
  task_pump_->Stop();
  // We do this because XmppConnection may get destroyed as a result
  // of a signal from XmppClient.  If we delete |task_pump_| here, bad
  // things happen when the stack pops back up to the XmppClient's
  // (which is deleted by |task_pump_|) function.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  task_pump_.release());
}

void XmppConnection::OnStateChange(buzz::XmppEngine::State state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "XmppClient state changed to " << state;
  if (!weak_xmpp_client_.get()) {
    LOG(DFATAL) << "weak_xmpp_client_ unexpectedly NULL";
    return;
  }
  if (!delegate_) {
    LOG(DFATAL) << "delegate_ unexpectedly NULL";
    return;
  }
  switch (state) {
    case buzz::XmppEngine::STATE_OPEN:
      if (on_connect_called_) {
        LOG(DFATAL) << "State changed to STATE_OPEN more than once";
      } else {
        delegate_->OnConnect(weak_xmpp_client_);
        on_connect_called_ = true;
      }
      break;
    case buzz::XmppEngine::STATE_CLOSED: {
      int subcode = 0;
      buzz::XmppEngine::Error error =
          weak_xmpp_client_->GetError(&subcode);
      const buzz::XmlElement* stream_error =
          weak_xmpp_client_->GetStreamError();
      ClearClient();
      Delegate* delegate = delegate_;
      delegate_ = NULL;
      delegate->OnError(error, subcode, stream_error);
      break;
    }
    default:
      // Do nothing.
      break;
  }
}

void XmppConnection::OnInputLog(const char* data, int len) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << "XMPP Input: " << base::StringPiece(data, len);
}

void XmppConnection::OnOutputLog(const char* data, int len) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << "XMPP Output: " << base::StringPiece(data, len);
}

void XmppConnection::ClearClient() {
  if (weak_xmpp_client_.get()) {
    weak_xmpp_client_->Invalidate();
    DCHECK(!weak_xmpp_client_.get());
  }
}

}  // namespace notifier
