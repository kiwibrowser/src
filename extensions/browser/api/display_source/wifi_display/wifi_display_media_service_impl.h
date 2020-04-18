// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_MEDIA_SERVICE_IMPL_H_
#define EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_MEDIA_SERVICE_IMPL_H_

#include <memory>

#include "base/containers/queue.h"
#include "extensions/common/mojo/wifi_display_session_service.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/socket/udp_socket.h"

namespace content {
class RenderFrameHost;
}

namespace extensions {

class WiFiDisplayMediaServiceImpl : public mojom::WiFiDisplayMediaService {
 public:
  ~WiFiDisplayMediaServiceImpl() override;
  static void BindToRequest(mojom::WiFiDisplayMediaServiceRequest request,
                            content::RenderFrameHost* render_frame_host);

  void SetDesinationPoint(const std::string& ip_address,
                          int32_t port,
                          const SetDesinationPointCallback& callback) override;
  void SendMediaPacket(mojom::WiFiDisplayMediaPacketPtr packet) override;

 private:
  static void Create(mojom::WiFiDisplayMediaServiceRequest request);
  WiFiDisplayMediaServiceImpl();
  void Send();
  void OnSent(int code);
  std::unique_ptr<net::UDPSocket> rtp_socket_;
  class PacketIOBuffer;
  base::queue<scoped_refptr<PacketIOBuffer>> write_buffers_;
  int last_send_code_;
  mojo::StrongBindingPtr<mojom::WiFiDisplayMediaService> binding_;
  base::WeakPtrFactory<WiFiDisplayMediaServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WiFiDisplayMediaServiceImpl);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_MEDIA_SERVICE_IMPL_H_
