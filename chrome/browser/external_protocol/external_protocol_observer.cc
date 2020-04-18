// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/external_protocol/external_protocol_observer.h"

#include "chrome/browser/external_protocol/external_protocol_handler.h"

using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ExternalProtocolObserver);

ExternalProtocolObserver::ExternalProtocolObserver(WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
}

ExternalProtocolObserver::~ExternalProtocolObserver() {
}

void ExternalProtocolObserver::DidGetUserInteraction(
    const blink::WebInputEvent::Type type) {
  // Ignore scroll events for allowing external protocol launch.
  if (type != blink::WebInputEvent::kGestureScrollBegin)
    ExternalProtocolHandler::PermitLaunchUrl();
}
