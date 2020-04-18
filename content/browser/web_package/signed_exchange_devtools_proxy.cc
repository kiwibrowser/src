// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_devtools_proxy.h"

#include "base/trace_event/trace_event.h"
#include "content/browser/devtools/render_frame_devtools_agent_host.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/web_package/signed_exchange_header.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

namespace {

void AddErrorMessageToConsoleOnUI(
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    std::string error_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  WebContents* web_contents =
      WebContents::FromFrameTreeNodeId(frame_tree_node_id_getter.Run());
  if (!web_contents)
    return;
  web_contents->GetMainFrame()->AddMessageToConsole(
      content::CONSOLE_MESSAGE_LEVEL_ERROR, error_message);
}

void CertificateRequestSentOnUI(
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    const base::UnguessableToken& request_id,
    const base::UnguessableToken& loader_id,
    const network::ResourceRequest& request,
    const GURL& signed_exchange_url) {
  FrameTreeNode* frame_tree_node =
      FrameTreeNode::GloballyFindByID(frame_tree_node_id_getter.Run());
  if (!frame_tree_node)
    return;
  RenderFrameDevToolsAgentHost::OnSignedExchangeCertificateRequestSent(
      frame_tree_node, request_id, loader_id, request, signed_exchange_url);
}

void CertificateResponseReceivedOnUI(
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    const base::UnguessableToken& request_id,
    const base::UnguessableToken& loader_id,
    const GURL& url,
    scoped_refptr<network::ResourceResponse> response) {
  FrameTreeNode* frame_tree_node =
      FrameTreeNode::GloballyFindByID(frame_tree_node_id_getter.Run());
  if (!frame_tree_node)
    return;
  RenderFrameDevToolsAgentHost::OnSignedExchangeCertificateResponseReceived(
      frame_tree_node, request_id, loader_id, url, response->head);
}

void CertificateRequestCompletedOnUI(
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    const base::UnguessableToken& request_id,
    const network::URLLoaderCompletionStatus& status) {
  FrameTreeNode* frame_tree_node =
      FrameTreeNode::GloballyFindByID(frame_tree_node_id_getter.Run());
  if (!frame_tree_node)
    return;
  RenderFrameDevToolsAgentHost::OnSignedExchangeCertificateRequestCompleted(
      frame_tree_node, request_id, status);
}

void OnSignedExchangeReceivedOnUI(
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    const GURL& outer_request_url,
    scoped_refptr<network::ResourceResponse> outer_response,
    base::Optional<const base::UnguessableToken> devtools_navigation_token,
    base::Optional<SignedExchangeHeader> header,
    base::Optional<net::SSLInfo> ssl_info,
    std::vector<std::string> error_messages) {
  FrameTreeNode* frame_tree_node =
      FrameTreeNode::GloballyFindByID(frame_tree_node_id_getter.Run());
  if (!frame_tree_node)
    return;
  RenderFrameDevToolsAgentHost::OnSignedExchangeReceived(
      frame_tree_node, devtools_navigation_token, outer_request_url,
      outer_response->head, header, ssl_info, error_messages);
}

}  // namespace

SignedExchangeDevToolsProxy::SignedExchangeDevToolsProxy(
    const GURL& outer_request_url,
    const network::ResourceResponseHead& outer_response,
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    base::Optional<const base::UnguessableToken> devtools_navigation_token,
    bool report_raw_headers)
    : outer_request_url_(outer_request_url),
      outer_response_(outer_response),
      frame_tree_node_id_getter_(frame_tree_node_id_getter),
      devtools_navigation_token_(devtools_navigation_token),
      devtools_enabled_(report_raw_headers) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

SignedExchangeDevToolsProxy::~SignedExchangeDevToolsProxy() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void SignedExchangeDevToolsProxy::ReportErrorMessage(
    const std::string& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  error_messages_.push_back(message);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AddErrorMessageToConsoleOnUI, frame_tree_node_id_getter_,
                     std::move(message)));
}

void SignedExchangeDevToolsProxy::CertificateRequestSent(
    const base::UnguessableToken& request_id,
    const network::ResourceRequest& request) {
  if (!devtools_enabled_)
    return;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &CertificateRequestSentOnUI, frame_tree_node_id_getter_, request_id,
          devtools_navigation_token_ ? *devtools_navigation_token_ : request_id,
          request, outer_request_url_));
}

void SignedExchangeDevToolsProxy::CertificateResponseReceived(
    const base::UnguessableToken& request_id,
    const GURL& url,
    const network::ResourceResponseHead& head) {
  if (!devtools_enabled_)
    return;

  // Make a deep copy of ResourceResponseHead before passing it cross-thread.
  auto resource_response = base::MakeRefCounted<network::ResourceResponse>();
  resource_response->head = head;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &CertificateResponseReceivedOnUI, frame_tree_node_id_getter_,
          request_id,
          devtools_navigation_token_ ? *devtools_navigation_token_ : request_id,
          url, resource_response->DeepCopy()));
}

void SignedExchangeDevToolsProxy::CertificateRequestCompleted(
    const base::UnguessableToken& request_id,
    const network::URLLoaderCompletionStatus& status) {
  if (!devtools_enabled_)
    return;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&CertificateRequestCompletedOnUI,
                     frame_tree_node_id_getter_, request_id, status));
}

void SignedExchangeDevToolsProxy::OnSignedExchangeReceived(
    const base::Optional<SignedExchangeHeader>& header,
    const net::SSLInfo* ssl_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!devtools_enabled_)
    return;
  base::Optional<net::SSLInfo> ssl_info_opt;
  if (ssl_info)
    ssl_info_opt = *ssl_info;

  // Make a deep copy of ResourceResponseHead before passing it cross-thread.
  auto resource_response = base::MakeRefCounted<network::ResourceResponse>();
  resource_response->head = outer_response_;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OnSignedExchangeReceivedOnUI, frame_tree_node_id_getter_,
                     outer_request_url_, resource_response->DeepCopy(),
                     devtools_navigation_token_, header,
                     std::move(ssl_info_opt), std::move(error_messages_)));
}

}  // namespace content
