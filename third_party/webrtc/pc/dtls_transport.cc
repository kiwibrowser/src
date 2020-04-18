/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "pc/dtls_transport.h"

#include <utility>

#include "pc/ice_transport.h"

namespace webrtc {

namespace {

DtlsTransportState TranslateState(cricket::DtlsTransportState internal_state) {
  switch (internal_state) {
    case cricket::DTLS_TRANSPORT_NEW:
      return DtlsTransportState::kNew;
      break;
    case cricket::DTLS_TRANSPORT_CONNECTING:
      return DtlsTransportState::kConnecting;
      break;
    case cricket::DTLS_TRANSPORT_CONNECTED:
      return DtlsTransportState::kConnected;
      break;
    case cricket::DTLS_TRANSPORT_CLOSED:
      return DtlsTransportState::kClosed;
      break;
    case cricket::DTLS_TRANSPORT_FAILED:
      return DtlsTransportState::kFailed;
      break;
  }
}

}  // namespace

// Implementation of DtlsTransportInterface
DtlsTransport::DtlsTransport(
    std::unique_ptr<cricket::DtlsTransportInternal> internal)
    : signaling_thread_(rtc::Thread::Current()),
      internal_dtls_transport_(std::move(internal)) {
  RTC_DCHECK(internal_dtls_transport_.get());
  internal_dtls_transport_->SignalDtlsState.connect(
      this, &DtlsTransport::OnInternalDtlsState);
  ice_transport_ = new rtc::RefCountedObject<IceTransportWithPointer>(
      internal_dtls_transport_->ice_transport());
}

DtlsTransport::~DtlsTransport() {
  // We depend on the signaling thread to call Clear() before dropping
  // its last reference to this object.
  RTC_DCHECK(signaling_thread_->IsCurrent() || !internal_dtls_transport_);
}

DtlsTransportInformation DtlsTransport::Information() {
  RTC_DCHECK(signaling_thread_->IsCurrent());
  if (internal()) {
    return DtlsTransportInformation(TranslateState(internal()->dtls_state()));
  } else {
    return DtlsTransportInformation(DtlsTransportState::kClosed);
  }
}

void DtlsTransport::RegisterObserver(DtlsTransportObserverInterface* observer) {
  RTC_DCHECK(signaling_thread_->IsCurrent());
  RTC_DCHECK(observer);
  observer_ = observer;
}

void DtlsTransport::UnregisterObserver() {
  RTC_DCHECK(signaling_thread_->IsCurrent());
  observer_ = nullptr;
}

rtc::scoped_refptr<IceTransportInterface> DtlsTransport::ice_transport() {
  return ice_transport_;
}

// Internal functions
void DtlsTransport::Clear() {
  RTC_DCHECK(signaling_thread_->IsCurrent());
  RTC_DCHECK(internal());
  if (internal()->dtls_state() != cricket::DTLS_TRANSPORT_CLOSED) {
    internal_dtls_transport_.reset();
    if (observer_) {
      observer_->OnStateChange(Information());
    }
  } else {
    internal_dtls_transport_.reset();
  }
  ice_transport_->Clear();
}

void DtlsTransport::OnInternalDtlsState(
    cricket::DtlsTransportInternal* transport,
    cricket::DtlsTransportState state) {
  RTC_DCHECK(signaling_thread_->IsCurrent());
  RTC_DCHECK(transport == internal());
  RTC_DCHECK(state == internal()->dtls_state());
  if (observer_) {
    observer_->OnStateChange(Information());
  }
}

}  // namespace webrtc
