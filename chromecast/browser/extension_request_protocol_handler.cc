// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extension_request_protocol_handler.h"

#include "extensions/browser/extension_protocols.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/extension.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_manager.h"

namespace chromecast {

namespace {

class CastExtensionURLRequestJob : public net::URLRequestJob,
                                   public net::URLRequest::Delegate {
 public:
  CastExtensionURLRequestJob(net::URLRequest* request,
                             net::NetworkDelegate* network_delegate,
                             const GURL& redirect_url);

  ~CastExtensionURLRequestJob() override;

  // net::URLRequestJob implementation:
  void Start() override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  void SetRequestHeadersCallback(net::RequestHeadersCallback callback) override;
  void SetResponseHeadersCallback(
      net::ResponseHeadersCallback callback) override;
  bool GetMimeType(std::string* mime_type) const override;
  int GetResponseCode() const override;
  net::HostPortPair GetSocketAddress() const override;
  void StopCaching() override;
  bool GetFullRequestHeaders(net::HttpRequestHeaders* headers) const override;
  int64_t GetTotalReceivedBytes() const override;
  int64_t GetTotalSentBytes() const override;
  net::LoadState GetLoadState() const override;
  bool GetCharset(std::string* charset) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void GetLoadTimingInfo(net::LoadTimingInfo* load_timing_info) const override;
  bool GetRemoteEndpoint(net::IPEndPoint* endpoint) const override;
  void PopulateNetErrorDetails(net::NetErrorDetails* details) const override;
  bool IsRedirectResponse(GURL* location, int* http_status_code) override;
  bool CopyFragmentOnRedirect(const GURL& location) const override;
  bool IsSafeRedirect(const GURL& location) override;
  bool NeedsAuth() override;
  void GetAuthChallengeInfo(
      scoped_refptr<net::AuthChallengeInfo>* auth_info) override;
  void SetAuth(const net::AuthCredentials& credentials) override;
  void CancelAuth() override;
  void ContinueWithCertificate(
      scoped_refptr<net::X509Certificate> client_cert,
      scoped_refptr<net::SSLPrivateKey> client_private_key) override;
  void ContinueDespiteLastError() override;

  // net::URLRequest::Delegate implementation:
  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnAuthRequired(net::URLRequest* request,
                      net::AuthChallengeInfo* auth_info) override;
  void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) override;
  void OnSSLCertificateError(net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

 private:
  std::unique_ptr<net::URLRequest> sub_request_;
};

CastExtensionURLRequestJob::CastExtensionURLRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& redirect_url)
    : net::URLRequestJob(request, network_delegate),
      sub_request_(request->context()->CreateRequest(redirect_url,
                                                     request->priority(),
                                                     this)) {}

CastExtensionURLRequestJob::~CastExtensionURLRequestJob() {}

void CastExtensionURLRequestJob::Start() {
  sub_request_->Start();
}

bool CastExtensionURLRequestJob::IsRedirectResponse(GURL* location,
                                                    int* http_status_code) {
  return false;
}

void CastExtensionURLRequestJob::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  sub_request_->SetExtraRequestHeaders(headers);
}

int CastExtensionURLRequestJob::ReadRawData(net::IOBuffer* buf, int buf_size) {
  return sub_request_->Read(buf, buf_size);
}

void CastExtensionURLRequestJob::SetRequestHeadersCallback(
    net::RequestHeadersCallback callback) {
  sub_request_->SetRequestHeadersCallback(callback);
}

void CastExtensionURLRequestJob::SetResponseHeadersCallback(
    net::ResponseHeadersCallback callback) {
  sub_request_->SetResponseHeadersCallback(callback);
}

bool CastExtensionURLRequestJob::GetMimeType(std::string* mime_type) const {
  sub_request_->GetMimeType(mime_type);
  return true;
}

int CastExtensionURLRequestJob::GetResponseCode() const {
  return sub_request_->GetResponseCode();
}

net::HostPortPair CastExtensionURLRequestJob::GetSocketAddress() const {
  return sub_request_->GetSocketAddress();
}

void CastExtensionURLRequestJob::StopCaching() {
  sub_request_->StopCaching();
}

bool CastExtensionURLRequestJob::GetFullRequestHeaders(
    net::HttpRequestHeaders* headers) const {
  return sub_request_->GetFullRequestHeaders(headers);
}

int64_t CastExtensionURLRequestJob::GetTotalReceivedBytes() const {
  return sub_request_->GetTotalReceivedBytes();
}

int64_t CastExtensionURLRequestJob::GetTotalSentBytes() const {
  return sub_request_->GetTotalSentBytes();
}

net::LoadState CastExtensionURLRequestJob::GetLoadState() const {
  return sub_request_->GetLoadState().state;
}

bool CastExtensionURLRequestJob::GetCharset(std::string* charset) {
  sub_request_->GetCharset(charset);
  return true;
}

void CastExtensionURLRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  *info = sub_request_->response_info();
}

void CastExtensionURLRequestJob::GetLoadTimingInfo(
    net::LoadTimingInfo* load_timing_info) const {
  sub_request_->GetLoadTimingInfo(load_timing_info);
}

bool CastExtensionURLRequestJob::GetRemoteEndpoint(
    net::IPEndPoint* endpoint) const {
  return sub_request_->GetRemoteEndpoint(endpoint);
}

void CastExtensionURLRequestJob::PopulateNetErrorDetails(
    net::NetErrorDetails* details) const {
  sub_request_->PopulateNetErrorDetails(details);
}

bool CastExtensionURLRequestJob::CopyFragmentOnRedirect(
    const GURL& location) const {
  return false;
}

bool CastExtensionURLRequestJob::IsSafeRedirect(const GURL& location) {
  return true;
}

bool CastExtensionURLRequestJob::NeedsAuth() {
  return false;
}

void CastExtensionURLRequestJob::GetAuthChallengeInfo(
    scoped_refptr<net::AuthChallengeInfo>* auth_info) {}

void CastExtensionURLRequestJob::SetAuth(
    const net::AuthCredentials& credentials) {
  sub_request_->SetAuth(credentials);
}

void CastExtensionURLRequestJob::CancelAuth() {
  sub_request_->CancelAuth();
}

void CastExtensionURLRequestJob::ContinueWithCertificate(
    scoped_refptr<net::X509Certificate> client_cert,
    scoped_refptr<net::SSLPrivateKey> client_private_key) {
  sub_request_->ContinueWithCertificate(client_cert, client_private_key);
}

void CastExtensionURLRequestJob::ContinueDespiteLastError() {
  sub_request_->ContinueDespiteLastError();
}

void CastExtensionURLRequestJob::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  net::URLRequest::Delegate::OnReceivedRedirect(request, redirect_info,
                                                defer_redirect);
}

void CastExtensionURLRequestJob::OnAuthRequired(
    net::URLRequest* request,
    net::AuthChallengeInfo* auth_info) {
  net::URLRequest::Delegate::OnAuthRequired(request, auth_info);
}

void CastExtensionURLRequestJob::OnCertificateRequested(
    net::URLRequest* request,
    net::SSLCertRequestInfo* cert_request_info) {
  NotifyCertificateRequested(cert_request_info);
}

void CastExtensionURLRequestJob::OnSSLCertificateError(
    net::URLRequest* request,
    const net::SSLInfo& ssl_info,
    bool fatal) {
  NotifySSLCertificateError(ssl_info, fatal);
}

void CastExtensionURLRequestJob::OnResponseStarted(net::URLRequest* request,
                                                   int net_error) {
  net::URLRequestStatus status = net::URLRequestStatus::FromError(net_error);
  if (status.status() == net::URLRequestStatus::SUCCESS)
    NotifyHeadersComplete();
  else
    NotifyStartError(status);
}

void CastExtensionURLRequestJob::OnReadCompleted(net::URLRequest* request,
                                                 int bytes_read) {
  ReadRawDataComplete(bytes_read);
}

}  // namespace

ExtensionRequestProtocolHandler::ExtensionRequestProtocolHandler(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}

ExtensionRequestProtocolHandler::~ExtensionRequestProtocolHandler() {}

net::URLRequestJob* ExtensionRequestProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  if (!info_map_) {
    // This can't be done in the constructor as extensions::ExtensionSystem::Get
    // will fail until after this is constructed.
    info_map_ = extensions::ExtensionSystem::Get(browser_context_)->info_map();
    default_handler_ = extensions::CreateExtensionProtocolHandler(
        false, const_cast<extensions::InfoMap*>(info_map_));
  }

  const extensions::Extension* extension =
      info_map_->extensions().GetByID(request->url().host());

  if (!extension) {
    LOG(ERROR) << "Can't find extension with id: " << request->url().host();
    return nullptr;
  }

  std::string cast_url;
  if (!extension->manifest()->GetString("cast_url", &cast_url)) {
    // Defer to the default handler to load from disk.
    return default_handler_->MaybeCreateJob(request, network_delegate);
  }

  // Replace chrome-extension://<id> with whatever the extension wants to go to.
  cast_url.append(request->url().path());

  // Force a redirect to the new URL but without changing where the webpage
  // thinks it is.
  return new CastExtensionURLRequestJob(request, network_delegate,
                                        GURL(cast_url));
}

}  // namespace chromecast
