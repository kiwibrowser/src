// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_stream_parser.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "third_party/libjingle_xmpp/xmllite/xmlbuilder.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmllite/xmlparser.h"

namespace remoting {

class XmppStreamParser::Core : public buzz::XmlParseHandler {
 public:
  typedef base::Callback<void(std::unique_ptr<buzz::XmlElement> stanza)>
      OnStanzaCallback;

  Core();
  ~Core() override;

  void SetCallbacks(const OnStanzaCallback& on_stanza_callback,
                    const base::Closure& on_error_callback);

  void AppendData(const std::string& data);

 private:
  // buzz::XmlParseHandler interface.
  void StartElement(buzz::XmlParseContext* context,
                    const char* name,
                    const char** atts) override;
  void EndElement(buzz::XmlParseContext* context, const char* name) override;
  void CharacterData(buzz::XmlParseContext* context,
                     const char* text,
                     int len) override;
  void Error(buzz::XmlParseContext* context, XML_Error error_code) override;

  void ProcessError();

  OnStanzaCallback on_stanza_callback_;
  base::Closure on_error_callback_;

  buzz::XmlParser parser_;
  int depth_;
  buzz::XmlBuilder builder_;

  bool error_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

XmppStreamParser::Core::Core()
    : parser_(this),
      depth_(0),
      error_(false) {
}

XmppStreamParser::Core::~Core() = default;

void XmppStreamParser::Core::SetCallbacks(
    const OnStanzaCallback& on_stanza_callback,
    const base::Closure& on_error_callback) {
  on_stanza_callback_ = on_stanza_callback;
  on_error_callback_ = on_error_callback;
}

void XmppStreamParser::Core::AppendData(const std::string& data) {
  if (error_)
    return;
  parser_.Parse(data.data(), data.size(), false);
}

void XmppStreamParser::Core::StartElement(buzz::XmlParseContext* context,
                                    const char* name,
                                    const char** atts) {
  DCHECK(!error_);

  ++depth_;
  if (depth_ == 1) {
    std::unique_ptr<buzz::XmlElement> header(
        buzz::XmlBuilder::BuildElement(context, name, atts));
    if (!header) {
      LOG(ERROR) << "Failed to parse XMPP stream header.";
      ProcessError();
    }
    return;
  }

  builder_.StartElement(context, name, atts);
}

void XmppStreamParser::Core::EndElement(buzz::XmlParseContext* context,
                                        const char* name) {
  DCHECK(!error_);

  --depth_;
  if (depth_ == 0) {
    LOG(ERROR) << "XMPP stream ended unexpectedly.";
    ProcessError();
    return;
  }

  builder_.EndElement(context, name);

  if (depth_ == 1) {
    if (!on_stanza_callback_.is_null())
      on_stanza_callback_.Run(base::WrapUnique(builder_.CreateElement()));
  }
}

void XmppStreamParser::Core::CharacterData(buzz::XmlParseContext* context,
                                           const char* text,
                                           int len) {
  DCHECK(!error_);

  // Ignore data between stanzas.
  if (depth_ <= 1) {
    // Only whitespace is allowed outside of the stanzas.
    bool all_spaces = true;
    for (char c: std::string(text, len)) {
      if (c != ' ') {
        all_spaces = false;
        break;
      }
    }
    if (!all_spaces) {
      LOG(ERROR) << "Received unexpected string: " << std::string(text,
                                                                  text + len);
      ProcessError();
    }
  } else if (depth_ > 1) {
    builder_.CharacterData(context, text, len);
  }
}

void XmppStreamParser::Core::Error(buzz::XmlParseContext* context,
                                   XML_Error error_code) {
  LOG(ERROR) << "XMPP parser error: " << error_code;
  ProcessError();
}

void XmppStreamParser::Core::ProcessError() {
  error_ = true;
  if (!on_error_callback_.is_null())
    on_error_callback_.Run();
}

XmppStreamParser::XmppStreamParser() : core_(new Core()) {
}

XmppStreamParser::~XmppStreamParser() {
  // Set null callbacks and delete |core_| asynchronously to make sure it's not
  // deleted from a callback.
  core_->SetCallbacks(OnStanzaCallback(), base::Closure());
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, core_.release());
}

void XmppStreamParser::SetCallbacks(const OnStanzaCallback& on_stanza_callback,
                                    const base::Closure& on_error_callback) {
  core_->SetCallbacks(on_stanza_callback, on_error_callback);
}

void XmppStreamParser::AppendData(const std::string& data) {
  core_->AppendData(data);
}

}  // namespace remoting
