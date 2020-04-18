// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_CONSOLE_MESSAGE_STRUCT_TRAITS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_CONSOLE_MESSAGE_STRUCT_TRAITS_H_

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/blink/public/web/console_message.mojom-shared.h"
#include "third_party/blink/public/web/web_console_message.h"

namespace mojo {

template <>
struct EnumTraits<::blink::mojom::ConsoleMessageLevel,
                  ::blink::WebConsoleMessage::Level> {
  static ::blink::mojom::ConsoleMessageLevel ToMojom(
      ::blink::WebConsoleMessage::Level);
  static bool FromMojom(::blink::mojom::ConsoleMessageLevel,
                        ::blink::WebConsoleMessage::Level* out);
};

}  // namespace mojo

#endif
