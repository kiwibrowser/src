// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_stream_parser.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"

namespace remoting {

class XmppStreamParserTest : public testing::Test {
 public:
  XmppStreamParserTest() : error_(false) {}

  void SetUp() override {
    parser_.reset(new remoting::XmppStreamParser());
    parser_->SetCallbacks(
        base::Bind(&XmppStreamParserTest::OnStanza, base::Unretained(this)),
        base::Bind(&XmppStreamParserTest::OnError, base::Unretained(this)));
  }

  void TearDown() override {
    parser_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void OnStanza(std::unique_ptr<buzz::XmlElement> stanza) {
    received_stanzas_.push_back(std::move(stanza));
  }

  void OnError() {
    error_ = true;
  }

 protected:
  base::MessageLoop message_loop_;

  std::unique_ptr<XmppStreamParser> parser_;
  std::vector<std::unique_ptr<buzz::XmlElement>> received_stanzas_;
  bool error_;
};

TEST_F(XmppStreamParserTest, ParseXmppStream) {
  parser_->AppendData("<stream><iq>text</iq>");
  EXPECT_EQ(received_stanzas_[0]->Str(), "<iq>text</iq>");
};

TEST_F(XmppStreamParserTest, HandleMultipleIncomingStanzas) {
  parser_->AppendData("<stream><iq>text</iq><iq>more text</iq>");
  EXPECT_EQ(received_stanzas_[0]->Str(), "<iq>text</iq>");
  EXPECT_EQ(received_stanzas_[1]->Str(), "<iq>more text</iq>");
};

TEST_F(XmppStreamParserTest, IgnoreWhitespaceBetweenStanzas) {
  parser_->AppendData("<stream> <iq>text</iq>");
  EXPECT_EQ(received_stanzas_[0]->Str(), "<iq>text</iq>");
};

TEST_F(XmppStreamParserTest, AssembleMessagesFromChunks) {
  parser_->AppendData("<stream><i");
  parser_->AppendData("q>");

  // Split one UTF-8 sequence into two chunks
  std::string data = "😃";
  parser_->AppendData(data.substr(0, 2));
  parser_->AppendData(data.substr(2));

  parser_->AppendData("</iq>");

  EXPECT_EQ(received_stanzas_[0]->Str(), "<iq>😃</iq>");
};

TEST_F(XmppStreamParserTest, StopParsingOnErrors) {
  parser_->AppendData("<stream><invalidtag p!='a'></invalidtag><iq>text</iq>");
  EXPECT_TRUE(error_);
  EXPECT_TRUE(received_stanzas_.empty());
};

TEST_F(XmppStreamParserTest, FailOnInvalidStreamHeader) {
  parser_->AppendData("<stream p!='a'>");
  EXPECT_TRUE(error_);
};

TEST_F(XmppStreamParserTest, FailOnLooseText) {
  parser_->AppendData("stream<");
  EXPECT_TRUE(error_);
};

}  // namespace remoting
