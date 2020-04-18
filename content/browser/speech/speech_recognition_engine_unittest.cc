// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/speech_recognition_engine.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/big_endian.h"
#include "base/containers/queue.h"
#include "base/message_loop/message_loop.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_byteorder.h"
#include "content/browser/speech/audio_buffer.h"
#include "content/browser/speech/proto/google_streaming_api.pb.h"
#include "content/public/common/speech_recognition_error.h"
#include "content/public/common/speech_recognition_result.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::HostToNet32;
using base::checked_cast;
using net::URLRequestStatus;
using net::TestURLFetcher;
using net::TestURLFetcherFactory;

namespace content {

// Frame types for framed POST data.
static const uint32_t kFrameTypePreamble = 0;
static const uint32_t kFrameTypeRecognitionAudio = 1;

// Note: the terms upstream and downstream are from the point-of-view of the
// client (engine_under_test_).

class SpeechRecognitionEngineTest
    : public SpeechRecognitionEngine::Delegate,
      public testing::Test {
 public:
  SpeechRecognitionEngineTest()
      : last_number_of_upstream_chunks_seen_(0U),
        error_(SPEECH_RECOGNITION_ERROR_NONE),
        end_of_utterance_counter_(0) { }

  // Creates a speech recognition request and invokes its URL fetcher delegate
  // with the given test data.
  void CreateAndTestRequest(bool success, const std::string& http_response);

  // SpeechRecognitionRequestDelegate methods.
  void OnSpeechRecognitionEngineResults(
      const SpeechRecognitionResults& results) override {
    results_.push(results);
  }
  void OnSpeechRecognitionEngineEndOfUtterance() override {
    ++end_of_utterance_counter_;
  }
  void OnSpeechRecognitionEngineError(
      const SpeechRecognitionError& error) override {
    error_ = error.code;
  }

  // testing::Test methods.
  void SetUp() override;
  void TearDown() override;

 protected:
  enum DownstreamError {
    DOWNSTREAM_ERROR_NONE,
    DOWNSTREAM_ERROR_HTTP500,
    DOWNSTREAM_ERROR_NETWORK,
    DOWNSTREAM_ERROR_WEBSERVICE_NO_MATCH
  };
  static bool ResultsAreEqual(const SpeechRecognitionResults& a,
                              const SpeechRecognitionResults& b);
  static std::string SerializeProtobufResponse(
      const proto::SpeechRecognitionEvent& msg);

  TestURLFetcher* GetUpstreamFetcher();
  TestURLFetcher* GetDownstreamFetcher();
  void StartMockRecognition();
  void EndMockRecognition();
  void InjectDummyAudioChunk();
  size_t UpstreamChunksUploadedFromLastCall();
  std::string LastUpstreamChunkUploaded();
  void ProvideMockProtoResultDownstream(
      const proto::SpeechRecognitionEvent& result);
  void ProvideMockResultDownstream(const SpeechRecognitionResult& result);
  void ExpectResultsReceived(const SpeechRecognitionResults& result);
  void ExpectFramedChunk(const std::string& chunk, uint32_t type);
  void CloseMockDownstream(DownstreamError error);

  std::unique_ptr<SpeechRecognitionEngine> engine_under_test_;
  TestURLFetcherFactory url_fetcher_factory_;
  size_t last_number_of_upstream_chunks_seen_;
  base::MessageLoop message_loop_;
  std::string response_buffer_;
  SpeechRecognitionErrorCode error_;
  int end_of_utterance_counter_;
  base::queue<SpeechRecognitionResults> results_;
};

TEST_F(SpeechRecognitionEngineTest, SingleDefinitiveResult) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  // Inject some dummy audio chunks and check a corresponding chunked upload
  // is performed every time on the server.
  for (int i = 0; i < 3; ++i) {
    InjectDummyAudioChunk();
    ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
  }

  // Ensure that a final (empty) audio chunk is uploaded on chunks end.
  engine_under_test_->AudioChunksEnded();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Simulate a protobuf message streamed from the server containing a single
  // result with two hypotheses.
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  result.is_provisional = false;
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("hypothesis 1"), 0.1F));
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("hypothesis 2"), 0.2F));

  ProvideMockResultDownstream(result);
  ExpectResultsReceived(results);
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Ensure everything is closed cleanly after the downstream is closed.
  CloseMockDownstream(DOWNSTREAM_ERROR_NONE);
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  ASSERT_EQ(0U, results_.size());
}

TEST_F(SpeechRecognitionEngineTest, SeveralStreamingResults) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  for (int i = 0; i < 4; ++i) {
    InjectDummyAudioChunk();
    ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());

    SpeechRecognitionResults results;
    results.push_back(SpeechRecognitionResult());
    SpeechRecognitionResult& result = results.back();
    result.is_provisional = (i % 2 == 0);  // Alternate result types.
    float confidence = result.is_provisional ? 0.0F : (i * 0.1F);
    result.hypotheses.push_back(SpeechRecognitionHypothesis(
        base::UTF8ToUTF16("hypothesis"), confidence));

    ProvideMockResultDownstream(result);
    ExpectResultsReceived(results);
    ASSERT_TRUE(engine_under_test_->IsRecognitionPending());
  }

  // Ensure that a final (empty) audio chunk is uploaded on chunks end.
  engine_under_test_->AudioChunksEnded();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Simulate a final definitive result.
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  result.is_provisional = false;
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("The final result"), 1.0F));
  ProvideMockResultDownstream(result);
  ExpectResultsReceived(results);
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Ensure everything is closed cleanly after the downstream is closed.
  CloseMockDownstream(DOWNSTREAM_ERROR_NONE);
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  ASSERT_EQ(0U, results_.size());
}

TEST_F(SpeechRecognitionEngineTest, NoFinalResultAfterAudioChunksEnded) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  // Simulate one pushed audio chunk.
  InjectDummyAudioChunk();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());

  // Simulate the corresponding definitive result.
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("hypothesis"), 1.0F));
  ProvideMockResultDownstream(result);
  ExpectResultsReceived(results);
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Simulate a silent downstream closure after |AudioChunksEnded|.
  engine_under_test_->AudioChunksEnded();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());
  CloseMockDownstream(DOWNSTREAM_ERROR_NONE);

  // Expect an empty result, aimed at notifying recognition ended with no
  // actual results nor errors.
  SpeechRecognitionResults empty_results;
  ExpectResultsReceived(empty_results);

  // Ensure everything is closed cleanly after the downstream is closed.
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  ASSERT_EQ(0U, results_.size());
}

TEST_F(SpeechRecognitionEngineTest, NoMatchError) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  for (int i = 0; i < 3; ++i)
    InjectDummyAudioChunk();
  engine_under_test_->AudioChunksEnded();
  ASSERT_EQ(4U, UpstreamChunksUploadedFromLastCall());
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Simulate only a provisional result.
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  result.is_provisional = true;
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("The final result"), 0.0F));
  ProvideMockResultDownstream(result);
  ExpectResultsReceived(results);
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  CloseMockDownstream(DOWNSTREAM_ERROR_WEBSERVICE_NO_MATCH);

  // Expect an empty result.
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  SpeechRecognitionResults empty_result;
  ExpectResultsReceived(empty_result);
}

TEST_F(SpeechRecognitionEngineTest, HTTPError) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  InjectDummyAudioChunk();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());

  // Close the downstream with a HTTP 500 error.
  CloseMockDownstream(DOWNSTREAM_ERROR_HTTP500);

  // Expect a SPEECH_RECOGNITION_ERROR_NETWORK error to be raised.
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NETWORK, error_);
  ASSERT_EQ(0U, results_.size());
}

TEST_F(SpeechRecognitionEngineTest, NetworkError) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  InjectDummyAudioChunk();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());

  // Close the downstream fetcher simulating a network failure.
  CloseMockDownstream(DOWNSTREAM_ERROR_NETWORK);

  // Expect a SPEECH_RECOGNITION_ERROR_NETWORK error to be raised.
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NETWORK, error_);
  ASSERT_EQ(0U, results_.size());
}

TEST_F(SpeechRecognitionEngineTest, Stability) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  ASSERT_EQ(0U, UpstreamChunksUploadedFromLastCall());

  // Upload a dummy audio chunk.
  InjectDummyAudioChunk();
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
  engine_under_test_->AudioChunksEnded();

  // Simulate a protobuf message with an intermediate result without confidence,
  // but with stability.
  proto::SpeechRecognitionEvent proto_event;
  proto_event.set_status(proto::SpeechRecognitionEvent::STATUS_SUCCESS);
  proto::SpeechRecognitionResult* proto_result = proto_event.add_result();
  proto_result->set_stability(0.5);
  proto::SpeechRecognitionAlternative *proto_alternative =
      proto_result->add_alternative();
  proto_alternative->set_transcript("foo");
  ProvideMockProtoResultDownstream(proto_event);

  // Set up expectations.
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  result.is_provisional = true;
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("foo"), 0.5));

  // Check that the protobuf generated the expected result.
  ExpectResultsReceived(results);

  // Since it was a provisional result, recognition is still pending.
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Shut down.
  CloseMockDownstream(DOWNSTREAM_ERROR_NONE);
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();

  // Since there was no final result, we get an empty "no match" result.
  SpeechRecognitionResults empty_result;
  ExpectResultsReceived(empty_result);
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  ASSERT_EQ(0U, results_.size());
}

TEST_F(SpeechRecognitionEngineTest, EndOfUtterance) {
  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());

  // Simulate a END_OF_UTTERANCE proto event with continuous true.
  SpeechRecognitionEngine::Config config;
  config.continuous = true;
  engine_under_test_->SetConfig(config);
  proto::SpeechRecognitionEvent proto_event;
  proto_event.set_endpoint(proto::SpeechRecognitionEvent::END_OF_UTTERANCE);
  ASSERT_EQ(0, end_of_utterance_counter_);
  ProvideMockProtoResultDownstream(proto_event);
  ASSERT_EQ(0, end_of_utterance_counter_);

  // Simulate a END_OF_UTTERANCE proto event with continuous false.
  config.continuous = false;
  engine_under_test_->SetConfig(config);
  ProvideMockProtoResultDownstream(proto_event);
  ASSERT_EQ(1, end_of_utterance_counter_);

  // Shut down.
  CloseMockDownstream(DOWNSTREAM_ERROR_NONE);
  EndMockRecognition();
}

TEST_F(SpeechRecognitionEngineTest, SendPreamble) {
  const size_t kPreambleLength = 100;
  scoped_refptr<SpeechRecognitionSessionPreamble> preamble =
      new SpeechRecognitionSessionPreamble();
  preamble->sample_rate = 16000;
  preamble->sample_depth = 2;
  preamble->sample_data.assign(kPreambleLength, 0);
  SpeechRecognitionEngine::Config config;
  config.auth_token = "foo";
  config.auth_scope = "bar";
  config.preamble = preamble;
  engine_under_test_->SetConfig(config);

  StartMockRecognition();
  ASSERT_TRUE(GetUpstreamFetcher());
  // First chunk uploaded should be the preamble.
  ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
  std::string chunk = LastUpstreamChunkUploaded();
  ExpectFramedChunk(chunk, kFrameTypePreamble);

  for (int i = 0; i < 3; ++i) {
    InjectDummyAudioChunk();
    ASSERT_EQ(1U, UpstreamChunksUploadedFromLastCall());
    chunk = LastUpstreamChunkUploaded();
    ExpectFramedChunk(chunk, kFrameTypeRecognitionAudio);
  }
  engine_under_test_->AudioChunksEnded();
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Simulate a protobuf message streamed from the server containing a single
  // result with one hypotheses.
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  result.is_provisional = false;
  result.hypotheses.push_back(
      SpeechRecognitionHypothesis(base::UTF8ToUTF16("hypothesis 1"), 0.1F));

  ProvideMockResultDownstream(result);
  ExpectResultsReceived(results);
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  // Ensure everything is closed cleanly after the downstream is closed.
  CloseMockDownstream(DOWNSTREAM_ERROR_NONE);
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());
  EndMockRecognition();
  ASSERT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  ASSERT_EQ(0U, results_.size());
}

void SpeechRecognitionEngineTest::SetUp() {
  engine_under_test_.reset(
      new SpeechRecognitionEngine(nullptr /*URLRequestContextGetter*/));
  engine_under_test_->set_delegate(this);
}

void SpeechRecognitionEngineTest::TearDown() {
  engine_under_test_.reset();
}

TestURLFetcher* SpeechRecognitionEngineTest::GetUpstreamFetcher() {
  return url_fetcher_factory_.GetFetcherByID(
        SpeechRecognitionEngine::kUpstreamUrlFetcherIdForTesting);
}

TestURLFetcher* SpeechRecognitionEngineTest::GetDownstreamFetcher() {
  return url_fetcher_factory_.GetFetcherByID(
        SpeechRecognitionEngine::kDownstreamUrlFetcherIdForTesting);
}

// Starts recognition on the engine, ensuring that both stream fetchers are
// created.
void SpeechRecognitionEngineTest::StartMockRecognition() {
  DCHECK(engine_under_test_.get());

  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());

  engine_under_test_->StartRecognition();
  ASSERT_TRUE(engine_under_test_->IsRecognitionPending());

  TestURLFetcher* upstream_fetcher = GetUpstreamFetcher();
  ASSERT_TRUE(upstream_fetcher);
  upstream_fetcher->set_url(upstream_fetcher->GetOriginalURL());

  TestURLFetcher* downstream_fetcher = GetDownstreamFetcher();
  ASSERT_TRUE(downstream_fetcher);
  downstream_fetcher->set_url(downstream_fetcher->GetOriginalURL());
}

void SpeechRecognitionEngineTest::EndMockRecognition() {
  DCHECK(engine_under_test_.get());
  engine_under_test_->EndRecognition();
  ASSERT_FALSE(engine_under_test_->IsRecognitionPending());

  // TODO(primiano): In order to be very pedantic we should check that both the
  // upstream and downstream URL fetchers have been disposed at this time.
  // Unfortunately it seems that there is no direct way to detect (in tests)
  // if a url_fetcher has been freed or not, since they are not automatically
  // de-registered from the TestURLFetcherFactory on destruction.
}

void SpeechRecognitionEngineTest::InjectDummyAudioChunk() {
  unsigned char dummy_audio_buffer_data[2] = {'\0', '\0'};
  scoped_refptr<AudioChunk> dummy_audio_chunk(
      new AudioChunk(&dummy_audio_buffer_data[0],
                     sizeof(dummy_audio_buffer_data),
                     2 /* bytes per sample */));
  DCHECK(engine_under_test_.get());
  engine_under_test_->TakeAudioChunk(*dummy_audio_chunk.get());
}

size_t SpeechRecognitionEngineTest::UpstreamChunksUploadedFromLastCall() {
  TestURLFetcher* upstream_fetcher = GetUpstreamFetcher();
  DCHECK(upstream_fetcher);
  const size_t number_of_chunks = upstream_fetcher->upload_chunks().size();
  DCHECK_GE(number_of_chunks, last_number_of_upstream_chunks_seen_);
  const size_t new_chunks = number_of_chunks -
                            last_number_of_upstream_chunks_seen_;
  last_number_of_upstream_chunks_seen_ = number_of_chunks;
  return new_chunks;
}

std::string SpeechRecognitionEngineTest::LastUpstreamChunkUploaded() {
  TestURLFetcher* upstream_fetcher = GetUpstreamFetcher();
  DCHECK(upstream_fetcher);
  DCHECK(!upstream_fetcher->upload_chunks().empty());
  return upstream_fetcher->upload_chunks().back();
}

void SpeechRecognitionEngineTest::ProvideMockProtoResultDownstream(
    const proto::SpeechRecognitionEvent& result) {
  TestURLFetcher* downstream_fetcher = GetDownstreamFetcher();

  ASSERT_TRUE(downstream_fetcher);
  downstream_fetcher->set_status(URLRequestStatus(/* default=SUCCESS */));
  downstream_fetcher->set_response_code(200);

  std::string response_string = SerializeProtobufResponse(result);
  response_buffer_.append(response_string);
  downstream_fetcher->SetResponseString(response_buffer_);
  downstream_fetcher->delegate()->OnURLFetchDownloadProgress(
      downstream_fetcher, response_buffer_.size(),
      -1 /* total response length not used */, response_buffer_.size());
}

void SpeechRecognitionEngineTest::ProvideMockResultDownstream(
    const SpeechRecognitionResult& result) {
  proto::SpeechRecognitionEvent proto_event;
  proto_event.set_status(proto::SpeechRecognitionEvent::STATUS_SUCCESS);
  proto::SpeechRecognitionResult* proto_result = proto_event.add_result();
  proto_result->set_final(!result.is_provisional);
  for (size_t i = 0; i < result.hypotheses.size(); ++i) {
    proto::SpeechRecognitionAlternative* proto_alternative =
        proto_result->add_alternative();
    const SpeechRecognitionHypothesis& hypothesis = result.hypotheses[i];
    proto_alternative->set_confidence(hypothesis.confidence);
    proto_alternative->set_transcript(base::UTF16ToUTF8(hypothesis.utterance));
  }
  ProvideMockProtoResultDownstream(proto_event);
}

void SpeechRecognitionEngineTest::CloseMockDownstream(
    DownstreamError error) {
  TestURLFetcher* downstream_fetcher = GetDownstreamFetcher();
  ASSERT_TRUE(downstream_fetcher);

  const net::Error net_error =
      (error == DOWNSTREAM_ERROR_NETWORK) ? net::ERR_FAILED : net::OK;
  downstream_fetcher->set_status(URLRequestStatus::FromError(net_error));
  downstream_fetcher->set_response_code(
      (error == DOWNSTREAM_ERROR_HTTP500) ? 500 : 200);

  if (error == DOWNSTREAM_ERROR_WEBSERVICE_NO_MATCH) {
    // Send empty response.
    proto::SpeechRecognitionEvent response;
    response_buffer_.append(SerializeProtobufResponse(response));
  }
  downstream_fetcher->SetResponseString(response_buffer_);
  downstream_fetcher->delegate()->OnURLFetchComplete(downstream_fetcher);
}

void SpeechRecognitionEngineTest::ExpectResultsReceived(
    const SpeechRecognitionResults& results) {
  ASSERT_GE(1U, results_.size());
  ASSERT_TRUE(ResultsAreEqual(results, results_.front()));
  results_.pop();
}

bool SpeechRecognitionEngineTest::ResultsAreEqual(
    const SpeechRecognitionResults& a, const SpeechRecognitionResults& b) {
  if (a.size() != b.size())
    return false;

  SpeechRecognitionResults::const_iterator it_a = a.begin();
  SpeechRecognitionResults::const_iterator it_b = b.begin();
  for (; it_a != a.end() && it_b != b.end(); ++it_a, ++it_b) {
    if (it_a->is_provisional != it_b->is_provisional ||
        it_a->hypotheses.size() != it_b->hypotheses.size()) {
      return false;
    }
    for (size_t i = 0; i < it_a->hypotheses.size(); ++i) {
      const SpeechRecognitionHypothesis& hyp_a = it_a->hypotheses[i];
      const SpeechRecognitionHypothesis& hyp_b = it_b->hypotheses[i];
      if (hyp_a.utterance != hyp_b.utterance ||
          hyp_a.confidence != hyp_b.confidence) {
        return false;
      }
    }
  }

  return true;
}

void SpeechRecognitionEngineTest::ExpectFramedChunk(
    const std::string& chunk, uint32_t type) {
  uint32_t value;
  base::ReadBigEndian(&chunk[0], &value);
  EXPECT_EQ(chunk.size() - 8, value);
  base::ReadBigEndian(&chunk[4], &value);
  EXPECT_EQ(type, value);
}

std::string SpeechRecognitionEngineTest::SerializeProtobufResponse(
    const proto::SpeechRecognitionEvent& msg) {
  std::string msg_string;
  msg.SerializeToString(&msg_string);

  // Prepend 4 byte prefix length indication to the protobuf message as
  // envisaged by the google streaming recognition webservice protocol.
  uint32_t prefix = HostToNet32(checked_cast<uint32_t>(msg_string.size()));
  msg_string.insert(0, reinterpret_cast<char*>(&prefix), sizeof(prefix));

  return msg_string;
}

}  // namespace content
