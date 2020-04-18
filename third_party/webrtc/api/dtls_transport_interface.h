/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_DTLS_TRANSPORT_INTERFACE_H_
#define API_DTLS_TRANSPORT_INTERFACE_H_

#include "api/ice_transport_interface.h"
#include "api/rtc_error.h"
#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"

namespace webrtc {

// States of a DTLS transport, corresponding to the JS API specification.
// http://w3c.github.io/webrtc-pc/#dom-rtcdtlstransportstate
enum class DtlsTransportState {
  kNew,         // Has not started negotiating yet.
  kConnecting,  // In the process of negotiating a secure connection.
  kConnected,   // Completed negotiation and verified fingerprints.
  kClosed,      // Intentionally closed.
  kFailed,      // Failure due to an error or failing to verify a remote
                // fingerprint.
  kNumValues
};

// This object gives snapshot information about the changeable state of a
// DTLSTransport.
class DtlsTransportInformation {
 public:
  explicit DtlsTransportInformation(DtlsTransportState state) : state_(state) {}
  DtlsTransportState state() const { return state_; }
  // TODO(hta): Add remote certificate access
 private:
  DtlsTransportState state_;
};

class DtlsTransportObserverInterface {
 public:
  // This callback carries information about the state of the transport.
  // The argument is a pass-by-value snapshot of the state.
  virtual void OnStateChange(DtlsTransportInformation info) = 0;
  // This callback is called when an error occurs, causing the transport
  // to go to the kFailed state.
  virtual void OnError(RTCError error) = 0;

 protected:
  virtual ~DtlsTransportObserverInterface() = default;
};

// A DTLS transport, as represented to the outside world.
// This object is created on the signaling thread, and can only be
// accessed on that thread.
// References can be held by other threads, and destruction can therefore
// be initiated by other threads.
class DtlsTransportInterface : public rtc::RefCountInterface {
 public:
  // Returns a pointer to the ICE transport that is owned by the DTLS transport.
  virtual rtc::scoped_refptr<IceTransportInterface> ice_transport() = 0;
  // These functions can only be called from the signalling thread.
  virtual DtlsTransportInformation Information() = 0;
  // Observer management.
  virtual void RegisterObserver(DtlsTransportObserverInterface* observer) = 0;
  virtual void UnregisterObserver() = 0;
};

}  // namespace webrtc

#endif  // API_DTLS_TRANSPORT_INTERFACE_H_
