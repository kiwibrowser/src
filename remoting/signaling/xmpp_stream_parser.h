// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_XMPP_STREAM_PARSER_H_
#define REMOTING_SIGNALING_XMPP_STREAM_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

// XmppStreamParser is used to parse XMPP stream. Data is fed to the parser
// using appendData() method and it calls |on_stanza_callback\ and
// |on_error_callback| specified using SetCallbacks().
class XmppStreamParser {
 public:
  typedef base::Callback<void(std::unique_ptr<buzz::XmlElement> stanza)>
      OnStanzaCallback;

  XmppStreamParser();
  ~XmppStreamParser();

  void SetCallbacks(const OnStanzaCallback& on_stanza_callback,
                    const base::Closure& on_error_callback);

  void AppendData(const std::string& data);

 private:
  class Core;

  std::unique_ptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(XmppStreamParser);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_XMPP_STREAM_PARSER_H_
