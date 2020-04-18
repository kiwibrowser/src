/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_ICE_TRANSPORT_FACTORY_H_
#define API_ICE_TRANSPORT_FACTORY_H_

#include "api/ice_transport_interface.h"
#include "api/scoped_refptr.h"

namespace cricket {
class PortAllocator;
}

namespace webrtc {

// Static factory for an IceTransport object that can be created
// without using a webrtc::PeerConnection.
// The returned object must be accessed and destroyed on the thread that
// created it.
// The PortAllocator must outlive the created IceTransportInterface object.
rtc::scoped_refptr<IceTransportInterface> CreateIceTransport(
    cricket::PortAllocator* port_allocator);

}  // namespace webrtc

#endif  // API_ICE_TRANSPORT_FACTORY_H_
