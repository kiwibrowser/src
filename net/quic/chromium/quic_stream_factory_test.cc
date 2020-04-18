// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/chromium/quic_stream_factory.h"

#include <memory>
#include <ostream>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "build/build_config.h"
#include "net/base/completion_once_callback.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/do_nothing_ct_verifier.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/http_util.h"
#include "net/http/transport_security_state.h"
#include "net/quic/chromium/crypto/proof_verifier_chromium.h"
#include "net/quic/chromium/mock_crypto_client_stream_factory.h"
#include "net/quic/chromium/mock_quic_data.h"
#include "net/quic/chromium/properties_based_quic_server_info.h"
#include "net/quic/chromium/quic_http_stream.h"
#include "net/quic/chromium/quic_http_utils.h"
#include "net/quic/chromium/quic_server_info.h"
#include "net/quic/chromium/quic_stream_factory_peer.h"
#include "net/quic/chromium/quic_test_packet_maker.h"
#include "net/quic/chromium/test_task_runner.h"
#include "net/socket/next_proto.h"
#include "net/socket/socket_test_util.h"
#include "net/spdy/spdy_session_test_util.h"
#include "net/spdy/spdy_test_util_common.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/test/cert_test_util.h"
#include "net/test/gtest_util.h"
#include "net/test/test_data_directory.h"
#include "net/test/test_with_scoped_task_environment.h"
#include "net/third_party/quic/core/crypto/crypto_handshake.h"
#include "net/third_party/quic/core/crypto/quic_crypto_client_config.h"
#include "net/third_party/quic/core/crypto/quic_decrypter.h"
#include "net/third_party/quic/core/crypto/quic_encrypter.h"
#include "net/third_party/quic/core/quic_client_promised_info.h"
#include "net/third_party/quic/platform/api/quic_test.h"
#include "net/third_party/quic/test_tools/mock_clock.h"
#include "net/third_party/quic/test_tools/mock_random.h"
#include "net/third_party/quic/test_tools/quic_config_peer.h"
#include "net/third_party/quic/test_tools/quic_spdy_session_peer.h"
#include "net/third_party/quic/test_tools/quic_test_utils.h"
#include "net/third_party/spdy/core/spdy_test_utils.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using std::string;

namespace net {

namespace {

class MockSSLConfigService : public SSLConfigService {
 public:
  MockSSLConfigService() {}

  void GetSSLConfig(SSLConfig* config) override { *config = config_; }

 private:
  ~MockSSLConfigService() override {}

  SSLConfig config_;
};

}  // namespace

namespace test {

namespace {

enum DestinationType {
  // In pooling tests with two requests for different origins to the same
  // destination, the destination should be
  SAME_AS_FIRST,   // the same as the first origin,
  SAME_AS_SECOND,  // the same as the second origin, or
  DIFFERENT,       // different from both.
};

const char kDefaultServerHostName[] = "www.example.org";
const char kServer2HostName[] = "mail.example.org";
const char kDifferentHostname[] = "different.example.com";
const int kDefaultServerPort = 443;
const char kDefaultUrl[] = "https://www.example.org/";
const char kServer2Url[] = "https://mail.example.org/";
const char kServer3Url[] = "https://docs.example.org/";
const char kServer4Url[] = "https://images.example.org/";
const int kDefaultRTTMilliSecs = 300;
const size_t kMinRetryTimeForDefaultNetworkSecs = 1;

// Run QuicStreamFactoryTest instances with all value combinations of version
// and enable_connection_racting.
struct TestParams {
  friend std::ostream& operator<<(std::ostream& os, const TestParams& p) {
    os << "{ version: " << QuicVersionToString(p.version)
       << ", client_headers_include_h2_stream_dependency: "
       << p.client_headers_include_h2_stream_dependency << " }";
    return os;
  }

  QuicTransportVersion version;
  bool client_headers_include_h2_stream_dependency;
};

std::vector<TestParams> GetTestParams() {
  std::vector<TestParams> params;
  QuicTransportVersionVector all_supported_versions =
      AllSupportedTransportVersions();
  for (const auto& version : all_supported_versions) {
    params.push_back(TestParams{version, false});
    params.push_back(TestParams{version, true});
  }
  return params;
}

// Run QuicStreamFactoryWithDestinationTest instances with all value
// combinations of version, enable_connection_racting, and destination_type.
struct PoolingTestParams {
  friend std::ostream& operator<<(std::ostream& os,
                                  const PoolingTestParams& p) {
    os << "{ version: " << QuicVersionToString(p.version)
       << ", destination_type: ";
    switch (p.destination_type) {
      case SAME_AS_FIRST:
        os << "SAME_AS_FIRST";
        break;
      case SAME_AS_SECOND:
        os << "SAME_AS_SECOND";
        break;
      case DIFFERENT:
        os << "DIFFERENT";
        break;
    }
    os << ", client_headers_include_h2_stream_dependency: "
       << p.client_headers_include_h2_stream_dependency;
    os << " }";
    return os;
  }

  QuicTransportVersion version;
  DestinationType destination_type;
  bool client_headers_include_h2_stream_dependency;
};

std::vector<PoolingTestParams> GetPoolingTestParams() {
  std::vector<PoolingTestParams> params;
  QuicTransportVersionVector all_supported_versions =
      AllSupportedTransportVersions();
  for (const QuicTransportVersion version : all_supported_versions) {
    params.push_back(PoolingTestParams{version, SAME_AS_FIRST, false});
    params.push_back(PoolingTestParams{version, SAME_AS_FIRST, true});
    params.push_back(PoolingTestParams{version, SAME_AS_SECOND, false});
    params.push_back(PoolingTestParams{version, SAME_AS_SECOND, true});
    params.push_back(PoolingTestParams{version, DIFFERENT, false});
    params.push_back(PoolingTestParams{version, DIFFERENT, true});
  }
  return params;
}

}  // namespace

class QuicHttpStreamPeer {
 public:
  static QuicChromiumClientSession::Handle* GetSessionHandle(
      HttpStream* stream) {
    return static_cast<QuicHttpStream*>(stream)->quic_session();
  }
};

// TestConnectionMigrationSocketFactory will vend sockets with incremental port
// number.
class TestConnectionMigrationSocketFactory : public MockClientSocketFactory {
 public:
  TestConnectionMigrationSocketFactory() : next_source_port_num_(1u) {}
  ~TestConnectionMigrationSocketFactory() override {}

  std::unique_ptr<DatagramClientSocket> CreateDatagramClientSocket(
      DatagramSocket::BindType bind_type,
      NetLog* net_log,
      const NetLogSource& source) override {
    SocketDataProvider* data_provider = mock_data().GetNext();
    std::unique_ptr<MockUDPClientSocket> socket(
        new MockUDPClientSocket(data_provider, net_log));
    socket->set_source_port(next_source_port_num_++);
    return std::move(socket);
  }

 private:
  uint16_t next_source_port_num_;

  DISALLOW_COPY_AND_ASSIGN(TestConnectionMigrationSocketFactory);
};

class QuicStreamFactoryTestBase : public WithScopedTaskEnvironment {
 protected:
  QuicStreamFactoryTestBase(QuicTransportVersion version,
                            bool client_headers_include_h2_stream_dependency)
      : ssl_config_service_(new MockSSLConfigService),
        socket_factory_(new MockClientSocketFactory),
        random_generator_(0),
        runner_(new TestTaskRunner(&clock_)),
        version_(version),
        client_headers_include_h2_stream_dependency_(
            client_headers_include_h2_stream_dependency),
        client_maker_(version_,
                      0,
                      &clock_,
                      kDefaultServerHostName,
                      Perspective::IS_CLIENT,
                      client_headers_include_h2_stream_dependency_),
        server_maker_(version_,
                      0,
                      &clock_,
                      kDefaultServerHostName,
                      Perspective::IS_SERVER,
                      false),
        cert_verifier_(std::make_unique<MockCertVerifier>()),
        channel_id_service_(
            new ChannelIDService(new DefaultChannelIDStore(nullptr))),
        cert_transparency_verifier_(std::make_unique<DoNothingCTVerifier>()),
        scoped_mock_network_change_notifier_(nullptr),
        factory_(nullptr),
        host_port_pair_(kDefaultServerHostName, kDefaultServerPort),
        url_(kDefaultUrl),
        url2_(kServer2Url),
        url3_(kServer3Url),
        url4_(kServer4Url),
        privacy_mode_(PRIVACY_MODE_DISABLED),
        store_server_configs_in_properties_(false),
        close_sessions_on_ip_change_(false),
        idle_connection_timeout_seconds_(kIdleConnectionTimeoutSeconds),
        reduced_ping_timeout_seconds_(kPingTimeoutSecs),
        max_time_before_crypto_handshake_seconds_(
            kMaxTimeForCryptoHandshakeSecs),
        max_idle_time_before_crypto_handshake_seconds_(kInitialIdleTimeoutSecs),
        migrate_sessions_on_network_change_(false),
        migrate_sessions_early_(false),
        migrate_sessions_on_network_change_v2_(false),
        migrate_sessions_early_v2_(false),
        allow_server_migration_(false),
        race_cert_verification_(false),
        estimate_initial_rtt_(false) {
    clock_.AdvanceTime(QuicTime::Delta::FromSeconds(1));
  }

  void Initialize() {
    DCHECK(!factory_);
    factory_.reset(new QuicStreamFactory(
        net_log_.net_log(), &host_resolver_, ssl_config_service_.get(),
        socket_factory_.get(), &http_server_properties_, cert_verifier_.get(),
        &ct_policy_enforcer_, channel_id_service_.get(),
        &transport_security_state_, cert_transparency_verifier_.get(),
        /*SocketPerformanceWatcherFactory*/ nullptr,
        &crypto_client_stream_factory_, &random_generator_, &clock_,
        kDefaultMaxPacketSize, string(), store_server_configs_in_properties_,
        close_sessions_on_ip_change_,
        /*mark_quic_broken_when_network_blackholes*/ false,
        idle_connection_timeout_seconds_, reduced_ping_timeout_seconds_,
        max_time_before_crypto_handshake_seconds_,
        max_idle_time_before_crypto_handshake_seconds_,
        migrate_sessions_on_network_change_, migrate_sessions_early_,
        migrate_sessions_on_network_change_v2_, migrate_sessions_early_v2_,
        base::TimeDelta::FromSeconds(kMaxTimeOnNonDefaultNetworkSecs),
        kMaxMigrationsToNonDefaultNetworkOnPathDegrading,
        allow_server_migration_, race_cert_verification_, estimate_initial_rtt_,
        client_headers_include_h2_stream_dependency_, connection_options_,
        client_connection_options_, /*enable_token_binding*/ false,
        /*enable_channel_id*/ false,
        /*enable_socket_recv_optimization*/ false));
  }

  void InitializeConnectionMigrationTest(
      NetworkChangeNotifier::NetworkList connected_networks) {
    scoped_mock_network_change_notifier_.reset(
        new ScopedMockNetworkChangeNotifier());
    MockNetworkChangeNotifier* mock_ncn =
        scoped_mock_network_change_notifier_->mock_network_change_notifier();
    mock_ncn->ForceNetworkHandlesSupported();
    mock_ncn->SetConnectedNetworksList(connected_networks);
    migrate_sessions_on_network_change_ = true;
    migrate_sessions_early_ = true;
    Initialize();
  }

  void InitializeConnectionMigrationV2Test(
      NetworkChangeNotifier::NetworkList connected_networks) {
    scoped_mock_network_change_notifier_.reset(
        new ScopedMockNetworkChangeNotifier());
    MockNetworkChangeNotifier* mock_ncn =
        scoped_mock_network_change_notifier_->mock_network_change_notifier();
    mock_ncn->ForceNetworkHandlesSupported();
    mock_ncn->SetConnectedNetworksList(connected_networks);
    migrate_sessions_on_network_change_v2_ = true;
    migrate_sessions_early_v2_ = true;
    socket_factory_.reset(new TestConnectionMigrationSocketFactory);
    Initialize();
  }

  std::unique_ptr<HttpStream> CreateStream(QuicStreamRequest* request) {
    std::unique_ptr<QuicChromiumClientSession::Handle> session =
        request->ReleaseSessionHandle();
    if (!session || !session->IsConnected())
      return nullptr;

    return std::make_unique<QuicHttpStream>(std::move(session));
  }

  bool HasActiveSession(const HostPortPair& host_port_pair) {
    QuicServerId server_id(host_port_pair, PRIVACY_MODE_DISABLED);
    return QuicStreamFactoryPeer::HasActiveSession(factory_.get(), server_id);
  }

  bool HasActiveJob(const HostPortPair& host_port_pair,
                    const PrivacyMode privacy_mode) {
    QuicServerId server_id(host_port_pair, privacy_mode);
    return QuicStreamFactoryPeer::HasActiveJob(factory_.get(), server_id);
  }

  bool HasActiveCertVerifierJob(const QuicServerId& server_id) {
    return QuicStreamFactoryPeer::HasActiveCertVerifierJob(factory_.get(),
                                                           server_id);
  }

  QuicChromiumClientSession* GetActiveSession(
      const HostPortPair& host_port_pair) {
    QuicServerId server_id(host_port_pair, PRIVACY_MODE_DISABLED);
    return QuicStreamFactoryPeer::GetActiveSession(factory_.get(), server_id);
  }

  int GetSourcePortForNewSession(const HostPortPair& destination) {
    return GetSourcePortForNewSessionInner(destination, false);
  }

  int GetSourcePortForNewSessionAndGoAway(const HostPortPair& destination) {
    return GetSourcePortForNewSessionInner(destination, true);
  }

  int GetSourcePortForNewSessionInner(const HostPortPair& destination,
                                      bool goaway_received) {
    // Should only be called if there is no active session for this destination.
    EXPECT_FALSE(HasActiveSession(destination));
    size_t socket_count = socket_factory_->udp_client_socket_ports().size();

    MockQuicData socket_data;
    socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
    socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
    socket_data.AddSocketDataToFactory(socket_factory_.get());

    QuicStreamRequest request(factory_.get());
    GURL url("https://" + destination.host() + "/");
    EXPECT_EQ(ERR_IO_PENDING,
              request.Request(destination, version_, privacy_mode_,
                              DEFAULT_PRIORITY, SocketTag(),
                              /*cert_verify_flags=*/0, url, net_log_,
                              &net_error_details_, callback_.callback()));

    EXPECT_THAT(callback_.WaitForResult(), IsOk());
    std::unique_ptr<HttpStream> stream = CreateStream(&request);
    EXPECT_TRUE(stream.get());
    stream.reset();

    QuicChromiumClientSession* session = GetActiveSession(destination);

    if (socket_count + 1 != socket_factory_->udp_client_socket_ports().size()) {
      ADD_FAILURE();
      return 0;
    }

    if (goaway_received) {
      QuicGoAwayFrame goaway(kInvalidControlFrameId, QUIC_NO_ERROR, 1, "");
      session->connection()->OnGoAwayFrame(goaway);
    }

    factory_->OnSessionClosed(session);
    EXPECT_FALSE(HasActiveSession(destination));
    EXPECT_TRUE(socket_data.AllReadDataConsumed());
    EXPECT_TRUE(socket_data.AllWriteDataConsumed());
    return socket_factory_->udp_client_socket_ports()[socket_count];
  }

  std::unique_ptr<QuicEncryptedPacket> ConstructClientConnectionClosePacket(
      QuicPacketNumber num) {
    return client_maker_.MakeConnectionClosePacket(
        num, false, QUIC_CRYPTO_VERSION_NOT_SUPPORTED, "Time to panic!");
  }

  std::unique_ptr<QuicEncryptedPacket> ConstructClientRstPacket(
      QuicPacketNumber packet_number,
      QuicRstStreamErrorCode error_code) {
    QuicStreamId stream_id = GetNthClientInitiatedStreamId(0);
    return client_maker_.MakeRstPacket(packet_number, true, stream_id,
                                       error_code);
  }

  static ProofVerifyDetailsChromium DefaultProofVerifyDetails() {
    // Load a certificate that is valid for *.example.org
    scoped_refptr<X509Certificate> test_cert(
        ImportCertFromFile(GetTestCertsDirectory(), "wildcard.pem"));
    EXPECT_TRUE(test_cert.get());
    ProofVerifyDetailsChromium verify_details;
    verify_details.cert_verify_result.verified_cert = test_cert;
    verify_details.cert_verify_result.is_issued_by_known_root = true;
    return verify_details;
  }

  void NotifyIPAddressChanged() {
    NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();
    // Spin the message loop so the notification is delivered.
    base::RunLoop().RunUntilIdle();
  }

  std::unique_ptr<QuicEncryptedPacket> ConstructGetRequestPacket(
      QuicPacketNumber packet_number,
      QuicStreamId stream_id,
      bool should_include_version,
      bool fin) {
    spdy::SpdyHeaderBlock headers =
        client_maker_.GetRequestHeaders("GET", "https", "/");
    spdy::SpdyPriority priority =
        ConvertRequestPriorityToQuicPriority(DEFAULT_PRIORITY);
    size_t spdy_headers_frame_len;
    return client_maker_.MakeRequestHeadersPacket(
        packet_number, stream_id, should_include_version, fin, priority,
        std::move(headers), 0, &spdy_headers_frame_len);
  }

  std::unique_ptr<QuicEncryptedPacket> ConstructGetRequestPacket(
      QuicPacketNumber packet_number,
      QuicStreamId stream_id,
      bool should_include_version,
      bool fin,
      QuicStreamOffset* offset) {
    spdy::SpdyHeaderBlock headers =
        client_maker_.GetRequestHeaders("GET", "https", "/");
    spdy::SpdyPriority priority =
        ConvertRequestPriorityToQuicPriority(DEFAULT_PRIORITY);
    size_t spdy_headers_frame_len;
    return client_maker_.MakeRequestHeadersPacket(
        packet_number, stream_id, should_include_version, fin, priority,
        std::move(headers), 0, &spdy_headers_frame_len, offset);
  }

  std::unique_ptr<QuicEncryptedPacket> ConstructOkResponsePacket(
      QuicPacketNumber packet_number,
      QuicStreamId stream_id,
      bool should_include_version,
      bool fin) {
    spdy::SpdyHeaderBlock headers = server_maker_.GetResponseHeaders("200 OK");
    size_t spdy_headers_frame_len;
    return server_maker_.MakeResponseHeadersPacket(
        packet_number, stream_id, should_include_version, fin,
        std::move(headers), &spdy_headers_frame_len);
  }

  std::unique_ptr<QuicReceivedPacket> ConstructInitialSettingsPacket() {
    return client_maker_.MakeInitialSettingsPacket(1, nullptr);
  }

  std::unique_ptr<QuicReceivedPacket> ConstructInitialSettingsPacket(
      QuicPacketNumber packet_number,
      QuicStreamOffset* offset) {
    return client_maker_.MakeInitialSettingsPacket(packet_number, offset);
  }

  // Helper method for server migration tests.
  void VerifyServerMigration(const QuicConfig& config,
                             IPEndPoint expected_address) {
    allow_server_migration_ = true;
    Initialize();

    ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
    crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
    crypto_client_stream_factory_.SetConfig(config);

    // Set up first socket data provider.
    MockQuicData socket_data1;
    socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
    socket_data1.AddSocketDataToFactory(socket_factory_.get());

    // Set up second socket data provider that is used after
    // migration.
    MockQuicData socket_data2;
    socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
    socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
    socket_data2.AddWrite(
        SYNCHRONOUS, client_maker_.MakePingPacket(2, /*include_version=*/true));
    socket_data2.AddWrite(
        SYNCHRONOUS,
        client_maker_.MakeRstPacket(3, true, GetNthClientInitiatedStreamId(0),
                                    QUIC_STREAM_CANCELLED));
    socket_data2.AddSocketDataToFactory(socket_factory_.get());

    // Create request and QuicHttpStream.
    QuicStreamRequest request(factory_.get());
    EXPECT_EQ(ERR_IO_PENDING,
              request.Request(host_port_pair_, version_, privacy_mode_,
                              DEFAULT_PRIORITY, SocketTag(),
                              /*cert_verify_flags=*/0, url_, net_log_,
                              &net_error_details_, callback_.callback()));
    EXPECT_EQ(OK, callback_.WaitForResult());

    // Run QuicChromiumClientSession::WriteToNewSocket()
    // posted by QuicChromiumClientSession::MigrateToSocket().
    base::RunLoop().RunUntilIdle();

    std::unique_ptr<HttpStream> stream = CreateStream(&request);
    EXPECT_TRUE(stream.get());

    // Cause QUIC stream to be created.
    HttpRequestInfo request_info;
    request_info.method = "GET";
    request_info.url = GURL("https://www.example.org/");
    request_info.traffic_annotation =
        MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
    EXPECT_EQ(OK,
              stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                       net_log_, CompletionOnceCallback()));
    // Ensure that session is alive and active.
    QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
    EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
    EXPECT_TRUE(HasActiveSession(host_port_pair_));

    IPEndPoint actual_address;
    session->GetDefaultSocket()->GetPeerAddress(&actual_address);
    EXPECT_EQ(actual_address, expected_address);
    DVLOG(1) << "Socket connected to: " << actual_address.address().ToString()
             << " " << actual_address.port();
    DVLOG(1) << "Expected address: " << expected_address.address().ToString()
             << " " << expected_address.port();

    stream.reset();
    EXPECT_TRUE(socket_data1.AllReadDataConsumed());
    EXPECT_TRUE(socket_data2.AllReadDataConsumed());
    EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
  }

  // Verifies that the QUIC stream factory is initialized correctly.
  void VerifyInitialization() {
    store_server_configs_in_properties_ = true;
    idle_connection_timeout_seconds_ = 500;
    Initialize();
    factory_->set_require_confirmation(false);
    ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
    crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
    crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
    crypto_client_stream_factory_.set_handshake_mode(
        MockCryptoClientStream::ZERO_RTT);
    const QuicConfig* config = QuicStreamFactoryPeer::GetConfig(factory_.get());
    EXPECT_EQ(500, config->IdleNetworkTimeout().ToSeconds());

    QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), runner_.get());

    const AlternativeService alternative_service1(
        kProtoQUIC, host_port_pair_.host(), host_port_pair_.port());
    AlternativeServiceInfoVector alternative_service_info_vector;
    base::Time expiration = base::Time::Now() + base::TimeDelta::FromDays(1);
    alternative_service_info_vector.push_back(
        AlternativeServiceInfo::CreateQuicAlternativeServiceInfo(
            alternative_service1, expiration, {version_}));
    http_server_properties_.SetAlternativeServices(
        url::SchemeHostPort(url_), alternative_service_info_vector);

    HostPortPair host_port_pair2(kServer2HostName, kDefaultServerPort);
    url::SchemeHostPort server2("https", kServer2HostName, kDefaultServerPort);
    const AlternativeService alternative_service2(
        kProtoQUIC, host_port_pair2.host(), host_port_pair2.port());
    AlternativeServiceInfoVector alternative_service_info_vector2;
    alternative_service_info_vector2.push_back(
        AlternativeServiceInfo::CreateQuicAlternativeServiceInfo(
            alternative_service2, expiration, {version_}));

    http_server_properties_.SetAlternativeServices(
        server2, alternative_service_info_vector2);
    // Verify that the properties of both QUIC servers are stored in the
    // HTTP properties map.
    EXPECT_EQ(2U, http_server_properties_.alternative_service_map().size());

    http_server_properties_.SetMaxServerConfigsStoredInProperties(
        kDefaultMaxQuicServerEntries);

    QuicServerId quic_server_id(kDefaultServerHostName, 443,
                                PRIVACY_MODE_DISABLED);
    std::unique_ptr<QuicServerInfo> quic_server_info =
        std::make_unique<PropertiesBasedQuicServerInfo>(
            quic_server_id, &http_server_properties_);

    // Update quic_server_info's server_config and persist it.
    QuicServerInfo::State* state = quic_server_info->mutable_state();
    // Minimum SCFG that passes config validation checks.
    const char scfg[] = {// SCFG
                         0x53, 0x43, 0x46, 0x47,
                         // num entries
                         0x01, 0x00,
                         // padding
                         0x00, 0x00,
                         // EXPY
                         0x45, 0x58, 0x50, 0x59,
                         // EXPY end offset
                         0x08, 0x00, 0x00, 0x00,
                         // Value
                         '1', '2', '3', '4', '5', '6', '7', '8'};

    // Create temporary strings becasue Persist() clears string data in |state|.
    string server_config(reinterpret_cast<const char*>(&scfg), sizeof(scfg));
    string source_address_token("test_source_address_token");
    string cert_sct("test_cert_sct");
    string chlo_hash("test_chlo_hash");
    string signature("test_signature");
    string test_cert("test_cert");
    std::vector<string> certs;
    certs.push_back(test_cert);
    state->server_config = server_config;
    state->source_address_token = source_address_token;
    state->cert_sct = cert_sct;
    state->chlo_hash = chlo_hash;
    state->server_config_sig = signature;
    state->certs = certs;

    quic_server_info->Persist();

    QuicServerId quic_server_id2(kServer2HostName, 443, PRIVACY_MODE_DISABLED);
    std::unique_ptr<QuicServerInfo> quic_server_info2 =
        std::make_unique<PropertiesBasedQuicServerInfo>(
            quic_server_id2, &http_server_properties_);
    // Update quic_server_info2's server_config and persist it.
    QuicServerInfo::State* state2 = quic_server_info2->mutable_state();

    // Minimum SCFG that passes config validation checks.
    const char scfg2[] = {// SCFG
                          0x53, 0x43, 0x46, 0x47,
                          // num entries
                          0x01, 0x00,
                          // padding
                          0x00, 0x00,
                          // EXPY
                          0x45, 0x58, 0x50, 0x59,
                          // EXPY end offset
                          0x08, 0x00, 0x00, 0x00,
                          // Value
                          '8', '7', '3', '4', '5', '6', '2', '1'};

    // Create temporary strings becasue Persist() clears string data in
    // |state2|.
    string server_config2(reinterpret_cast<const char*>(&scfg2), sizeof(scfg2));
    string source_address_token2("test_source_address_token2");
    string cert_sct2("test_cert_sct2");
    string chlo_hash2("test_chlo_hash2");
    string signature2("test_signature2");
    string test_cert2("test_cert2");
    std::vector<string> certs2;
    certs2.push_back(test_cert2);
    state2->server_config = server_config2;
    state2->source_address_token = source_address_token2;
    state2->cert_sct = cert_sct2;
    state2->chlo_hash = chlo_hash2;
    state2->server_config_sig = signature2;
    state2->certs = certs2;

    quic_server_info2->Persist();

    // Verify the MRU order is maintained.
    const QuicServerInfoMap& quic_server_info_map =
        http_server_properties_.quic_server_info_map();
    EXPECT_EQ(2u, quic_server_info_map.size());
    QuicServerInfoMap::const_iterator quic_server_info_map_it =
        quic_server_info_map.begin();
    EXPECT_EQ(quic_server_info_map_it->first, quic_server_id2);
    ++quic_server_info_map_it;
    EXPECT_EQ(quic_server_info_map_it->first, quic_server_id);

    host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                             "192.168.0.1", "");

    // Create a session and verify that the cached state is loaded.
    MockQuicData socket_data;
    socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
    socket_data.AddSocketDataToFactory(socket_factory_.get());

    QuicStreamRequest request(factory_.get());
    EXPECT_EQ(ERR_IO_PENDING,
              request.Request(quic_server_id.host_port_pair(), version_,
                              privacy_mode_, DEFAULT_PRIORITY, SocketTag(),
                              /*cert_verify_flags=*/0, url_, net_log_,
                              &net_error_details_, callback_.callback()));
    EXPECT_THAT(callback_.WaitForResult(), IsOk());

    EXPECT_FALSE(QuicStreamFactoryPeer::CryptoConfigCacheIsEmpty(
        factory_.get(), quic_server_id));
    QuicCryptoClientConfig* crypto_config =
        QuicStreamFactoryPeer::GetCryptoConfig(factory_.get());
    QuicCryptoClientConfig::CachedState* cached =
        crypto_config->LookupOrCreate(quic_server_id);
    EXPECT_FALSE(cached->server_config().empty());
    EXPECT_TRUE(cached->GetServerConfig());
    EXPECT_EQ(server_config, cached->server_config());
    EXPECT_EQ(source_address_token, cached->source_address_token());
    EXPECT_EQ(cert_sct, cached->cert_sct());
    EXPECT_EQ(chlo_hash, cached->chlo_hash());
    EXPECT_EQ(signature, cached->signature());
    ASSERT_EQ(1U, cached->certs().size());
    EXPECT_EQ(test_cert, cached->certs()[0]);

    EXPECT_TRUE(socket_data.AllWriteDataConsumed());

    // Create a session and verify that the cached state is loaded.
    MockQuicData socket_data2;
    socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
    socket_data2.AddSocketDataToFactory(socket_factory_.get());

    host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                             "192.168.0.2", "");

    QuicStreamRequest request2(factory_.get());
    EXPECT_EQ(ERR_IO_PENDING,
              request2.Request(quic_server_id2.host_port_pair(), version_,
                               privacy_mode_, DEFAULT_PRIORITY, SocketTag(),
                               /*cert_verify_flags=*/0,
                               GURL("https://mail.example.org/"), net_log_,
                               &net_error_details_, callback_.callback()));
    EXPECT_THAT(callback_.WaitForResult(), IsOk());

    EXPECT_FALSE(QuicStreamFactoryPeer::CryptoConfigCacheIsEmpty(
        factory_.get(), quic_server_id2));
    QuicCryptoClientConfig::CachedState* cached2 =
        crypto_config->LookupOrCreate(quic_server_id2);
    EXPECT_FALSE(cached2->server_config().empty());
    EXPECT_TRUE(cached2->GetServerConfig());
    EXPECT_EQ(server_config2, cached2->server_config());
    EXPECT_EQ(source_address_token2, cached2->source_address_token());
    EXPECT_EQ(cert_sct2, cached2->cert_sct());
    EXPECT_EQ(chlo_hash2, cached2->chlo_hash());
    EXPECT_EQ(signature2, cached2->signature());
    ASSERT_EQ(1U, cached->certs().size());
    EXPECT_EQ(test_cert2, cached2->certs()[0]);
  }

  void RunTestLoopUntilIdle() {
    while (!runner_->GetPostedTasks().empty())
      runner_->RunNextTask();
  }

  QuicStreamId GetNthClientInitiatedStreamId(int n) {
    return test::GetNthClientInitiatedStreamId(version_, n);
  }

  QuicStreamId GetNthServerInitiatedStreamId(int n) {
    return test::GetNthServerInitiatedStreamId(version_, n);
  }

  // Helper methods for tests of connection migration on write error.
  void TestMigrationOnWriteErrorNonMigratableStream(IoMode write_error_mode);
  void TestMigrationOnWriteErrorMigrationDisabled(IoMode write_error_mode);
  void TestMigrationOnWriteError(IoMode write_error_mode);
  void TestMigrationOnWriteErrorNoNewNetwork(IoMode write_error_mode);
  void TestMigrationOnMultipleWriteErrors(IoMode first_write_error_mode,
                                          IoMode second_write_error_mode);
  void TestMigrationOnWriteErrorWithNotificationQueued(bool disconnected);
  void TestMigrationOnNotificationWithWriteErrorQueued(bool disconnected);
  void OnNetworkDisconnected(bool async_write_before);
  void OnNetworkMadeDefault(bool async_write_before);
  void TestMigrationOnWriteErrorPauseBeforeConnected(IoMode write_error_mode);
  void OnNetworkDisconnectedWithNetworkList(
      NetworkChangeNotifier::NetworkList network_list);
  void TestMigrationOnWriteErrorWithNetworkAddedBeforeNotification(
      IoMode write_error_mode,
      bool disconnected);

  QuicFlagSaver flags_;  // Save/restore all QUIC flag values.
  MockHostResolver host_resolver_;
  scoped_refptr<SSLConfigService> ssl_config_service_;
  std::unique_ptr<MockClientSocketFactory> socket_factory_;
  MockCryptoClientStreamFactory crypto_client_stream_factory_;
  MockRandom random_generator_;
  MockClock clock_;
  scoped_refptr<TestTaskRunner> runner_;
  const QuicTransportVersion version_;
  const bool client_headers_include_h2_stream_dependency_;
  QuicTestPacketMaker client_maker_;
  QuicTestPacketMaker server_maker_;
  HttpServerPropertiesImpl http_server_properties_;
  std::unique_ptr<CertVerifier> cert_verifier_;
  std::unique_ptr<ChannelIDService> channel_id_service_;
  TransportSecurityState transport_security_state_;
  std::unique_ptr<CTVerifier> cert_transparency_verifier_;
  DefaultCTPolicyEnforcer ct_policy_enforcer_;
  std::unique_ptr<ScopedMockNetworkChangeNotifier>
      scoped_mock_network_change_notifier_;
  std::unique_ptr<QuicStreamFactory> factory_;
  HostPortPair host_port_pair_;
  GURL url_;
  GURL url2_;
  GURL url3_;
  GURL url4_;

  PrivacyMode privacy_mode_;
  NetLogWithSource net_log_;
  TestCompletionCallback callback_;
  NetErrorDetails net_error_details_;

  // Variables to configure QuicStreamFactory.
  bool store_server_configs_in_properties_;
  bool close_sessions_on_ip_change_;
  int idle_connection_timeout_seconds_;
  int reduced_ping_timeout_seconds_;
  int max_time_before_crypto_handshake_seconds_;
  int max_idle_time_before_crypto_handshake_seconds_;
  bool migrate_sessions_on_network_change_;
  bool migrate_sessions_early_;
  bool migrate_sessions_on_network_change_v2_;
  bool migrate_sessions_early_v2_;
  bool allow_server_migration_;
  bool race_cert_verification_;
  bool estimate_initial_rtt_;
  QuicTagVector connection_options_;
  QuicTagVector client_connection_options_;
};

class QuicStreamFactoryTest : public QuicStreamFactoryTestBase,
                              public ::testing::TestWithParam<TestParams> {
 protected:
  QuicStreamFactoryTest()
      : QuicStreamFactoryTestBase(
            GetParam().version,
            GetParam().client_headers_include_h2_stream_dependency) {}
};

INSTANTIATE_TEST_CASE_P(VersionIncludeStreamDependencySequence,
                        QuicStreamFactoryTest,
                        ::testing::ValuesIn(GetTestParams()));

TEST_P(QuicStreamFactoryTest, Create) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  EXPECT_EQ(DEFAULT_PRIORITY, host_resolver_.last_request_priority());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(host_port_pair_, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback_.callback()));
  // Will reset stream 3.
  stream = CreateStream(&request2);

  EXPECT_TRUE(stream.get());

  // TODO(rtenneti): We should probably have a tests that HTTP and HTTPS result
  // in streams on different sessions.
  QuicStreamRequest request3(factory_.get());
  EXPECT_EQ(OK, request3.Request(host_port_pair_, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback_.callback()));
  stream = CreateStream(&request3);  // Will reset stream 5.
  stream.reset();                    // Will reset stream 7.

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, CreateZeroRtt) {
  Initialize();
  factory_->set_require_confirmation(false);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, DefaultInitialRtt) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(session->require_confirmation());
  EXPECT_EQ(100000u, session->connection()->GetStats().srtt_us);
  ASSERT_FALSE(session->config()->HasInitialRoundTripTimeUsToSend());
}

TEST_P(QuicStreamFactoryTest, FactoryDestroyedWhenJobPending) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  auto request = std::make_unique<QuicStreamRequest>(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request->Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  request.reset();
  EXPECT_TRUE(HasActiveJob(host_port_pair_, privacy_mode_));
  // Tearing down a QuicStreamFactory with a pending Job should not cause any
  // crash. crbug.com/768343.
  factory_.reset();
}

TEST_P(QuicStreamFactoryTest, RequireConfirmation) {
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");
  Initialize();
  factory_->set_require_confirmation(true);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  IPAddress last_address;
  EXPECT_FALSE(http_server_properties_.GetSupportsQuic(&last_address));

  crypto_client_stream_factory_.last_stream()->SendOnCryptoHandshakeEvent(
      QuicSession::HANDSHAKE_CONFIRMED);

  EXPECT_TRUE(http_server_properties_.GetSupportsQuic(&last_address));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(session->require_confirmation());
}

TEST_P(QuicStreamFactoryTest, DontRequireConfirmationFromSameIP) {
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");
  Initialize();
  factory_->set_require_confirmation(true);
  http_server_properties_.SetSupportsQuic(IPAddress(192, 0, 2, 33));

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_THAT(request.Request(host_port_pair_, version_, privacy_mode_,
                              DEFAULT_PRIORITY, SocketTag(),
                              /*cert_verify_flags=*/0, url_, net_log_,
                              &net_error_details_, callback_.callback()),
              IsOk());

  IPAddress last_address;
  EXPECT_FALSE(http_server_properties_.GetSupportsQuic(&last_address));

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_FALSE(session->require_confirmation());

  crypto_client_stream_factory_.last_stream()->SendOnCryptoHandshakeEvent(
      QuicSession::HANDSHAKE_CONFIRMED);

  EXPECT_TRUE(http_server_properties_.GetSupportsQuic(&last_address));
}

TEST_P(QuicStreamFactoryTest, CachedInitialRtt) {
  ServerNetworkStats stats;
  stats.srtt = base::TimeDelta::FromMilliseconds(10);
  http_server_properties_.SetServerNetworkStats(url::SchemeHostPort(url_),
                                                stats);
  estimate_initial_rtt_ = true;

  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_EQ(10000u, session->connection()->GetStats().srtt_us);
  ASSERT_TRUE(session->config()->HasInitialRoundTripTimeUsToSend());
  EXPECT_EQ(10000u, session->config()->GetInitialRoundTripTimeUsToSend());
}

TEST_P(QuicStreamFactoryTest, 2gInitialRtt) {
  ScopedMockNetworkChangeNotifier notifier;
  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_2G);
  estimate_initial_rtt_ = true;

  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_EQ(1200000u, session->connection()->GetStats().srtt_us);
  ASSERT_TRUE(session->config()->HasInitialRoundTripTimeUsToSend());
  EXPECT_EQ(1200000u, session->config()->GetInitialRoundTripTimeUsToSend());
}

TEST_P(QuicStreamFactoryTest, 3gInitialRtt) {
  ScopedMockNetworkChangeNotifier notifier;
  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_3G);
  estimate_initial_rtt_ = true;

  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_EQ(400000u, session->connection()->GetStats().srtt_us);
  ASSERT_TRUE(session->config()->HasInitialRoundTripTimeUsToSend());
  EXPECT_EQ(400000u, session->config()->GetInitialRoundTripTimeUsToSend());
}

TEST_P(QuicStreamFactoryTest, GoAway) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);

  session->OnGoAway(QuicGoAwayFrame());

  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, GoAwayForConnectionMigrationWithPortOnly) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);

  session->OnGoAway(
      QuicGoAwayFrame(kInvalidControlFrameId, QUIC_ERROR_MIGRATING_PORT, 0,
                      "peer connection migration due to port change only"));
  NetErrorDetails details;
  EXPECT_FALSE(details.quic_port_migration_detected);
  session->PopulateNetErrorDetails(&details);
  EXPECT_TRUE(details.quic_port_migration_detected);
  details.quic_port_migration_detected = false;
  stream->PopulateNetErrorDetails(&details);
  EXPECT_TRUE(details.quic_port_migration_detected);

  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, Pooling) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server2(kServer2HostName, kDefaultServerPort);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  TestCompletionCallback callback;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_EQ(GetActiveSession(host_port_pair_), GetActiveSession(server2));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, PoolingWithServerMigration) {
  // Set up session to migrate.
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");
  IPEndPoint alt_address = IPEndPoint(IPAddress(1, 2, 3, 4), 443);
  QuicConfig config;
  config.SetAlternateServerAddressToSend(
      QuicSocketAddress(QuicSocketAddressImpl(alt_address)));

  VerifyServerMigration(config, alt_address);

  // Close server-migrated session.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  session->CloseSessionOnError(0u, QUIC_NO_ERROR);

  // Set up server IP, socket, proof, and config for new session.
  HostPortPair server2(kServer2HostName, kDefaultServerPort);
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  std::unique_ptr<QuicEncryptedPacket> settings_packet(
      client_maker_.MakeInitialSettingsPacket(1, nullptr));
  MockWrite writes[] = {MockWrite(SYNCHRONOUS, settings_packet->data(),
                                  settings_packet->length(), 1)};

  SequencedSocketData socket_data(reads, writes);
  socket_factory_->AddSocketDataProvider(&socket_data);

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  QuicConfig config2;
  crypto_client_stream_factory_.SetConfig(config2);

  // Create new request to cause new session creation.
  TestCompletionCallback callback;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(server2, version_, privacy_mode_, DEFAULT_PRIORITY,
                             SocketTag(),
                             /*cert_verify_flags=*/0, url2_, net_log_,
                             &net_error_details_, callback.callback()));
  EXPECT_EQ(OK, callback.WaitForResult());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  // EXPECT_EQ(GetActiveSession(host_port_pair_), GetActiveSession(server2));
}

TEST_P(QuicStreamFactoryTest, NoPoolingAfterGoAway) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data1;
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data1.AddSocketDataToFactory(socket_factory_.get());
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server2(kServer2HostName, kDefaultServerPort);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  TestCompletionCallback callback;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  factory_->OnSessionGoingAway(GetActiveSession(host_port_pair_));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_FALSE(HasActiveSession(server2));

  TestCompletionCallback callback3;
  QuicStreamRequest request3(factory_.get());
  EXPECT_EQ(OK, request3.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback3.callback()));
  std::unique_ptr<HttpStream> stream3 = CreateStream(&request3);
  EXPECT_TRUE(stream3.get());

  EXPECT_TRUE(HasActiveSession(server2));

  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, HttpsPooling) {
  Initialize();

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server1(kDefaultServerHostName, 443);
  HostPortPair server2(kServer2HostName, 443);

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(server1.host(), "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(server1, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  TestCompletionCallback callback;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_EQ(GetActiveSession(server1), GetActiveSession(server2));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, HttpsPoolingWithMatchingPins) {
  Initialize();
  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server1(kDefaultServerHostName, 443);
  HostPortPair server2(kServer2HostName, 443);
  uint8_t primary_pin = 1;
  uint8_t backup_pin = 2;
  test::AddPin(&transport_security_state_, kServer2HostName, primary_pin,
               backup_pin);

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  verify_details.cert_verify_result.public_key_hashes.push_back(
      test::GetTestHashValue(primary_pin));
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(server1.host(), "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(server1, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  TestCompletionCallback callback;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_EQ(GetActiveSession(server1), GetActiveSession(server2));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, NoHttpsPoolingWithDifferentPins) {
  Initialize();

  MockQuicData socket_data1;
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data1.AddSocketDataToFactory(socket_factory_.get());
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server1(kDefaultServerHostName, 443);
  HostPortPair server2(kServer2HostName, 443);
  uint8_t primary_pin = 1;
  uint8_t backup_pin = 2;
  uint8_t bad_pin = 3;
  test::AddPin(&transport_security_state_, kServer2HostName, primary_pin,
               backup_pin);

  ProofVerifyDetailsChromium verify_details1 = DefaultProofVerifyDetails();
  verify_details1.cert_verify_result.public_key_hashes.push_back(
      test::GetTestHashValue(bad_pin));
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details1);

  ProofVerifyDetailsChromium verify_details2 = DefaultProofVerifyDetails();
  verify_details2.cert_verify_result.public_key_hashes.push_back(
      test::GetTestHashValue(primary_pin));
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details2);

  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(server1.host(), "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(server1, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  TestCompletionCallback callback;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_NE(GetActiveSession(server1), GetActiveSession(server2));

  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, Goaway) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Mark the session as going away.  Ensure that while it is still alive
  // that it is no longer active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  factory_->OnSessionGoingAway(session);
  EXPECT_EQ(true,
            QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  // Create a new request for the same destination and verify that a
  // new session is created.
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_NE(session, GetActiveSession(host_port_pair_));
  EXPECT_EQ(true,
            QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));

  stream2.reset();
  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MaxOpenStream) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  QuicStreamId stream_id = GetNthClientInitiatedStreamId(0);
  MockQuicData socket_data;
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, stream_id, QUIC_STREAM_CANCELLED));
  socket_data.AddRead(ASYNC, server_maker_.MakeRstPacket(
                                 1, false, stream_id, QUIC_STREAM_CANCELLED));
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<std::unique_ptr<HttpStream>> streams;
  // The MockCryptoClientStream sets max_open_streams to be
  // kDefaultMaxStreamsPerConnection / 2.
  for (size_t i = 0; i < kDefaultMaxStreamsPerConnection / 2; i++) {
    QuicStreamRequest request(factory_.get());
    int rv = request.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback());
    if (i == 0) {
      EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
      EXPECT_THAT(callback_.WaitForResult(), IsOk());
    } else {
      EXPECT_THAT(rv, IsOk());
    }
    std::unique_ptr<HttpStream> stream = CreateStream(&request);
    EXPECT_TRUE(stream);
    EXPECT_EQ(OK,
              stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                       net_log_, CompletionOnceCallback()));
    streams.push_back(std::move(stream));
  }

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, CompletionCallback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream);
  EXPECT_EQ(ERR_IO_PENDING,
            stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                     net_log_, callback_.callback()));

  // Close the first stream.
  streams.front()->Close(false);
  // Trigger exchange of RSTs that in turn allow progress for the last
  // stream.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());

  // Force close of the connection to suppress the generation of RST
  // packets when streams are torn down, which wouldn't be relevant to
  // this test anyway.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  session->connection()->CloseConnection(QUIC_PUBLIC_RESET, "test",
                                         ConnectionCloseBehavior::SILENT_CLOSE);
}

TEST_P(QuicStreamFactoryTest, ResolutionErrorInCreate) {
  Initialize();
  MockQuicData socket_data;
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  host_resolver_.rules()->AddSimulatedFailure(kDefaultServerHostName);

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, ConnectErrorInCreate) {
  Initialize();

  MockQuicData socket_data;
  socket_data.AddConnect(SYNCHRONOUS, ERR_ADDRESS_IN_USE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsError(ERR_ADDRESS_IN_USE));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, CancelCreate) {
  Initialize();
  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());
  {
    QuicStreamRequest request(factory_.get());
    EXPECT_EQ(ERR_IO_PENDING,
              request.Request(host_port_pair_, version_, privacy_mode_,
                              DEFAULT_PRIORITY, SocketTag(),
                              /*cert_verify_flags=*/0, url_, net_log_,
                              &net_error_details_, callback_.callback()));
  }

  base::RunLoop().RunUntilIdle();

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(host_port_pair_, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream = CreateStream(&request2);

  EXPECT_TRUE(stream.get());
  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, CloseAllSessions) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(SYNCHRONOUS,
                       ConstructClientRstPacket(2, QUIC_RST_ACKNOWLEDGEMENT));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Close the session and verify that stream saw the error.
  factory_->CloseAllSessions(ERR_INTERNET_DISCONNECTED, QUIC_INTERNAL_ERROR);
  EXPECT_EQ(ERR_INTERNET_DISCONNECTED,
            stream->ReadResponseHeaders(callback_.callback()));

  // Now attempting to request a stream to the same origin should create
  // a new session.

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  stream = CreateStream(&request2);
  stream.reset();  // Will reset stream 3.

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

// Regression test for crbug.com/700617. Test a write error during the
// crypto handshake will not hang QuicStreamFactory::Job and should
// report QUIC_HANDSHAKE_FAILED to upper layers. Subsequent
// QuicStreamRequest should succeed without hanging.
TEST_P(QuicStreamFactoryTest,
       WriteErrorInCryptoConnectWithAsyncHostResolution) {
  Initialize();
  // Use unmocked crypto stream to do crypto connect.
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::USE_DEFAULT_CRYPTO_STREAM);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  // Trigger PACKET_WRITE_ERROR when sending packets in crypto connect.
  socket_data.AddWrite(SYNCHRONOUS, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request, should fail after the write of the CHLO fails.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(ERR_QUIC_HANDSHAKE_FAILED, callback_.WaitForResult());
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_FALSE(HasActiveJob(host_port_pair_, privacy_mode_));

  // Verify new requests can be sent normally without hanging.
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::COLD_START);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_TRUE(HasActiveJob(host_port_pair_, privacy_mode_));
  // Run the message loop to complete host resolution.
  base::RunLoop().RunUntilIdle();

  // Complete handshake. QuicStreamFactory::Job should complete and succeed.
  crypto_client_stream_factory_.last_stream()->SendOnCryptoHandshakeEvent(
      QuicSession::HANDSHAKE_CONFIRMED);
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_FALSE(HasActiveJob(host_port_pair_, privacy_mode_));

  // Create QuicHttpStream.
  std::unique_ptr<HttpStream> stream = CreateStream(&request2);
  EXPECT_TRUE(stream.get());
  stream.reset();
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, WriteErrorInCryptoConnectWithSyncHostResolution) {
  Initialize();
  // Use unmocked crypto stream to do crypto connect.
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::USE_DEFAULT_CRYPTO_STREAM);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  // Trigger PACKET_WRITE_ERROR when sending packets in crypto connect.
  socket_data.AddWrite(SYNCHRONOUS, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request, should fail immediately.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_QUIC_HANDSHAKE_FAILED,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  // Check no active session, or active jobs left for this server.
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_FALSE(HasActiveJob(host_port_pair_, privacy_mode_));

  // Verify new requests can be sent normally without hanging.
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::COLD_START);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_TRUE(HasActiveJob(host_port_pair_, privacy_mode_));

  // Complete handshake.
  crypto_client_stream_factory_.last_stream()->SendOnCryptoHandshakeEvent(
      QuicSession::HANDSHAKE_CONFIRMED);
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_FALSE(HasActiveJob(host_port_pair_, privacy_mode_));

  // Create QuicHttpStream.
  std::unique_ptr<HttpStream> stream = CreateStream(&request2);
  EXPECT_TRUE(stream.get());
  stream.reset();
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnIPAddressChanged) {
  close_sessions_on_ip_change_ = true;
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(SYNCHRONOUS,
                       ConstructClientRstPacket(2, QUIC_RST_ACKNOWLEDGEMENT));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  IPAddress last_address;
  EXPECT_TRUE(http_server_properties_.GetSupportsQuic(&last_address));
  // Change the IP address and verify that stream saw the error.
  NotifyIPAddressChanged();
  EXPECT_EQ(ERR_NETWORK_CHANGED,
            stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_TRUE(factory_->require_confirmation());
  EXPECT_FALSE(http_server_properties_.GetSupportsQuic(&last_address));

  // Now attempting to request a stream to the same origin should create
  // a new session.

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  stream = CreateStream(&request2);
  stream.reset();  // Will reset stream 3.

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnIPAddressChangedWithConnectionMigration) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(SYNCHRONOUS,
                       ConstructClientRstPacket(2, QUIC_STREAM_CANCELLED));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  IPAddress last_address;
  EXPECT_TRUE(http_server_properties_.GetSupportsQuic(&last_address));

  // Change the IP address and verify that the connection is unaffected.
  NotifyIPAddressChanged();
  EXPECT_FALSE(factory_->require_confirmation());
  EXPECT_TRUE(http_server_properties_.GetSupportsQuic(&last_address));

  // Attempting a new request to the same origin uses the same connection.
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(host_port_pair_, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback_.callback()));
  stream = CreateStream(&request2);

  stream.reset();
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkMadeDefaultWithSynchronousWriteBefore) {
  OnNetworkMadeDefault(/*async_write_before=*/false);
}

TEST_P(QuicStreamFactoryTest, OnNetworkMadeDefaultWithAsyncWriteBefore) {
  OnNetworkMadeDefault(/*async_write_before=*/true);
}

void QuicStreamFactoryTestBase::OnNetworkMadeDefault(bool async_write_before) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  int packet_number = 1;
  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS,
      ConstructInitialSettingsPacket(packet_number++, &header_stream_offset));
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructGetRequestPacket(
                       packet_number++, GetNthClientInitiatedStreamId(0), true,
                       true, &header_stream_offset));
  if (async_write_before) {
    socket_data.AddWrite(ASYNC, OK);
    packet_number++;
  }
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Do an async write to leave writer blocked.
  if (async_write_before) {
    session->SendPing();
  }

  // Set up second socket data provider that is used after migration.
  // The response to the earlier request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakePingPacket(packet_number++, /*include_version=*/true));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(
      SYNCHRONOUS, client_maker_.MakeAckAndRstPacket(
                       packet_number++, false, GetNthClientInitiatedStreamId(0),
                       QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Trigger connection migration. This should cause a PING frame
  // to be emitted.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_THAT(stream->ReadResponseHeaders(callback_.callback()), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Create a new request for the same destination and verify that a
  // new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  QuicChromiumClientSession* new_session = GetActiveSession(host_port_pair_);
  EXPECT_NE(session, new_session);

  // On a DISCONNECTED notification, nothing happens to the migrated
  // session, but the new session is closed since it has no open
  // streams.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_FALSE(
      QuicStreamFactoryPeer::IsLiveSession(factory_.get(), new_session));
  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkDisconnectedWithSynchronousWriteBefore) {
  OnNetworkDisconnected(/*async_write_before=*/false);
}

TEST_P(QuicStreamFactoryTest, OnNetworkDisconnectedWithAsyncWriteBefore) {
  OnNetworkDisconnected(/*async_write_before=*/true);
}

void QuicStreamFactoryTestBase::OnNetworkDisconnected(bool async_write_before) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  int packet_number = 1;
  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS,
      ConstructInitialSettingsPacket(packet_number++, &header_stream_offset));
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructGetRequestPacket(
                       packet_number++, GetNthClientInitiatedStreamId(0), true,
                       true, &header_stream_offset));
  if (async_write_before) {
    socket_data.AddWrite(ASYNC, OK);
    packet_number++;
  }
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response_info;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response_info,
                                    callback_.callback()));

  // Do an async write to leave writer blocked.
  if (async_write_before) {
    session->SendPing();
  }

  // Set up second socket data provider that is used after migration.
  // The response to the earlier request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakePingPacket(packet_number++, /*include_version=*/true));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(
      SYNCHRONOUS, client_maker_.MakeAckAndRstPacket(
                       packet_number++, false, GetNthClientInitiatedStreamId(0),
                       QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Trigger connection migration. This should cause a PING frame
  // to be emitted.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Create a new request for the same destination and verify that a
  // new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_NE(session, GetActiveSession(host_port_pair_));
  EXPECT_EQ(true,
            QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkDisconnectedNoNetworks) {
  NetworkChangeNotifier::NetworkList no_networks(0);
  OnNetworkDisconnectedWithNetworkList(no_networks);
}

TEST_P(QuicStreamFactoryTest, OnNetworkDisconnectedNoNewNetwork) {
  OnNetworkDisconnectedWithNetworkList({kDefaultNetworkForTests});
}

void QuicStreamFactoryTestBase::OnNetworkDisconnectedWithNetworkList(
    NetworkChangeNotifier::NetworkList network_list) {
  InitializeConnectionMigrationTest(network_list);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Use the test task runner, to force the migration alarm timeout later.
  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), runner_.get());

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration. Since there are no networks
  // to migrate to, this should cause the session to wait for a new network.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  // The migration will not fail until the migration alarm timeout.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_EQ(true, session->connection()->writer()->IsWriteBlocked());

  // Force the migration alarm timeout to run.
  RunTestLoopUntilIdle();

  // The connection should now be closed. A request for response
  // headers should fail.
  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(ERR_NETWORK_CHANGED, callback_.WaitForResult());

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkMadeDefaultNonMigratableStream) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created, but marked as non-migratable.
  HttpRequestInfo request_info;
  request_info.load_flags |= LOAD_DISABLE_CONNECTION_MIGRATION;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration. Since there is a non-migratable stream,
  // this should cause session to continue but be marked as going away.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkMadeDefaultConnectionMigrationDisabled) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set session config to have connection migration disabled.
  QuicConfigPeer::SetReceivedDisableConnectionMigration(session->config());
  EXPECT_TRUE(session->config()->DisableConnectionMigration());

  // Trigger connection migration. Since there is a non-migratable stream,
  // this should cause session to continue but be marked as going away.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkDisconnectedNonMigratableStream) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_RST_ACKNOWLEDGEMENT));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created, but marked as non-migratable.
  HttpRequestInfo request_info;
  request_info.load_flags |= LOAD_DISABLE_CONNECTION_MIGRATION;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration. Since there is a non-migratable stream,
  // this should cause a RST_STREAM frame to be emitted with
  // QUIC_RST_ACKNOWLEDGEMENT error code, and the session will be closed.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       OnNetworkDisconnectedConnectionMigrationDisabled) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_RST_ACKNOWLEDGEMENT));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set session config to have connection migration disabled.
  QuicConfigPeer::SetReceivedDisableConnectionMigration(session->config());
  EXPECT_TRUE(session->config()->DisableConnectionMigration());

  // Trigger connection migration. Since there is a non-migratable stream,
  // this should cause a RST_STREAM frame to be emitted with
  // QUIC_RST_ACKNOWLEDGEMENT error code, and the session will be closed.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkMadeDefaultNoOpenStreams) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkDisconnectedNoOpenStreams) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration. Since there are no active streams,
  // the session will be closed.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

// This test receives NCN signals in the following order:
// - default network disconnected
// - after a pause, new network is connected.
// - new network is made default.
TEST_P(QuicStreamFactoryTest, NewNetworkConnectedAfterNoNetwork) {
  InitializeConnectionMigrationV2Test({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Use the test task runner.
  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), runner_.get());

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                        2, GetNthClientInitiatedStreamId(0),
                                        true, true, &header_stream_offset));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Trigger connection migration. Since there are no networks
  // to migrate to, this should cause the session to wait for a new network.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  // The connection should still be alive, not marked as going away.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Set up second socket data provider that is used after migration.
  // The response to the earlier request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(
      SYNCHRONOUS, client_maker_.MakePingPacket(3, /*include_version=*/true));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            4, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Add a new network and notify the stream factory of a new connected network.
  // This causes a PING packet to be sent over the new network.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->SetConnectedNetworksList({kNewNetworkForTests});
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkConnected(kNewNetworkForTests);

  // Ensure that the session is still alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  runner_->RunNextTask();

  // Response headers are received over the new network.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Check that the session is still alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // There should posted tasks not executed, which is to migrate back to default
  // network.
  EXPECT_FALSE(runner_->GetPostedTasks().empty());

  // Receive signal to mark new network as default.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  stream.reset();
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
}

// This test verifies that the connection migrates to the alternate network
// early when path degrading is detected.
TEST_P(QuicStreamFactoryTest, MigrateEarlyOnPathDegrading) {
  InitializeConnectionMigrationV2Test(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Using a testing task runner so that we can control time.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), task_runner.get());

  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->QueueNetworkMadeDefault(kDefaultNetworkForTests);

  MockQuicData quic_data1;
  QuicStreamOffset header_stream_offset = 0;
  quic_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);  // Hanging Read.
  quic_data1.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  quic_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
      2, GetNthClientInitiatedStreamId(0), true, true, &header_stream_offset));
  quic_data1.AddSocketDataToFactory(socket_factory_.get());

  // Set up the second socket data provider that is used after migration.
  // The response to the earlier request is read on the new socket.
  MockQuicData quic_data2;
  // Connectivity probe to be sent on the new path.
  quic_data2.AddWrite(SYNCHRONOUS,
      client_maker_.MakeConnectivityProbingPacket(3, true, 1338));
  quic_data2.AddRead(ASYNC, ERR_IO_PENDING);  // Pause
  // Connectivity probe to receive from the server.
  quic_data2.AddRead(ASYNC,
      server_maker_.MakeConnectivityProbingPacket(1, false, 1338));
  // Ping packet to send after migration is completed.
  quic_data2.AddWrite(ASYNC,
      client_maker_.MakeAckAndPingPacket(4, false, 1, 1, 1));
  quic_data2.AddRead(ASYNC, ConstructOkResponsePacket(
      2, GetNthClientInitiatedStreamId(0), false, false));
  quic_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  quic_data2.AddWrite(SYNCHRONOUS, client_maker_.MakeAckAndRstPacket(
      5, false, GetNthClientInitiatedStreamId(0), QUIC_STREAM_CANCELLED, 2, 2,
      1, true));
  quic_data2.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Cause the connection to report path degrading to the session.
  // Session will start to probe the alternate network.
  session->connection()->OnPathDegradingTimeout();

  // Next connectivity probe is scheduled to be sent in 2 *
  // kDefaultRTTMilliSecs.
  EXPECT_EQ(1u, task_runner->GetPendingTaskCount());
  base::TimeDelta next_task_delay = task_runner->NextPendingTaskDelay();
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(2 * kDefaultRTTMilliSecs),
            next_task_delay);

  // The connection should still be alive, and not marked as going away.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Resume quic data and a connectivity probe response will be read on the new
  // socket.
  quic_data2.Resume();

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // There should be three pending tasks, the nearest one will complete
  // migration to the new network.
  EXPECT_EQ(3u, task_runner->GetPendingTaskCount());
  next_task_delay = task_runner->NextPendingTaskDelay();
  EXPECT_EQ(base::TimeDelta(), next_task_delay);
  task_runner->FastForwardBy(next_task_delay);

  // Response headers are received over the new network.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Now there are two pending tasks, the nearest one was to send connectivity
  // probe and has been cancelled due to successful migration.
  EXPECT_EQ(2u, task_runner->GetPendingTaskCount());
  next_task_delay = task_runner->NextPendingTaskDelay();
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(2 * kDefaultRTTMilliSecs),
            next_task_delay);
  task_runner->FastForwardBy(next_task_delay);

  // There's one more task to mgirate back to the default network in 0.4s.
  EXPECT_EQ(1u, task_runner->GetPendingTaskCount());
  next_task_delay = task_runner->NextPendingTaskDelay();
  base::TimeDelta expected_delay =
      base::TimeDelta::FromSeconds(kMinRetryTimeForDefaultNetworkSecs) -
      base::TimeDelta::FromMilliseconds(2 * kDefaultRTTMilliSecs);
  EXPECT_EQ(expected_delay, next_task_delay);

  // Deliver a signal that the alternate network now becomes default to session,
  // this will cancel mgirate back to default network timer.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  task_runner->FastForwardBy(next_task_delay);
  EXPECT_EQ(0u, task_runner->GetPendingTaskCount());

  // Verify that the session is still alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  stream.reset();
  EXPECT_TRUE(quic_data1.AllReadDataConsumed());
  EXPECT_TRUE(quic_data1.AllWriteDataConsumed());
  EXPECT_TRUE(quic_data2.AllReadDataConsumed());
  EXPECT_TRUE(quic_data2.AllWriteDataConsumed());
}

// Regression test for http://crbug.com/835444.
// This test verifies that the connection migrates to the alternate network
// when the alternate network is connected after path has been degrading.
TEST_P(QuicStreamFactoryTest, MigrateOnNewNetworkConnectAfterPathDegrading) {
  InitializeConnectionMigrationV2Test({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Using a testing task runner so that we can control time.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), task_runner.get());

  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->QueueNetworkMadeDefault(kDefaultNetworkForTests);

  MockQuicData quic_data1;
  QuicStreamOffset header_stream_offset = 0;
  quic_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);  // Hanging Read.
  quic_data1.AddWrite(SYNCHRONOUS,
                      ConstructInitialSettingsPacket(1, &header_stream_offset));
  quic_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                       2, GetNthClientInitiatedStreamId(0),
                                       true, true, &header_stream_offset));
  quic_data1.AddSocketDataToFactory(socket_factory_.get());

  // Set up the second socket data provider that is used after migration.
  // The response to the earlier request is read on the new socket.
  MockQuicData quic_data2;
  // Connectivity probe to be sent on the new path.
  quic_data2.AddWrite(
      SYNCHRONOUS, client_maker_.MakeConnectivityProbingPacket(3, true, 1338));
  quic_data2.AddRead(ASYNC, ERR_IO_PENDING);  // Pause
  // Connectivity probe to receive from the server.
  quic_data2.AddRead(
      ASYNC, server_maker_.MakeConnectivityProbingPacket(1, false, 1338));
  // Ping packet to send after migration is completed.
  quic_data2.AddWrite(ASYNC,
                      client_maker_.MakeAckAndPingPacket(4, false, 1, 1, 1));
  quic_data2.AddRead(
      ASYNC, ConstructOkResponsePacket(2, GetNthClientInitiatedStreamId(0),
                                       false, false));
  quic_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  quic_data2.AddWrite(SYNCHRONOUS,
                      client_maker_.MakeAckAndRstPacket(
                          5, false, GetNthClientInitiatedStreamId(0),
                          QUIC_STREAM_CANCELLED, 2, 2, 1, true));
  quic_data2.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Cause the connection to report path degrading to the session.
  // Due to lack of alternate network, session will not mgirate connection.
  EXPECT_EQ(0u, task_runner->GetPendingTaskCount());
  session->connection()->OnPathDegradingTimeout();
  EXPECT_EQ(0u, task_runner->GetPendingTaskCount());

  // Deliver a signal that a alternate network is connected now, this should
  // cause the connection to start early migration on path degrading.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->SetConnectedNetworksList(
          {kDefaultNetworkForTests, kNewNetworkForTests});
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkConnected(kNewNetworkForTests);

  // Next connectivity probe is scheduled to be sent in 2 *
  // kDefaultRTTMilliSecs.
  EXPECT_EQ(1u, task_runner->GetPendingTaskCount());
  base::TimeDelta next_task_delay = task_runner->NextPendingTaskDelay();
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(2 * kDefaultRTTMilliSecs),
            next_task_delay);

  // The connection should still be alive, and not marked as going away.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Resume quic data and a connectivity probe response will be read on the new
  // socket.
  quic_data2.Resume();

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // There should be three pending tasks, the nearest one will complete
  // migration to the new network.
  EXPECT_EQ(3u, task_runner->GetPendingTaskCount());
  next_task_delay = task_runner->NextPendingTaskDelay();
  EXPECT_EQ(base::TimeDelta(), next_task_delay);
  task_runner->FastForwardBy(next_task_delay);

  // Response headers are received over the new network.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Now there are two pending tasks, the nearest one was to send connectivity
  // probe and has been cancelled due to successful migration.
  EXPECT_EQ(2u, task_runner->GetPendingTaskCount());
  next_task_delay = task_runner->NextPendingTaskDelay();
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(2 * kDefaultRTTMilliSecs),
            next_task_delay);
  task_runner->FastForwardBy(next_task_delay);

  // There's one more task to mgirate back to the default network in 0.4s.
  EXPECT_EQ(1u, task_runner->GetPendingTaskCount());
  next_task_delay = task_runner->NextPendingTaskDelay();
  base::TimeDelta expected_delay =
      base::TimeDelta::FromSeconds(kMinRetryTimeForDefaultNetworkSecs) -
      base::TimeDelta::FromMilliseconds(2 * kDefaultRTTMilliSecs);
  EXPECT_EQ(expected_delay, next_task_delay);

  // Deliver a signal that the alternate network now becomes default to session,
  // this will cancel mgirate back to default network timer.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);

  task_runner->FastForwardBy(next_task_delay);
  EXPECT_EQ(0u, task_runner->GetPendingTaskCount());

  // Verify that the session is still alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  stream.reset();
  EXPECT_TRUE(quic_data1.AllReadDataConsumed());
  EXPECT_TRUE(quic_data1.AllWriteDataConsumed());
  EXPECT_TRUE(quic_data2.AllReadDataConsumed());
  EXPECT_TRUE(quic_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnNetworkChangeDisconnectedPauseBeforeConnected) {
  InitializeConnectionMigrationTest({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                        2, GetNthClientInitiatedStreamId(0),
                                        true, true, &header_stream_offset));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Trigger connection migration. Since there are no networks
  // to migrate to, this should cause the session to wait for a new network.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  // The connection should still be alive, but marked as going away.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Set up second socket data provider that is used after migration.
  // The response to the earlier request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(
      SYNCHRONOUS, client_maker_.MakePingPacket(3, /*include_version=*/true));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            4, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Add a new network and notify the stream factory of a new connected network.
  // This causes a PING packet to be sent over the new network.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->SetConnectedNetworksList({kNewNetworkForTests});
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkConnected(kNewNetworkForTests);

  // Ensure that the session is still alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // Response headers are received over the new network.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Create a new request and verify that a new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_NE(session, GetActiveSession(host_port_pair_));
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));

  stream.reset();
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       OnNetworkChangeDisconnectedPauseBeforeConnectedMultipleSessions) {
  InitializeConnectionMigrationTest({kDefaultNetworkForTests});

  MockQuicData socket_data1;
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data1.AddWrite(ASYNC, OK);
  socket_data1.AddSocketDataToFactory(socket_factory_.get());
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddWrite(ASYNC, OK);
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server1(kDefaultServerHostName, 443);
  HostPortPair server2(kServer2HostName, 443);

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(server1.host(), "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.2", "");

  // Create request and QuicHttpStream to create session1.
  QuicStreamRequest request1(factory_.get());
  EXPECT_EQ(OK, request1.Request(server1, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream1 = CreateStream(&request1);
  EXPECT_TRUE(stream1.get());

  // Create request and QuicHttpStream to create session2.
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback_.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  QuicChromiumClientSession* session1 = GetActiveSession(server1);
  QuicChromiumClientSession* session2 = GetActiveSession(server2);
  EXPECT_NE(session1, session2);

  // Cause QUIC stream to be created and send GET so session1 has an open
  // stream.
  HttpRequestInfo request_info1;
  request_info1.method = "GET";
  request_info1.url = url_;
  request_info1.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK,
            stream1->InitializeStream(&request_info1, true, DEFAULT_PRIORITY,
                                      net_log_, CompletionOnceCallback()));
  HttpResponseInfo response1;
  HttpRequestHeaders request_headers1;
  EXPECT_EQ(OK, stream1->SendRequest(request_headers1, &response1,
                                     callback_.callback()));

  // Cause QUIC stream to be created and send GET so session2 has an open
  // stream.
  HttpRequestInfo request_info2;
  request_info2.method = "GET";
  request_info2.url = url_;
  request_info2.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK,
            stream2->InitializeStream(&request_info2, true, DEFAULT_PRIORITY,
                                      net_log_, CompletionOnceCallback()));
  HttpResponseInfo response2;
  HttpRequestHeaders request_headers2;
  EXPECT_EQ(OK, stream2->SendRequest(request_headers2, &response2,
                                     callback_.callback()));

  // Cause both sessions to be paused due to DISCONNECTED.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  // Ensure that both sessions are paused but alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session1));
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session2));

  // Add new sockets to use post migration.
  MockConnect connect_result =
      MockConnect(SYNCHRONOUS, ERR_INTERNET_DISCONNECTED);
  SequencedSocketData socket_data3(connect_result, base::span<MockRead>(),
                                   base::span<MockWrite>());
  socket_factory_->AddSocketDataProvider(&socket_data3);
  SequencedSocketData socket_data4(connect_result, base::span<MockRead>(),
                                   base::span<MockWrite>());
  socket_factory_->AddSocketDataProvider(&socket_data4);

  // Add a new network and cause migration to bad sockets, causing sessions to
  // close.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->SetConnectedNetworksList({kNewNetworkForTests});
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkConnected(kNewNetworkForTests);

  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session1));
  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session2));

  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MigrateSessionEarly) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                        2, GetNthClientInitiatedStreamId(0),
                                        true, true, &header_stream_offset));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  MockQuicData socket_data1;
  socket_data1.AddWrite(
      SYNCHRONOUS, client_maker_.MakePingPacket(3, /*include_version=*/true));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            4, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Trigger early connection migration. This should cause a PING frame
  // to be emitted.
  session->OnPathDegrading();

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_THAT(stream->ReadResponseHeaders(callback_.callback()), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Create a new request for the same destination and verify that a
  // new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  QuicChromiumClientSession* new_session = GetActiveSession(host_port_pair_);
  EXPECT_NE(session, new_session);

  // On a NETWORK_MADE_DEFAULT notification, nothing happens to the
  // migrated session, but the new session is closed since it has no
  // open streams.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_FALSE(
      QuicStreamFactoryPeer::IsLiveSession(factory_.get(), new_session));

  // On a DISCONNECTED notification, nothing happens to the migrated session.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();
  stream2.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MigrateSessionEarlyWithAsyncWrites) {
  // Nearly identical to MigrateSessionEarly except that the write to
  // the second socket is asynchronous.  Ensures that the callback
  // infrastructure for asynchronous writes is set up correctly for
  // the old connection on the migrated socket.
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                        2, GetNthClientInitiatedStreamId(0),
                                        true, true, &header_stream_offset));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Set up second socket data provider that is used after migration.
  // The response to the earlier request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(
      SYNCHRONOUS, client_maker_.MakePingPacket(3, /*include_version=*/true));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            4, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Trigger early connection migration. This should cause a PING frame
  // to be emitted.
  session->OnPathDegrading();

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_THAT(stream->ReadResponseHeaders(callback_.callback()), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Create a new request for the same destination and verify that a
  // new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  QuicChromiumClientSession* new_session = GetActiveSession(host_port_pair_);
  EXPECT_NE(session, new_session);

  // On a NETWORK_MADE_DEFAULT notification, nothing happens to the
  // migrated session, but the new session is closed since it has no
  // open streams.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkMadeDefault(kNewNetworkForTests);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_FALSE(
      QuicStreamFactoryPeer::IsLiveSession(factory_.get(), new_session));

  // On a DISCONNECTED notification, nothing happens to the migrated session.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();
  stream2.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MigrateSessionEarlyNoNewNetwork) {
  InitializeConnectionMigrationTest({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration. Since there are no networks
  // to migrate to, this should cause session to be continue but be marked as
  // going away.
  session->OnPathDegrading();

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MigrateSessionEarlyNonMigratableStream) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created, but marked as non-migratable.
  HttpRequestInfo request_info;
  request_info.load_flags |= LOAD_DISABLE_CONNECTION_MIGRATION;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Trigger connection migration. Since there is a non-migratable stream,
  // this should cause session to be continue without migrating.
  session->OnPathDegrading();

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MigrateSessionEarlyConnectionMigrationDisabled) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set session config to have connection migration disabled.
  QuicConfigPeer::SetReceivedDisableConnectionMigration(session->config());
  EXPECT_TRUE(session->config()->DisableConnectionMigration());

  // Trigger connection migration. Since there is a non-migratable stream,
  // this should cause session to be continue without migrating.
  session->OnPathDegrading();

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

void QuicStreamFactoryTestBase::TestMigrationOnWriteError(
    IoMode write_error_mode) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(write_error_mode, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set up second socket data provider that is used after
  // migration. The request is rewritten to this new socket, and the
  // response to the request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                         2, GetNthClientInitiatedStreamId(0),
                                         true, true, &header_stream_offset));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            3, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Send GET request on stream. This should cause a write error, which triggers
  // a connection migration attempt.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Run the message loop so that the migration attempt is executed and
  // data queued in the new socket is read by the packet reader.
  base::RunLoop().RunUntilIdle();

  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_EQ(OK, stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_EQ(200, response.headers->response_code());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnWriteErrorSynchronous) {
  TestMigrationOnWriteError(SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnWriteErrorAsync) {
  TestMigrationOnWriteError(ASYNC);
}

void QuicStreamFactoryTestBase::TestMigrationOnWriteErrorNoNewNetwork(
    IoMode write_error_mode) {
  InitializeConnectionMigrationTest({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Use the test task runner, to force the migration alarm timeout later.
  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), runner_.get());

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(write_error_mode, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream. This causes a write error, which triggers
  // a connection migration attempt. Since there are no networks
  // to migrate to, this causes the session to wait for a new network.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Complete any pending writes. Pending async MockQuicData writes
  // are run on the message loop, not on the test runner.
  base::RunLoop().RunUntilIdle();

  // Write error causes migration task to be posted. Spin the loop.
  if (write_error_mode == ASYNC)
    runner_->RunNextTask();

  // Migration has not yet failed. The session should be alive and active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_TRUE(session->connection()->writer()->IsWriteBlocked());

  // The migration will not fail until the migration alarm timeout.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Force migration alarm timeout to run.
  RunTestLoopUntilIdle();

  // The connection should be closed. A request for response headers
  // should fail.
  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(ERR_NETWORK_CHANGED, callback_.WaitForResult());
  EXPECT_EQ(ERR_NETWORK_CHANGED,
            stream->ReadResponseHeaders(callback_.callback()));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorNoNewNetworkSynchronous) {
  TestMigrationOnWriteErrorNoNewNetwork(SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnWriteErrorNoNewNetworkAsync) {
  TestMigrationOnWriteErrorNoNewNetwork(ASYNC);
}

void QuicStreamFactoryTestBase::TestMigrationOnWriteErrorNonMigratableStream(
    IoMode write_error_mode) {
  DVLOG(1) << "Mode: "
           << ((write_error_mode == SYNCHRONOUS) ? "SYNCHRONOUS" : "ASYNC");
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(write_error_mode, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created, but marked as non-migratable.
  HttpRequestInfo request_info;
  request_info.load_flags |= LOAD_DISABLE_CONNECTION_MIGRATION;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream. This should cause a write error, which triggers
  // a connection migration attempt.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Run message loop to execute migration attempt.
  base::RunLoop().RunUntilIdle();

  // Migration fails, and session is closed and deleted.
  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorNonMigratableStreamSynchronous) {
  TestMigrationOnWriteErrorNonMigratableStream(SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorNonMigratableStreamAsync) {
  TestMigrationOnWriteErrorNonMigratableStream(ASYNC);
}

void QuicStreamFactoryTestBase::TestMigrationOnWriteErrorMigrationDisabled(
    IoMode write_error_mode) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(write_error_mode, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set session config to have connection migration disabled.
  QuicConfigPeer::SetReceivedDisableConnectionMigration(session->config());
  EXPECT_TRUE(session->config()->DisableConnectionMigration());

  // Send GET request on stream. This should cause a write error, which triggers
  // a connection migration attempt.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));
  // Run message loop to execute migration attempt.
  base::RunLoop().RunUntilIdle();
  // Migration fails, and session is closed and deleted.
  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorMigrationDisabledSynchronous) {
  TestMigrationOnWriteErrorMigrationDisabled(SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorMigrationDisabledAsync) {
  TestMigrationOnWriteErrorMigrationDisabled(ASYNC);
}

void QuicStreamFactoryTestBase::TestMigrationOnMultipleWriteErrors(
    IoMode first_write_error_mode,
    IoMode second_write_error_mode) {
  const int kMaxReadersPerQuicSession = 5;
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Set up kMaxReadersPerQuicSession socket data providers, since
  // migration will cause kMaxReadersPerQuicSession write failures as
  // the session hops repeatedly between the two networks.
  MockQuicData socket_data[kMaxReadersPerQuicSession + 1];
  for (int i = 0; i <= kMaxReadersPerQuicSession; ++i) {
    // The last socket is created but never used.
    if (i < kMaxReadersPerQuicSession) {
      socket_data[i].AddRead(SYNCHRONOUS, ERR_IO_PENDING);
      if (i == 0) {
        socket_data[i].AddWrite(SYNCHRONOUS,
                                ConstructInitialSettingsPacket(1, nullptr));
      }
      socket_data[i].AddWrite(
          (i % 2 == 0) ? first_write_error_mode : second_write_error_mode,
          ERR_FAILED);
    }
    socket_data[i].AddSocketDataToFactory(socket_factory_.get());
  }

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream. This should cause a write error, which triggers
  // a connection migration attempt.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // The connection should be closed because of a write error after migration.
  EXPECT_FALSE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(ERR_QUIC_PROTOCOL_ERROR,
            stream->ReadResponseHeaders(callback_.callback()));

  stream.reset();
  for (int i = 0; i <= kMaxReadersPerQuicSession; ++i) {
    DLOG(INFO) << "Socket number: " << i;
    EXPECT_TRUE(socket_data[i].AllReadDataConsumed());
    EXPECT_TRUE(socket_data[i].AllWriteDataConsumed());
  }
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnMultipleWriteErrorsSyncSync) {
  TestMigrationOnMultipleWriteErrors(SYNCHRONOUS, SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnMultipleWriteErrorsSyncAsync) {
  TestMigrationOnMultipleWriteErrors(SYNCHRONOUS, ASYNC);
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnMultipleWriteErrorsAsyncSync) {
  TestMigrationOnMultipleWriteErrors(ASYNC, SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest, MigrateSessionOnMultipleWriteErrorsAsyncAsync) {
  TestMigrationOnMultipleWriteErrors(ASYNC, ASYNC);
}

void QuicStreamFactoryTestBase::TestMigrationOnWriteErrorWithNotificationQueued(
    bool disconnected) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set up second socket data provider that is used after
  // migration. The request is rewritten to this new socket, and the
  // response to the request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                         2, GetNthClientInitiatedStreamId(0),
                                         true, true, &header_stream_offset));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            3, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // First queue a network change notification in the message loop.
  if (disconnected) {
    scoped_mock_network_change_notifier_->mock_network_change_notifier()
        ->QueueNetworkDisconnected(kDefaultNetworkForTests);
  } else {
    scoped_mock_network_change_notifier_->mock_network_change_notifier()
        ->QueueNetworkMadeDefault(kNewNetworkForTests);
  }
  // Send GET request on stream. This should cause a write error,
  // which triggers a connection migration attempt. This will queue a
  // migration attempt behind the notification in the message loop.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  base::RunLoop().RunUntilIdle();
  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_EQ(OK, stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_EQ(200, response.headers->response_code());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorWithNetworkDisconnectedQueued) {
  TestMigrationOnWriteErrorWithNotificationQueued(/*disconnected=*/true);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorWithNetworkMadeDefaultQueued) {
  TestMigrationOnWriteErrorWithNotificationQueued(/*disconnected=*/false);
}

void QuicStreamFactoryTestBase::TestMigrationOnNotificationWithWriteErrorQueued(
    bool disconnected) {
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ERR_ADDRESS_UNREACHABLE);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Set up second socket data provider that is used after
  // migration. The request is rewritten to this new socket, and the
  // response to the request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                         2, GetNthClientInitiatedStreamId(0),
                                         true, true, &header_stream_offset));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            3, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Send GET request on stream. This should cause a write error,
  // which triggers a connection migration attempt. This will queue a
  // migration attempt in the message loop.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Now queue a network change notification in the message loop behind
  // the migration attempt.
  if (disconnected) {
    scoped_mock_network_change_notifier_->mock_network_change_notifier()
        ->QueueNetworkDisconnected(kDefaultNetworkForTests);
  } else {
    scoped_mock_network_change_notifier_->mock_network_change_notifier()
        ->QueueNetworkMadeDefault(kNewNetworkForTests);
  }

  base::RunLoop().RunUntilIdle();
  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_EQ(OK, stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_EQ(200, response.headers->response_code());

  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnNetworkDisconnectedWithWriteErrorQueued) {
  TestMigrationOnNotificationWithWriteErrorQueued(/*disconnected=*/true);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnNetworkMadeDefaultWithWriteErrorQueued) {
  TestMigrationOnNotificationWithWriteErrorQueued(/*disconnected=*/true);
}

void QuicStreamFactoryTestBase::TestMigrationOnWriteErrorPauseBeforeConnected(
    IoMode write_error_mode) {
  InitializeConnectionMigrationTest({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ERR_FAILED);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream. This should cause a write error, which triggers
  // a connection migration attempt.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // In this particular code path, the network will not yet be marked
  // as going away and the session will still be alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // On a DISCONNECTED notification, nothing happens.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkDisconnected(kDefaultNetworkForTests);

  // Set up second socket data provider that is used after
  // migration. The request is rewritten to this new socket, and the
  // response to the request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                         2, GetNthClientInitiatedStreamId(0),
                                         true, true, &header_stream_offset));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            3, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->SetConnectedNetworksList({kNewNetworkForTests});
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkConnected(kNewNetworkForTests);

  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // This is the callback for the response headers that returned
  // pending previously, because no result was available.  Check that
  // the result is now available due to the successful migration.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Create a new request for the same destination and verify that a
  // new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  QuicChromiumClientSession* new_session = GetActiveSession(host_port_pair_);
  EXPECT_NE(session, new_session);

  stream.reset();
  stream2.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorPauseBeforeConnectedSync) {
  TestMigrationOnWriteErrorPauseBeforeConnected(SYNCHRONOUS);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorPauseBeforeConnectedAsync) {
  TestMigrationOnWriteErrorPauseBeforeConnected(ASYNC);
}

void QuicStreamFactoryTestBase::
    TestMigrationOnWriteErrorWithNetworkAddedBeforeNotification(
        IoMode write_error_mode,
        bool disconnected) {
  InitializeConnectionMigrationTest({kDefaultNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ERR_FAILED);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream. This should cause a write error, which triggers
  // a connection migration attempt.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // In this particular code path, the network will not yet be marked
  // as going away and the session will still be alive.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());
  EXPECT_EQ(ERR_IO_PENDING, stream->ReadResponseHeaders(callback_.callback()));

  // Set up second socket data provider that is used after
  // migration. The request is rewritten to this new socket, and the
  // response to the request is read on this new socket.
  MockQuicData socket_data1;
  socket_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                         2, GetNthClientInitiatedStreamId(0),
                                         true, true, &header_stream_offset));
  socket_data1.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            3, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->SetConnectedNetworksList(
          {kDefaultNetworkForTests, kNewNetworkForTests});

  // A notification triggers and completes migration.
  if (disconnected) {
    scoped_mock_network_change_notifier_->mock_network_change_notifier()
        ->NotifyNetworkDisconnected(kDefaultNetworkForTests);
  } else {
    scoped_mock_network_change_notifier_->mock_network_change_notifier()
        ->NotifyNetworkMadeDefault(kNewNetworkForTests);
  }
  // The session should now be marked as going away. Ensure that
  // while it is still alive, it is no longer active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // This is the callback for the response headers that returned
  // pending previously, because no result was available.  Check that
  // the result is now available due to the successful migration.
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  EXPECT_EQ(200, response.headers->response_code());

  // Now deliver a CONNECTED notification. Nothing happens since
  // migration was already finished earlier.
  scoped_mock_network_change_notifier_->mock_network_change_notifier()
      ->NotifyNetworkConnected(kNewNetworkForTests);

  // Create a new request for the same destination and verify that a
  // new session is created.
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  QuicChromiumClientSession* new_session = GetActiveSession(host_port_pair_);
  EXPECT_NE(session, new_session);

  stream.reset();
  stream2.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorWithNetworkAddedBeforeDisconnectedSync) {
  TestMigrationOnWriteErrorWithNetworkAddedBeforeNotification(SYNCHRONOUS,
                                                              true);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorWithNetworkAddedBeforeDisconnectedAsync) {
  TestMigrationOnWriteErrorWithNetworkAddedBeforeNotification(ASYNC, true);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorWithNetworkAddedBeforeMadeDefaultSync) {
  TestMigrationOnWriteErrorWithNetworkAddedBeforeNotification(SYNCHRONOUS,
                                                              false);
}

TEST_P(QuicStreamFactoryTest,
       MigrateSessionOnWriteErrorWithNetworkAddedBeforeMadeDefaultAsync) {
  TestMigrationOnWriteErrorWithNetworkAddedBeforeNotification(ASYNC, false);
}

TEST_P(QuicStreamFactoryTest, MigrateSessionEarlyToBadSocket) {
  // This simulates the case where we attempt to migrate to a new
  // socket but the socket is unusable, such as an ipv4/ipv6 mismatch.
  InitializeConnectionMigrationTest(
      {kDefaultNetworkForTests, kNewNetworkForTests});
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  QuicStreamOffset header_stream_offset = 0;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                        2, GetNthClientInitiatedStreamId(0),
                                        true, true, &header_stream_offset));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = url_;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  // Set up second socket that will immediately return disconnected.
  // The stream factory will attempt to migrate to the new socket and
  // immediately fail.
  MockConnect connect_result =
      MockConnect(SYNCHRONOUS, ERR_INTERNET_DISCONNECTED);
  SequencedSocketData socket_data1(connect_result, base::span<MockRead>(),
                                   base::span<MockWrite>());
  socket_factory_->AddSocketDataProvider(&socket_data1);

  // Trigger early connection migration.
  session->OnPathDegrading();

  // Migration fails, and the session is marked as going away.
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, ServerMigration) {
  allow_server_migration_ = true;
  Initialize();

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data1;
  QuicStreamOffset header_stream_offset = 0;
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(
      SYNCHRONOUS, ConstructInitialSettingsPacket(1, &header_stream_offset));
  socket_data1.AddWrite(SYNCHRONOUS, ConstructGetRequestPacket(
                                         2, GetNthClientInitiatedStreamId(0),
                                         true, true, &header_stream_offset));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Send GET request on stream.
  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  EXPECT_EQ(OK, stream->SendRequest(request_headers, &response,
                                    callback_.callback()));

  IPEndPoint ip;
  session->GetDefaultSocket()->GetPeerAddress(&ip);
  DVLOG(1) << "Socket connected to: " << ip.address().ToString() << " "
           << ip.port();

  // Set up second socket data provider that is used after
  // migration. The request is rewritten to this new socket, and the
  // response to the request is read on this new socket.
  MockQuicData socket_data2;
  socket_data2.AddWrite(
      SYNCHRONOUS, client_maker_.MakePingPacket(3, /*include_version=*/true));
  socket_data2.AddRead(
      ASYNC, ConstructOkResponsePacket(1, GetNthClientInitiatedStreamId(0),
                                       false, false));
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS,
                        client_maker_.MakeAckAndRstPacket(
                            4, false, GetNthClientInitiatedStreamId(0),
                            QUIC_STREAM_CANCELLED, 1, 1, 1, true));
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  const uint8_t kTestIpAddress[] = {1, 2, 3, 4};
  const uint16_t kTestPort = 123;
  session->Migrate(NetworkChangeNotifier::kInvalidNetworkHandle,
                   IPEndPoint(IPAddress(kTestIpAddress), kTestPort), true,
                   net_log_);

  session->GetDefaultSocket()->GetPeerAddress(&ip);
  DVLOG(1) << "Socket migrated to: " << ip.address().ToString() << " "
           << ip.port();

  // The session should be alive and active.
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_EQ(1u, session->GetNumActiveStreams());

  // Run the message loop so that data queued in the new socket is read by the
  // packet reader.
  base::RunLoop().RunUntilIdle();

  // Verify that response headers on the migrated socket were delivered to the
  // stream.
  EXPECT_EQ(OK, stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_EQ(200, response.headers->response_code());

  stream.reset();

  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, ServerMigrationIPv4ToIPv4) {
  // Add alternate IPv4 server address to config.
  IPEndPoint alt_address = IPEndPoint(IPAddress(1, 2, 3, 4), 123);
  QuicConfig config;
  config.SetAlternateServerAddressToSend(
      QuicSocketAddress(QuicSocketAddressImpl(alt_address)));
  VerifyServerMigration(config, alt_address);
}

TEST_P(QuicStreamFactoryTest, ServerMigrationIPv6ToIPv6) {
  // Add a resolver rule to make initial connection to an IPv6 address.
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "fe80::aebc:32ff:febb:1e33", "");
  // Add alternate IPv6 server address to config.
  IPEndPoint alt_address = IPEndPoint(
      IPAddress(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), 123);
  QuicConfig config;
  config.SetAlternateServerAddressToSend(
      QuicSocketAddress(QuicSocketAddressImpl(alt_address)));
  VerifyServerMigration(config, alt_address);
}

TEST_P(QuicStreamFactoryTest, ServerMigrationIPv6ToIPv4) {
  // Add a resolver rule to make initial connection to an IPv6 address.
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "fe80::aebc:32ff:febb:1e33", "");
  // Add alternate IPv4 server address to config.
  IPEndPoint alt_address = IPEndPoint(IPAddress(1, 2, 3, 4), 123);
  QuicConfig config;
  config.SetAlternateServerAddressToSend(
      QuicSocketAddress(QuicSocketAddressImpl(alt_address)));
  IPEndPoint expected_address(
      ConvertIPv4ToIPv4MappedIPv6(alt_address.address()), alt_address.port());
  VerifyServerMigration(config, expected_address);
}

TEST_P(QuicStreamFactoryTest, ServerMigrationIPv4ToIPv6Fails) {
  allow_server_migration_ = true;
  Initialize();

  // Add a resolver rule to make initial connection to an IPv4 address.
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(), "1.2.3.4",
                                           "");
  // Add alternate IPv6 server address to config.
  IPEndPoint alt_address = IPEndPoint(
      IPAddress(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), 123);
  QuicConfig config;
  config.SetAlternateServerAddressToSend(
      QuicSocketAddress(QuicSocketAddressImpl(alt_address)));

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  crypto_client_stream_factory_.SetConfig(config);

  // Set up only socket data provider.
  MockQuicData socket_data1;
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data1.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthClientInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  // Create request and QuicHttpStream.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Cause QUIC stream to be created.
  HttpRequestInfo request_info;
  request_info.method = "GET";
  request_info.url = GURL("https://www.example.org/");
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, true, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  // Ensure that session is alive and active.
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  IPEndPoint actual_address;
  session->GetDefaultSocket()->GetPeerAddress(&actual_address);
  // No migration should have happened.
  IPEndPoint expected_address =
      IPEndPoint(IPAddress(1, 2, 3, 4), kDefaultServerPort);
  EXPECT_EQ(actual_address, expected_address);
  DVLOG(1) << "Socket connected to: " << actual_address.address().ToString()
           << " " << actual_address.port();
  DVLOG(1) << "Expected address: " << expected_address.address().ToString()
           << " " << expected_address.port();

  stream.reset();
  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnSSLConfigChanged) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddWrite(SYNCHRONOUS,
                       ConstructClientRstPacket(2, QUIC_RST_ACKNOWLEDGEMENT));
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS,
                        ConstructInitialSettingsPacket(1, nullptr));
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionCallback()));

  ssl_config_service_->NotifySSLConfigChange();
  EXPECT_EQ(ERR_CERT_DATABASE_CHANGED,
            stream->ReadResponseHeaders(callback_.callback()));
  EXPECT_FALSE(factory_->require_confirmation());

  // Now attempting to request a stream to the same origin should create
  // a new session.

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  stream = CreateStream(&request2);
  stream.reset();  // Will reset stream 3.

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, OnCertDBChanged) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS,
                        ConstructInitialSettingsPacket(1, nullptr));
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream);
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);

  // Change the CA cert and verify that stream saw the event.
  factory_->OnCertDBChanged();

  EXPECT_FALSE(factory_->require_confirmation());
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_FALSE(HasActiveSession(host_port_pair_));

  // Now attempting to request a stream to the same origin should create
  // a new session.

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2);
  QuicChromiumClientSession* session2 = GetActiveSession(host_port_pair_);
  EXPECT_TRUE(HasActiveSession(host_port_pair_));
  EXPECT_NE(session, session2);
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session));
  EXPECT_TRUE(QuicStreamFactoryPeer::IsLiveSession(factory_.get(), session2));

  stream2.reset();
  stream.reset();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, SharedCryptoConfig) {
  Initialize();

  std::vector<string> cannoncial_suffixes;
  cannoncial_suffixes.push_back(string(".c.youtube.com"));
  cannoncial_suffixes.push_back(string(".googlevideo.com"));

  for (unsigned i = 0; i < cannoncial_suffixes.size(); ++i) {
    string r1_host_name("r1");
    string r2_host_name("r2");
    r1_host_name.append(cannoncial_suffixes[i]);
    r2_host_name.append(cannoncial_suffixes[i]);

    HostPortPair host_port_pair1(r1_host_name, 80);
    QuicCryptoClientConfig* crypto_config =
        QuicStreamFactoryPeer::GetCryptoConfig(factory_.get());
    QuicServerId server_id1(host_port_pair1, privacy_mode_);
    QuicCryptoClientConfig::CachedState* cached1 =
        crypto_config->LookupOrCreate(server_id1);
    EXPECT_FALSE(cached1->proof_valid());
    EXPECT_TRUE(cached1->source_address_token().empty());

    // Mutate the cached1 to have different data.
    // TODO(rtenneti): mutate other members of CachedState.
    cached1->set_source_address_token(r1_host_name);
    cached1->SetProofValid();

    HostPortPair host_port_pair2(r2_host_name, 80);
    QuicServerId server_id2(host_port_pair2, privacy_mode_);
    QuicCryptoClientConfig::CachedState* cached2 =
        crypto_config->LookupOrCreate(server_id2);
    EXPECT_EQ(cached1->source_address_token(), cached2->source_address_token());
    EXPECT_TRUE(cached2->proof_valid());
  }
}

TEST_P(QuicStreamFactoryTest, CryptoConfigWhenProofIsInvalid) {
  Initialize();
  std::vector<string> cannoncial_suffixes;
  cannoncial_suffixes.push_back(string(".c.youtube.com"));
  cannoncial_suffixes.push_back(string(".googlevideo.com"));

  for (unsigned i = 0; i < cannoncial_suffixes.size(); ++i) {
    string r3_host_name("r3");
    string r4_host_name("r4");
    r3_host_name.append(cannoncial_suffixes[i]);
    r4_host_name.append(cannoncial_suffixes[i]);

    HostPortPair host_port_pair1(r3_host_name, 80);
    QuicCryptoClientConfig* crypto_config =
        QuicStreamFactoryPeer::GetCryptoConfig(factory_.get());
    QuicServerId server_id1(host_port_pair1, privacy_mode_);
    QuicCryptoClientConfig::CachedState* cached1 =
        crypto_config->LookupOrCreate(server_id1);
    EXPECT_FALSE(cached1->proof_valid());
    EXPECT_TRUE(cached1->source_address_token().empty());

    // Mutate the cached1 to have different data.
    // TODO(rtenneti): mutate other members of CachedState.
    cached1->set_source_address_token(r3_host_name);
    cached1->SetProofInvalid();

    HostPortPair host_port_pair2(r4_host_name, 80);
    QuicServerId server_id2(host_port_pair2, privacy_mode_);
    QuicCryptoClientConfig::CachedState* cached2 =
        crypto_config->LookupOrCreate(server_id2);
    EXPECT_NE(cached1->source_address_token(), cached2->source_address_token());
    EXPECT_TRUE(cached2->source_address_token().empty());
    EXPECT_FALSE(cached2->proof_valid());
  }
}

TEST_P(QuicStreamFactoryTest, EnableNotLoadFromDiskCache) {
  Initialize();
  factory_->set_require_confirmation(false);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), runner_.get());

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));

  // If we are waiting for disk cache, we would have posted a task. Verify that
  // the CancelWaitForDataReady task hasn't been posted.
  ASSERT_EQ(0u, runner_->GetPostedTasks().size());

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, ReducePingTimeoutOnConnectionTimeOutOpenStreams) {
  reduced_ping_timeout_seconds_ = 10;
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  QuicStreamFactoryPeer::SetTaskRunner(factory_.get(), runner_.get());

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS,
                        ConstructInitialSettingsPacket(1, nullptr));
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  HostPortPair server2(kServer2HostName, kDefaultServerPort);

  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::CONFIRM_HANDSHAKE);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");
  host_resolver_.rules()->AddIPLiteralRule(server2.host(), "192.168.0.1", "");

  // Quic should use default PING timeout when no previous connection times out
  // with open stream.
  EXPECT_EQ(QuicTime::Delta::FromSeconds(kPingTimeoutSecs),
            QuicStreamFactoryPeer::GetPingTimeout(factory_.get()));
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);
  EXPECT_EQ(QuicTime::Delta::FromSeconds(kPingTimeoutSecs),
            session->connection()->ping_timeout());

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());
  HttpRequestInfo request_info;
  request_info.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(OK, stream->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                         net_log_, CompletionOnceCallback()));

  DVLOG(1)
      << "Created 1st session and initialized a stream. Now trigger timeout";
  session->connection()->CloseConnection(QUIC_NETWORK_IDLE_TIMEOUT, "test",
                                         ConnectionCloseBehavior::SILENT_CLOSE);
  // Need to spin the loop now to ensure that
  // QuicStreamFactory::OnSessionClosed() runs.
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  // The first connection times out with open stream, QUIC should reduce initial
  // PING time for subsequent connections.
  EXPECT_EQ(QuicTime::Delta::FromSeconds(10),
            QuicStreamFactoryPeer::GetPingTimeout(factory_.get()));

  // Test two-in-a-row timeouts with open streams.
  DVLOG(1) << "Create 2nd session and timeout with open stream";
  TestCompletionCallback callback2;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(server2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2_, net_log_,
                                 &net_error_details_, callback2.callback()));
  QuicChromiumClientSession* session2 = GetActiveSession(server2);
  EXPECT_EQ(QuicTime::Delta::FromSeconds(10),
            session2->connection()->ping_timeout());

  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());
  EXPECT_EQ(OK,
            stream2->InitializeStream(&request_info, false, DEFAULT_PRIORITY,
                                      net_log_, CompletionOnceCallback()));
  session2->connection()->CloseConnection(
      QUIC_NETWORK_IDLE_TIMEOUT, "test", ConnectionCloseBehavior::SILENT_CLOSE);
  // Need to spin the loop now to ensure that
  // QuicStreamFactory::OnSessionClosed() runs.
  base::RunLoop run_loop2;
  run_loop2.RunUntilIdle();

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

// Verifies that the QUIC stream factory is initialized correctly.
TEST_P(QuicStreamFactoryTest, MaybeInitialize) {
  VerifyInitialization();
}

TEST_P(QuicStreamFactoryTest, StartCertVerifyJob) {
  Initialize();

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  // Save current state of |race_cert_verification|.
  bool race_cert_verification =
      QuicStreamFactoryPeer::GetRaceCertVerification(factory_.get());

  // Load server config.
  HostPortPair host_port_pair(kDefaultServerHostName, kDefaultServerPort);
  QuicServerId quic_server_id(host_port_pair_, privacy_mode_);
  QuicStreamFactoryPeer::CacheDummyServerConfig(factory_.get(), quic_server_id);

  QuicStreamFactoryPeer::SetRaceCertVerification(factory_.get(), true);
  EXPECT_FALSE(HasActiveCertVerifierJob(quic_server_id));

  // Start CertVerifyJob.
  QuicAsyncStatus status = QuicStreamFactoryPeer::StartCertVerifyJob(
      factory_.get(), quic_server_id, /*cert_verify_flags=*/0, net_log_);
  if (status == QUIC_PENDING) {
    // Verify CertVerifierJob has started.
    EXPECT_TRUE(HasActiveCertVerifierJob(quic_server_id));

    while (HasActiveCertVerifierJob(quic_server_id)) {
      base::RunLoop().RunUntilIdle();
    }
  }
  // Verify CertVerifierJob has finished.
  EXPECT_FALSE(HasActiveCertVerifierJob(quic_server_id));

  // Start a QUIC request.
  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_EQ(OK, callback_.WaitForResult());

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  // Restore |race_cert_verification|.
  QuicStreamFactoryPeer::SetRaceCertVerification(factory_.get(),
                                                 race_cert_verification);

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());

  // Verify there are no outstanding CertVerifierJobs after request has
  // finished.
  EXPECT_FALSE(HasActiveCertVerifierJob(quic_server_id));
}

TEST_P(QuicStreamFactoryTest, YieldAfterPackets) {
  Initialize();
  factory_->set_require_confirmation(false);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  QuicStreamFactoryPeer::SetYieldAfterPackets(factory_.get(), 0);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ConstructClientConnectionClosePacket(0));
  socket_data.AddRead(ASYNC, OK);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");

  // Set up the TaskObserver to verify QuicChromiumPacketReader::StartReading
  // posts a task.
  // TODO(rtenneti): Change SpdySessionTestTaskObserver to NetTestTaskObserver??
  SpdySessionTestTaskObserver observer("quic_chromium_packet_reader.cc",
                                       "StartReading");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));

  // Call run_loop so that QuicChromiumPacketReader::OnReadComplete() gets
  // called.
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  // Verify task that the observer's executed_count is 1, which indicates
  // QuicChromiumPacketReader::StartReading() has posted only one task and
  // yielded the read.
  EXPECT_EQ(1u, observer.executed_count());

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_FALSE(stream.get());  // Session is already closed.
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, YieldAfterDuration) {
  Initialize();
  factory_->set_require_confirmation(false);
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  QuicStreamFactoryPeer::SetYieldAfterDuration(
      factory_.get(), QuicTime::Delta::FromMilliseconds(-1));

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ConstructClientConnectionClosePacket(0));
  socket_data.AddRead(ASYNC, OK);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  host_resolver_.set_synchronous_mode(true);
  host_resolver_.rules()->AddIPLiteralRule(host_port_pair_.host(),
                                           "192.168.0.1", "");

  // Set up the TaskObserver to verify QuicChromiumPacketReader::StartReading
  // posts a task.
  // TODO(rtenneti): Change SpdySessionTestTaskObserver to NetTestTaskObserver??
  SpdySessionTestTaskObserver observer("quic_chromium_packet_reader.cc",
                                       "StartReading");

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(OK, request.Request(host_port_pair_, version_, privacy_mode_,
                                DEFAULT_PRIORITY, SocketTag(),
                                /*cert_verify_flags=*/0, url_, net_log_,
                                &net_error_details_, callback_.callback()));

  // Call run_loop so that QuicChromiumPacketReader::OnReadComplete() gets
  // called.
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  // Verify task that the observer's executed_count is 1, which indicates
  // QuicChromiumPacketReader::StartReading() has posted only one task and
  // yielded the read.
  EXPECT_EQ(1u, observer.executed_count());

  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_FALSE(stream.get());  // Session is already closed.
  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

TEST_P(QuicStreamFactoryTest, ServerPushSessionAffinity) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  EXPECT_EQ(0, QuicStreamFactoryPeer::GetNumPushStreamsCreated(factory_.get()));

  string url = "https://www.example.org/";

  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);

  QuicClientPromisedInfo promised(session, GetNthServerInitiatedStreamId(0),
                                  kDefaultUrl);
  (*QuicStreamFactoryPeer::GetPushPromiseIndex(factory_.get())
        ->promised_by_url())[kDefaultUrl] = &promised;

  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(host_port_pair_, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback_.callback()));

  EXPECT_EQ(1, QuicStreamFactoryPeer::GetNumPushStreamsCreated(factory_.get()));
}

TEST_P(QuicStreamFactoryTest, ServerPushPrivacyModeMismatch) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data1;
  socket_data1.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data1.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data1.AddWrite(
      SYNCHRONOUS,
      client_maker_.MakeRstPacket(2, true, GetNthServerInitiatedStreamId(0),
                                  QUIC_STREAM_CANCELLED));
  socket_data1.AddSocketDataToFactory(socket_factory_.get());

  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  EXPECT_EQ(0, QuicStreamFactoryPeer::GetNumPushStreamsCreated(factory_.get()));

  string url = "https://www.example.org/";
  QuicChromiumClientSession* session = GetActiveSession(host_port_pair_);

  QuicClientPromisedInfo promised(session, GetNthServerInitiatedStreamId(0),
                                  kDefaultUrl);

  QuicClientPushPromiseIndex* index =
      QuicStreamFactoryPeer::GetPushPromiseIndex(factory_.get());

  (*index->promised_by_url())[kDefaultUrl] = &promised;
  EXPECT_EQ(index->GetPromised(kDefaultUrl), &promised);

  // Doing the request should not use the push stream, but rather
  // cancel it because the privacy modes do not match.
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(host_port_pair_, version_, PRIVACY_MODE_ENABLED,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));

  EXPECT_EQ(0, QuicStreamFactoryPeer::GetNumPushStreamsCreated(factory_.get()));
  EXPECT_EQ(index->GetPromised(kDefaultUrl), nullptr);

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  EXPECT_TRUE(socket_data1.AllReadDataConsumed());
  EXPECT_TRUE(socket_data1.AllWriteDataConsumed());
  EXPECT_TRUE(socket_data2.AllReadDataConsumed());
  EXPECT_TRUE(socket_data2.AllWriteDataConsumed());
}

// Pool to existing session with matching QuicServerId
// even if destination is different.
TEST_P(QuicStreamFactoryTest, PoolByOrigin) {
  Initialize();

  HostPortPair destination1("first.example.com", 443);
  HostPortPair destination2("second.example.com", 443);

  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request1(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request1.Request(destination1, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url_, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream1 = CreateStream(&request1);
  EXPECT_TRUE(stream1.get());
  EXPECT_TRUE(HasActiveSession(host_port_pair_));

  // Second request returns synchronously because it pools to existing session.
  TestCompletionCallback callback2;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(destination2, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url_, net_log_,
                                 &net_error_details_, callback2.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  QuicChromiumClientSession::Handle* session1 =
      QuicHttpStreamPeer::GetSessionHandle(stream1.get());
  QuicChromiumClientSession::Handle* session2 =
      QuicHttpStreamPeer::GetSessionHandle(stream2.get());
  EXPECT_TRUE(session1->SharesSameSession(*session2));
  EXPECT_EQ(QuicServerId(host_port_pair_, privacy_mode_),
            session1->server_id());

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

class QuicStreamFactoryWithDestinationTest
    : public QuicStreamFactoryTestBase,
      public ::testing::TestWithParam<PoolingTestParams> {
 protected:
  QuicStreamFactoryWithDestinationTest()
      : QuicStreamFactoryTestBase(
            GetParam().version,
            GetParam().client_headers_include_h2_stream_dependency),
        destination_type_(GetParam().destination_type),
        hanging_read_(SYNCHRONOUS, ERR_IO_PENDING, 0) {}

  HostPortPair GetDestination() {
    switch (destination_type_) {
      case SAME_AS_FIRST:
        return origin1_;
      case SAME_AS_SECOND:
        return origin2_;
      case DIFFERENT:
        return HostPortPair(kDifferentHostname, 443);
      default:
        NOTREACHED();
        return HostPortPair();
    }
  }

  void AddHangingSocketData() {
    std::unique_ptr<SequencedSocketData> sequenced_socket_data(
        new SequencedSocketData(base::make_span(&hanging_read_, 1),
                                base::span<MockWrite>()));
    socket_factory_->AddSocketDataProvider(sequenced_socket_data.get());
    sequenced_socket_data_vector_.push_back(std::move(sequenced_socket_data));
  }

  bool AllDataConsumed() {
    for (const auto& socket_data_ptr : sequenced_socket_data_vector_) {
      if (!socket_data_ptr->AllReadDataConsumed() ||
          !socket_data_ptr->AllWriteDataConsumed()) {
        return false;
      }
    }
    return true;
  }

  DestinationType destination_type_;
  HostPortPair origin1_;
  HostPortPair origin2_;
  MockRead hanging_read_;
  std::vector<std::unique_ptr<SequencedSocketData>>
      sequenced_socket_data_vector_;
};

INSTANTIATE_TEST_CASE_P(VersionIncludeStreamDependencySequence,
                        QuicStreamFactoryWithDestinationTest,
                        ::testing::ValuesIn(GetPoolingTestParams()));

// A single QUIC request fails because the certificate does not match the origin
// hostname, regardless of whether it matches the alternative service hostname.
TEST_P(QuicStreamFactoryWithDestinationTest, InvalidCertificate) {
  if (destination_type_ == DIFFERENT)
    return;

  Initialize();

  GURL url("https://mail.example.com/");
  origin1_ = HostPortPair::FromURL(url);

  // Not used for requests, but this provides a test case where the certificate
  // is valid for the hostname of the alternative service.
  origin2_ = HostPortPair("mail.example.org", 433);

  HostPortPair destination = GetDestination();

  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "wildcard.pem"));
  ASSERT_FALSE(cert->VerifyNameMatch(origin1_.host()));
  ASSERT_TRUE(cert->VerifyNameMatch(origin2_.host()));

  ProofVerifyDetailsChromium verify_details;
  verify_details.cert_verify_result.verified_cert = cert;
  verify_details.cert_verify_result.is_issued_by_known_root = true;
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  AddHangingSocketData();

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(destination, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsError(ERR_QUIC_HANDSHAKE_FAILED));

  EXPECT_TRUE(AllDataConsumed());
}

// QuicStreamRequest is pooled based on |destination| if certificate matches.
TEST_P(QuicStreamFactoryWithDestinationTest, SharedCertificate) {
  Initialize();

  GURL url1("https://www.example.org/");
  GURL url2("https://mail.example.org/");
  origin1_ = HostPortPair::FromURL(url1);
  origin2_ = HostPortPair::FromURL(url2);

  HostPortPair destination = GetDestination();

  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "wildcard.pem"));
  ASSERT_TRUE(cert->VerifyNameMatch(origin1_.host()));
  ASSERT_TRUE(cert->VerifyNameMatch(origin2_.host()));
  ASSERT_FALSE(cert->VerifyNameMatch(kDifferentHostname));

  ProofVerifyDetailsChromium verify_details;
  verify_details.cert_verify_result.verified_cert = cert;
  verify_details.cert_verify_result.is_issued_by_known_root = true;
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  std::unique_ptr<QuicEncryptedPacket> settings_packet(
      client_maker_.MakeInitialSettingsPacket(1, nullptr));
  MockWrite writes[] = {MockWrite(SYNCHRONOUS, settings_packet->data(),
                                  settings_packet->length(), 1)};
  std::unique_ptr<SequencedSocketData> sequenced_socket_data(
      new SequencedSocketData(reads, writes));
  socket_factory_->AddSocketDataProvider(sequenced_socket_data.get());
  sequenced_socket_data_vector_.push_back(std::move(sequenced_socket_data));

  QuicStreamRequest request1(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request1.Request(destination, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url1, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());

  std::unique_ptr<HttpStream> stream1 = CreateStream(&request1);
  EXPECT_TRUE(stream1.get());
  EXPECT_TRUE(HasActiveSession(origin1_));

  // Second request returns synchronously because it pools to existing session.
  TestCompletionCallback callback2;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(OK, request2.Request(destination, version_, privacy_mode_,
                                 DEFAULT_PRIORITY, SocketTag(),
                                 /*cert_verify_flags=*/0, url2, net_log_,
                                 &net_error_details_, callback2.callback()));
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  QuicChromiumClientSession::Handle* session1 =
      QuicHttpStreamPeer::GetSessionHandle(stream1.get());
  QuicChromiumClientSession::Handle* session2 =
      QuicHttpStreamPeer::GetSessionHandle(stream2.get());
  EXPECT_TRUE(session1->SharesSameSession(*session2));

  EXPECT_EQ(QuicServerId(origin1_, privacy_mode_), session1->server_id());

  EXPECT_TRUE(AllDataConsumed());
}

// QuicStreamRequest is not pooled if PrivacyMode differs.
TEST_P(QuicStreamFactoryWithDestinationTest, DifferentPrivacyMode) {
  Initialize();

  GURL url1("https://www.example.org/");
  GURL url2("https://mail.example.org/");
  origin1_ = HostPortPair::FromURL(url1);
  origin2_ = HostPortPair::FromURL(url2);

  HostPortPair destination = GetDestination();

  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "wildcard.pem"));
  ASSERT_TRUE(cert->VerifyNameMatch(origin1_.host()));
  ASSERT_TRUE(cert->VerifyNameMatch(origin2_.host()));
  ASSERT_FALSE(cert->VerifyNameMatch(kDifferentHostname));

  ProofVerifyDetailsChromium verify_details1;
  verify_details1.cert_verify_result.verified_cert = cert;
  verify_details1.cert_verify_result.is_issued_by_known_root = true;
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details1);

  ProofVerifyDetailsChromium verify_details2;
  verify_details2.cert_verify_result.verified_cert = cert;
  verify_details2.cert_verify_result.is_issued_by_known_root = true;
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details2);

  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  std::unique_ptr<QuicEncryptedPacket> settings_packet(
      client_maker_.MakeInitialSettingsPacket(1, nullptr));
  MockWrite writes[] = {MockWrite(SYNCHRONOUS, settings_packet->data(),
                                  settings_packet->length(), 1)};
  std::unique_ptr<SequencedSocketData> sequenced_socket_data(
      new SequencedSocketData(reads, writes));
  socket_factory_->AddSocketDataProvider(sequenced_socket_data.get());
  sequenced_socket_data_vector_.push_back(std::move(sequenced_socket_data));
  std::unique_ptr<SequencedSocketData> sequenced_socket_data1(
      new SequencedSocketData(reads, writes));
  socket_factory_->AddSocketDataProvider(sequenced_socket_data1.get());
  sequenced_socket_data_vector_.push_back(std::move(sequenced_socket_data1));

  QuicStreamRequest request1(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request1.Request(destination, version_, PRIVACY_MODE_DISABLED,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url1, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_EQ(OK, callback_.WaitForResult());
  std::unique_ptr<HttpStream> stream1 = CreateStream(&request1);
  EXPECT_TRUE(stream1.get());
  EXPECT_TRUE(HasActiveSession(origin1_));

  TestCompletionCallback callback2;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(destination, version_, PRIVACY_MODE_ENABLED,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url2, net_log_,
                             &net_error_details_, callback2.callback()));
  EXPECT_EQ(OK, callback2.WaitForResult());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  // |request2| does not pool to the first session, because PrivacyMode does not
  // match.  Instead, another session is opened to the same destination, but
  // with a different QuicServerId.
  QuicChromiumClientSession::Handle* session1 =
      QuicHttpStreamPeer::GetSessionHandle(stream1.get());
  QuicChromiumClientSession::Handle* session2 =
      QuicHttpStreamPeer::GetSessionHandle(stream2.get());
  EXPECT_FALSE(session1->SharesSameSession(*session2));

  EXPECT_EQ(QuicServerId(origin1_, PRIVACY_MODE_DISABLED),
            session1->server_id());
  EXPECT_EQ(QuicServerId(origin2_, PRIVACY_MODE_ENABLED),
            session2->server_id());

  EXPECT_TRUE(AllDataConsumed());
}

// QuicStreamRequest is not pooled if certificate does not match its origin.
TEST_P(QuicStreamFactoryWithDestinationTest, DisjointCertificate) {
  Initialize();

  GURL url1("https://news.example.org/");
  GURL url2("https://mail.example.com/");
  origin1_ = HostPortPair::FromURL(url1);
  origin2_ = HostPortPair::FromURL(url2);

  HostPortPair destination = GetDestination();

  scoped_refptr<X509Certificate> cert1(
      ImportCertFromFile(GetTestCertsDirectory(), "wildcard.pem"));
  ASSERT_TRUE(cert1->VerifyNameMatch(origin1_.host()));
  ASSERT_FALSE(cert1->VerifyNameMatch(origin2_.host()));
  ASSERT_FALSE(cert1->VerifyNameMatch(kDifferentHostname));

  ProofVerifyDetailsChromium verify_details1;
  verify_details1.cert_verify_result.verified_cert = cert1;
  verify_details1.cert_verify_result.is_issued_by_known_root = true;
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details1);

  scoped_refptr<X509Certificate> cert2(
      ImportCertFromFile(GetTestCertsDirectory(), "spdy_pooling.pem"));
  ASSERT_TRUE(cert2->VerifyNameMatch(origin2_.host()));
  ASSERT_FALSE(cert2->VerifyNameMatch(kDifferentHostname));

  ProofVerifyDetailsChromium verify_details2;
  verify_details2.cert_verify_result.verified_cert = cert2;
  verify_details2.cert_verify_result.is_issued_by_known_root = true;
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details2);

  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  std::unique_ptr<QuicEncryptedPacket> settings_packet(
      client_maker_.MakeInitialSettingsPacket(1, nullptr));
  MockWrite writes[] = {MockWrite(SYNCHRONOUS, settings_packet->data(),
                                  settings_packet->length(), 1)};
  std::unique_ptr<SequencedSocketData> sequenced_socket_data(
      new SequencedSocketData(reads, writes));
  socket_factory_->AddSocketDataProvider(sequenced_socket_data.get());
  sequenced_socket_data_vector_.push_back(std::move(sequenced_socket_data));
  std::unique_ptr<SequencedSocketData> sequenced_socket_data1(
      new SequencedSocketData(reads, writes));
  socket_factory_->AddSocketDataProvider(sequenced_socket_data1.get());
  sequenced_socket_data_vector_.push_back(std::move(sequenced_socket_data1));

  QuicStreamRequest request1(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request1.Request(destination, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url1, net_log_,
                             &net_error_details_, callback_.callback()));
  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream1 = CreateStream(&request1);
  EXPECT_TRUE(stream1.get());
  EXPECT_TRUE(HasActiveSession(origin1_));

  TestCompletionCallback callback2;
  QuicStreamRequest request2(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request2.Request(destination, version_, privacy_mode_,
                             DEFAULT_PRIORITY, SocketTag(),
                             /*cert_verify_flags=*/0, url2, net_log_,
                             &net_error_details_, callback2.callback()));
  EXPECT_THAT(callback2.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream2 = CreateStream(&request2);
  EXPECT_TRUE(stream2.get());

  // |request2| does not pool to the first session, because the certificate does
  // not match.  Instead, another session is opened to the same destination, but
  // with a different QuicServerId.
  QuicChromiumClientSession::Handle* session1 =
      QuicHttpStreamPeer::GetSessionHandle(stream1.get());
  QuicChromiumClientSession::Handle* session2 =
      QuicHttpStreamPeer::GetSessionHandle(stream2.get());
  EXPECT_FALSE(session1->SharesSameSession(*session2));

  EXPECT_EQ(QuicServerId(origin1_, privacy_mode_), session1->server_id());
  EXPECT_EQ(QuicServerId(origin2_, privacy_mode_), session2->server_id());

  EXPECT_TRUE(AllDataConsumed());
}

// This test verifies that QuicStreamFactory::ClearCachedStatesInCryptoConfig
// correctly transform an origin filter to a ServerIdFilter. Whether the
// deletion itself works correctly is tested in QuicCryptoClientConfigTest.
TEST_P(QuicStreamFactoryTest, ClearCachedStatesInCryptoConfig) {
  Initialize();
  QuicCryptoClientConfig* crypto_config =
      QuicStreamFactoryPeer::GetCryptoConfig(factory_.get());

  struct TestCase {
    TestCase(const std::string& host,
             int port,
             PrivacyMode privacy_mode,
             QuicCryptoClientConfig* crypto_config)
        : server_id(host, port, privacy_mode),
          state(crypto_config->LookupOrCreate(server_id)) {
      std::vector<string> certs(1);
      certs[0] = "cert";
      state->SetProof(certs, "cert_sct", "chlo_hash", "signature");
      state->set_source_address_token("TOKEN");
      state->SetProofValid();

      EXPECT_FALSE(state->certs().empty());
    }

    QuicServerId server_id;
    QuicCryptoClientConfig::CachedState* state;
  } test_cases[] = {
      TestCase("www.google.com", 443, privacy_mode_, crypto_config),
      TestCase("www.example.com", 443, privacy_mode_, crypto_config),
      TestCase("www.example.com", 4433, privacy_mode_, crypto_config)};

  // Clear cached states for the origin https://www.example.com:4433.
  GURL origin("https://www.example.com:4433");
  factory_->ClearCachedStatesInCryptoConfig(base::Bind(
      static_cast<bool (*)(const GURL&, const GURL&)>(::operator==), origin));
  EXPECT_FALSE(test_cases[0].state->certs().empty());
  EXPECT_FALSE(test_cases[1].state->certs().empty());
  EXPECT_TRUE(test_cases[2].state->certs().empty());

  // Clear all cached states.
  factory_->ClearCachedStatesInCryptoConfig(
      base::Callback<bool(const GURL&)>());
  EXPECT_TRUE(test_cases[0].state->certs().empty());
  EXPECT_TRUE(test_cases[1].state->certs().empty());
  EXPECT_TRUE(test_cases[2].state->certs().empty());
}

// Passes connection options and client connection options to QuicStreamFactory,
// then checks that its internal QuicConfig is correct.
TEST_P(QuicStreamFactoryTest, ConfigConnectionOptions) {
  connection_options_.push_back(net::kTIME);
  connection_options_.push_back(net::kTBBR);
  connection_options_.push_back(net::kREJ);

  client_connection_options_.push_back(net::kTBBR);
  client_connection_options_.push_back(net::k1RTT);

  Initialize();

  const QuicConfig* config = QuicStreamFactoryPeer::GetConfig(factory_.get());
  EXPECT_EQ(connection_options_, config->SendConnectionOptions());
  EXPECT_TRUE(config->HasClientRequestedIndependentOption(
      net::kTBBR, Perspective::IS_CLIENT));
  EXPECT_TRUE(config->HasClientRequestedIndependentOption(
      net::k1RTT, Perspective::IS_CLIENT));
}

// Verifies that the host resolver uses the request priority passed to
// QuicStreamRequest::Request().
TEST_P(QuicStreamFactoryTest, HostResolverUsesRequestPriority) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            MAXIMUM_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  EXPECT_THAT(callback_.WaitForResult(), IsOk());
  std::unique_ptr<HttpStream> stream = CreateStream(&request);
  EXPECT_TRUE(stream.get());

  EXPECT_EQ(MAXIMUM_PRIORITY, host_resolver_.last_request_priority());

  EXPECT_TRUE(socket_data.AllReadDataConsumed());
  EXPECT_TRUE(socket_data.AllWriteDataConsumed());
}

// Passes |max_time_before_crypto_handshake_seconds| and
// |max_idle_time_before_crypto_handshake_seconds| to QuicStreamFactory, then
// checks that its internal QuicConfig is correct.
TEST_P(QuicStreamFactoryTest, ConfigMaxTimeBeforeCryptoHandshake) {
  max_time_before_crypto_handshake_seconds_ = 11;
  max_idle_time_before_crypto_handshake_seconds_ = 13;

  Initialize();

  const QuicConfig* config = QuicStreamFactoryPeer::GetConfig(factory_.get());
  EXPECT_EQ(QuicTime::Delta::FromSeconds(11),
            config->max_time_before_crypto_handshake());
  EXPECT_EQ(QuicTime::Delta::FromSeconds(13),
            config->max_idle_time_before_crypto_handshake());
}

// Verify ResultAfterHostResolutionCallback behavior when host resolution
// succeeds asynchronously, then crypto handshake fails synchronously.
TEST_P(QuicStreamFactoryTest, ResultAfterHostResolutionCallbackAsyncSync) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.set_ondemand_mode(true);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_FAILED);
  socket_data.AddWrite(SYNCHRONOUS, ERR_FAILED);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  TestCompletionCallback host_resolution_callback;
  EXPECT_TRUE(
      request.WaitForHostResolution(host_resolution_callback.callback()));

  // |host_resolver_| has not finished host resolution at this point, so
  // |host_resolution_callback| should not have a result.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(host_resolution_callback.have_result());

  // Allow |host_resolver_| to finish host resolution.
  // Since the request fails immediately after host resolution (getting
  // ERR_FAILED from socket reads/writes), |host_resolution_callback| should be
  // called with ERR_QUIC_PROTOCOL_ERROR since that's the next result in
  // forming the connection.
  host_resolver_.ResolveAllPending();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(host_resolution_callback.have_result());
  EXPECT_EQ(ERR_QUIC_PROTOCOL_ERROR, host_resolution_callback.WaitForResult());

  // Calling WaitForHostResolution() a second time should return
  // false since host resolution has finished already.
  EXPECT_FALSE(
      request.WaitForHostResolution(host_resolution_callback.callback()));

  EXPECT_TRUE(callback_.have_result());
  EXPECT_EQ(ERR_QUIC_PROTOCOL_ERROR, callback_.WaitForResult());
}

// Verify ResultAfterHostResolutionCallback behavior when host resolution
// succeeds asynchronously, then crypto handshake fails asynchronously.
TEST_P(QuicStreamFactoryTest, ResultAfterHostResolutionCallbackAsyncAsync) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.set_ondemand_mode(true);
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  factory_->set_require_confirmation(true);

  MockQuicData socket_data;
  socket_data.AddRead(ASYNC, ERR_IO_PENDING);  // Pause
  socket_data.AddRead(ASYNC, ERR_FAILED);
  socket_data.AddWrite(ASYNC, ERR_FAILED);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  TestCompletionCallback host_resolution_callback;
  EXPECT_TRUE(
      request.WaitForHostResolution(host_resolution_callback.callback()));

  // |host_resolver_| has not finished host resolution at this point, so
  // |host_resolution_callback| should not have a result.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(host_resolution_callback.have_result());

  // Allow |host_resolver_| to finish host resolution. Since crypto handshake
  // will hang after host resolution, |host_resolution_callback| should run with
  // ERR_IO_PENDING since that's the next result in forming the connection.
  host_resolver_.ResolveAllPending();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(host_resolution_callback.have_result());
  EXPECT_EQ(ERR_IO_PENDING, host_resolution_callback.WaitForResult());

  // Calling WaitForHostResolution() a second time should return
  // false since host resolution has finished already.
  EXPECT_FALSE(
      request.WaitForHostResolution(host_resolution_callback.callback()));

  EXPECT_FALSE(callback_.have_result());
  socket_data.GetSequencedSocketData()->Resume();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_.have_result());
  EXPECT_EQ(ERR_QUIC_PROTOCOL_ERROR, callback_.WaitForResult());
}

// Verify ResultAfterHostResolutionCallback behavior when host resolution
// succeeds synchronously, then crypto handshake fails synchronously.
TEST_P(QuicStreamFactoryTest, ResultAfterHostResolutionCallbackSyncSync) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.set_synchronous_mode(true);

  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_FAILED);
  socket_data.AddWrite(SYNCHRONOUS, ERR_FAILED);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_QUIC_PROTOCOL_ERROR,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  // WaitForHostResolution() should return false since host
  // resolution has finished already.
  TestCompletionCallback host_resolution_callback;
  EXPECT_FALSE(
      request.WaitForHostResolution(host_resolution_callback.callback()));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(host_resolution_callback.have_result());
  EXPECT_FALSE(callback_.have_result());
}

// Verify ResultAfterHostResolutionCallback behavior when host resolution
// succeeds synchronously, then crypto handshake fails asynchronously.
TEST_P(QuicStreamFactoryTest, ResultAfterHostResolutionCallbackSyncAsync) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Host resolution will succeed synchronously, but Request() as a whole
  // will fail asynchronously.
  host_resolver_.set_synchronous_mode(true);
  crypto_client_stream_factory_.set_handshake_mode(
      MockCryptoClientStream::ZERO_RTT);
  factory_->set_require_confirmation(true);

  MockQuicData socket_data;
  socket_data.AddRead(ASYNC, ERR_IO_PENDING);  // Pause
  socket_data.AddRead(ASYNC, ERR_FAILED);
  socket_data.AddWrite(ASYNC, ERR_FAILED);
  socket_data.AddSocketDataToFactory(socket_factory_.get());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  // WaitForHostResolution() should return false since host
  // resolution has finished already.
  TestCompletionCallback host_resolution_callback;
  EXPECT_FALSE(
      request.WaitForHostResolution(host_resolution_callback.callback()));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(host_resolution_callback.have_result());

  EXPECT_FALSE(callback_.have_result());
  socket_data.GetSequencedSocketData()->Resume();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_.have_result());
  EXPECT_EQ(ERR_QUIC_PROTOCOL_ERROR, callback_.WaitForResult());
}

// Verify ResultAfterHostResolutionCallback behavior when host resolution fails
// synchronously.
TEST_P(QuicStreamFactoryTest, ResultAfterHostResolutionCallbackFailSync) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Host resolution will fail synchronously.
  host_resolver_.rules()->AddSimulatedFailure(host_port_pair_.host());
  host_resolver_.set_synchronous_mode(true);

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  // WaitForHostResolution() should return false since host
  // resolution has failed already.
  TestCompletionCallback host_resolution_callback;
  EXPECT_FALSE(
      request.WaitForHostResolution(host_resolution_callback.callback()));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(host_resolution_callback.have_result());
}

// Verify ResultAfterHostResolutionCallback behavior when host resolution fails
// asynchronously.
TEST_P(QuicStreamFactoryTest, ResultAfterHostResolutionCallbackFailAsync) {
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  host_resolver_.rules()->AddSimulatedFailure(host_port_pair_.host());

  QuicStreamRequest request(factory_.get());
  EXPECT_EQ(ERR_IO_PENDING,
            request.Request(host_port_pair_, version_, privacy_mode_,
                            DEFAULT_PRIORITY, SocketTag(),
                            /*cert_verify_flags=*/0, url_, net_log_,
                            &net_error_details_, callback_.callback()));

  TestCompletionCallback host_resolution_callback;
  EXPECT_TRUE(
      request.WaitForHostResolution(host_resolution_callback.callback()));

  // Allow |host_resolver_| to fail host resolution. |host_resolution_callback|
  // Should run with ERR_NAME_NOT_RESOLVED since that's the error host
  // resolution failed with.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(host_resolution_callback.have_result());
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, host_resolution_callback.WaitForResult());

  EXPECT_TRUE(callback_.have_result());
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, callback_.WaitForResult());
}

// Test that QuicStreamRequests with similar and different tags results in
// reused and unique QUIC streams using appropriately tagged sockets.
TEST_P(QuicStreamFactoryTest, Tag) {
  MockTaggingClientSocketFactory* socket_factory =
      new MockTaggingClientSocketFactory();
  socket_factory_.reset(socket_factory);
  Initialize();
  ProofVerifyDetailsChromium verify_details = DefaultProofVerifyDetails();
  crypto_client_stream_factory_.AddProofVerifyDetails(&verify_details);

  // Prepare to establish two QUIC sessions.
  MockQuicData socket_data;
  socket_data.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data.AddSocketDataToFactory(socket_factory_.get());
  MockQuicData socket_data2;
  socket_data2.AddRead(SYNCHRONOUS, ERR_IO_PENDING);
  socket_data2.AddWrite(SYNCHRONOUS, ConstructInitialSettingsPacket());
  socket_data2.AddSocketDataToFactory(socket_factory_.get());

#if defined(OS_ANDROID)
  SocketTag tag1(SocketTag::UNSET_UID, 0x12345678);
  SocketTag tag2(getuid(), 0x87654321);
#else
  // On non-Android platforms we can only use the default constructor.
  SocketTag tag1, tag2;
#endif

  // Request a stream with |tag1|.
  QuicStreamRequest request1(factory_.get());
  int rv =
      request1.Request(host_port_pair_, version_, privacy_mode_,
                       DEFAULT_PRIORITY, tag1, /*cert_verify_flags=*/0, url_,
                       net_log_, &net_error_details_, callback_.callback());
  EXPECT_THAT(callback_.GetResult(rv), IsOk());
  EXPECT_EQ(socket_factory->GetLastProducedUDPSocket()->tag(), tag1);
  EXPECT_TRUE(socket_factory->GetLastProducedUDPSocket()
                  ->tagged_before_data_transferred());
  std::unique_ptr<QuicChromiumClientSession::Handle> stream1 =
      request1.ReleaseSessionHandle();
  EXPECT_TRUE(stream1);
  EXPECT_TRUE(stream1->IsConnected());

  // Request a stream with |tag1| and verify underlying session is reused.
  QuicStreamRequest request2(factory_.get());
  rv = request2.Request(host_port_pair_, version_, privacy_mode_,
                        DEFAULT_PRIORITY, tag1,
                        /*cert_verify_flags=*/0, url_, net_log_,
                        &net_error_details_, callback_.callback());
  EXPECT_THAT(callback_.GetResult(rv), IsOk());
  std::unique_ptr<QuicChromiumClientSession::Handle> stream2 =
      request2.ReleaseSessionHandle();
  EXPECT_TRUE(stream2);
  EXPECT_TRUE(stream2->IsConnected());
  EXPECT_TRUE(stream2->SharesSameSession(*stream1));

  // Request a stream with |tag2| and verify a new session is created.
  QuicStreamRequest request3(factory_.get());
  rv = request3.Request(host_port_pair_, version_, privacy_mode_,
                        DEFAULT_PRIORITY, tag2,
                        /*cert_verify_flags=*/0, url_, net_log_,
                        &net_error_details_, callback_.callback());
  EXPECT_THAT(callback_.GetResult(rv), IsOk());
  EXPECT_EQ(socket_factory->GetLastProducedUDPSocket()->tag(), tag2);
  EXPECT_TRUE(socket_factory->GetLastProducedUDPSocket()
                  ->tagged_before_data_transferred());
  std::unique_ptr<QuicChromiumClientSession::Handle> stream3 =
      request3.ReleaseSessionHandle();
  EXPECT_TRUE(stream3);
  EXPECT_TRUE(stream3->IsConnected());
#if defined(OS_ANDROID)
  EXPECT_FALSE(stream3->SharesSameSession(*stream1));
#else
  // Same tag should reuse session.
  EXPECT_TRUE(stream3->SharesSameSession(*stream1));
#endif
}

}  // namespace test
}  // namespace net
