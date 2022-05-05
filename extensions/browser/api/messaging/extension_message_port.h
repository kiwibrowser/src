// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_MESSAGING_EXTENSION_MESSAGE_PORT_H_
#define EXTENSIONS_BROWSER_API_MESSAGING_EXTENSION_MESSAGE_PORT_H_

#include <set>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "extensions/browser/api/messaging/message_port.h"
#include "extensions/common/api/messaging/port_id.h"
#include "url/origin.h"

class GURL;

namespace content {
class BrowserContext;
class RenderFrameHost;
class RenderProcessHost;
}  // namespace content

namespace IPC {
class Message;
}  // namespace IPC

namespace extensions {
class ExtensionHost;

// A port that manages communication with an extension.
// The port's lifetime will end when either all receivers close the port, or
// when the opener / receiver explicitly closes the channel.
class ExtensionMessagePort : public MessagePort {
 public:
  // Create a port that is tied to frame(s) in a single tab.
  ExtensionMessagePort(base::WeakPtr<ChannelDelegate> channel_delegate,
                       const PortId& port_id,
                       const std::string& extension_id,
                       content::RenderFrameHost* rfh,
                       bool include_child_frames);
  // Create a port that is tied to all frames of an extension, possibly spanning
  // multiple tabs, including the invisible background page, popups, etc.
  ExtensionMessagePort(base::WeakPtr<ChannelDelegate> channel_delegate,
                       const PortId& port_id,
                       const std::string& extension_id,
                       content::RenderProcessHost* extension_process);
  ~ExtensionMessagePort() override;

  // Checks whether the frames to which this port is tied at its construction
  // are still aware of this port's existence. Frames that don't know about
  // the port are removed from the set of frames. This should be used for opener
  // ports because the frame may be navigated before the port was initialized.
  void RevalidatePort();

  // MessagePort:
  void RemoveCommonFrames(const MessagePort& port) override;
  bool HasFrame(content::RenderFrameHost* rfh) const override;
  bool IsValidPort() override;
  void DispatchOnConnect(const std::string& channel_name,
                         std::unique_ptr<base::DictionaryValue> source_tab,
                         int source_frame_id,
                         int guest_process_id,
                         int guest_render_frame_routing_id,
                         const std::string& source_extension_id,
                         const std::string& target_extension_id,
                         const GURL& source_url,
			 base::Optional<url::Origin> source_origin,
                         const std::string& tls_channel_id) override;
  void DispatchOnDisconnect(const std::string& error_message) override;
  void DispatchOnMessage(const Message& message) override;
  void IncrementLazyKeepaliveCount() override;
  void DecrementLazyKeepaliveCount() override;
  void OpenPort(int process_id, int routing_id) override;
  void ClosePort(int process_id, int routing_id) override;

 private:
  class FrameTracker;

  // Registers a frame as a receiver / sender.
  void RegisterFrame(content::RenderFrameHost* rfh);

  // Unregisters a frame as a receiver / sender. When there are no registered
  // frames any more, the port closes via CloseChannel().
  void UnregisterFrame(content::RenderFrameHost* rfh);

  // Immediately close the port and its associated channel.
  void CloseChannel();

  // Send a IPC message to the renderer for all registered frames.
  void SendToPort(std::unique_ptr<IPC::Message> msg);

  base::WeakPtr<ChannelDelegate> weak_channel_delegate_;

  const PortId port_id_;
  std::string extension_id_;
  content::BrowserContext* browser_context_;
  // Only for receivers in an extension process.
  content::RenderProcessHost* extension_process_;

  // When the port is used as a sender, this set contains only one element.
  // If used as a receiver, it may contain any number of frames.
  // This set is populated before the first message is sent to the destination,
  // and shrinks over time when the port is rejected by the recipient frame, or
  // when the frame is removed or unloaded.
  std::set<content::RenderFrameHost*> frames_;

  // Whether the renderer acknowledged creation of the port. This is used to
  // distinguish abnormal port closure (e.g. no receivers) from explicit port
  // closure (e.g. by the port.disconnect() JavaScript method in the renderer).
  bool did_create_port_;

  ExtensionHost* background_host_ptr_;  // used in IncrementLazyKeepaliveCount
  std::unique_ptr<FrameTracker> frame_tracker_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionMessagePort);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_MESSAGING_EXTENSION_MESSAGE_PORT_H_
