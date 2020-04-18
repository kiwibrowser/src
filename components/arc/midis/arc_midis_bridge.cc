// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/midis/arc_midis_bridge.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/singleton.h"
#include "chromeos/dbus/arc_midis_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace arc {
namespace {

// Singleton factory for ArcMidisBridge
class ArcMidisBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcMidisBridge,
          ArcMidisBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcMidisBridgeFactory";

  static ArcMidisBridgeFactory* GetInstance() {
    return base::Singleton<ArcMidisBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcMidisBridgeFactory>;
  ArcMidisBridgeFactory() = default;
  ~ArcMidisBridgeFactory() override = default;
};

}  // namespace

// static
ArcMidisBridge* ArcMidisBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcMidisBridgeFactory::GetForBrowserContext(context);
}

ArcMidisBridge::ArcMidisBridge(content::BrowserContext* context,
                               ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service), weak_factory_(this) {
  arc_bridge_service_->midis()->SetHost(this);
}

ArcMidisBridge::~ArcMidisBridge() {
  arc_bridge_service_->midis()->SetHost(nullptr);
}

void ArcMidisBridge::OnBootstrapMojoConnection(
    mojom::MidisServerRequest request,
    mojom::MidisClientPtr client_ptr,
    bool result) {
  if (!result) {
    LOG(ERROR) << "ArcMidisBridge had a failure in D-Bus with the daemon.";
    midis_host_ptr_.reset();
    return;
  }
  DVLOG(1) << "ArcMidisBridge succeeded with Mojo bootstrapping.";
  midis_host_ptr_->Connect(std::move(request), std::move(client_ptr));
}

void ArcMidisBridge::Connect(mojom::MidisServerRequest request,
                             mojom::MidisClientPtr client_ptr) {
  if (midis_host_ptr_.is_bound()) {
    DVLOG(1) << "Re-using bootstrap connection for MidisServer Connect.";
    midis_host_ptr_->Connect(std::move(request), std::move(client_ptr));
    return;
  }
  DVLOG(1) << "Bootstrapping the Midis connection via D-Bus.";
  mojo::edk::OutgoingBrokerClientInvitation invitation;
  mojo::edk::PlatformChannelPair channel_pair;
  mojo::ScopedMessagePipeHandle server_pipe =
      invitation.AttachMessagePipe("arc-midis-pipe");
  invitation.Send(
      base::kNullProcessHandle,
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  channel_pair.PassServerHandle()));
  mojo::edk::ScopedInternalPlatformHandle child_handle =
      channel_pair.PassClientHandle();
  base::ScopedFD fd(child_handle.release().handle);

  midis_host_ptr_.Bind(
      mojo::InterfacePtrInfo<mojom::MidisHost>(std::move(server_pipe), 0u));
  DVLOG(1) << "Bound remote MidisHost interface to pipe.";
  midis_host_ptr_.set_connection_error_handler(
      base::BindOnce(&mojo::InterfacePtr<mojom::MidisHost>::reset,
                     base::Unretained(&midis_host_ptr_)));
  chromeos::DBusThreadManager::Get()
      ->GetArcMidisClient()
      ->BootstrapMojoConnection(
          std::move(fd),
          base::BindOnce(&ArcMidisBridge::OnBootstrapMojoConnection,
                         weak_factory_.GetWeakPtr(), std::move(request),
                         std::move(client_ptr)));
}

}  // namespace arc
