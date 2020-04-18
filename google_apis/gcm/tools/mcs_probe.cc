// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A standalone tool for testing MCS connections and the MCS client on their
// own.

#include <stdint.h>

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "build/build_config.h"
#include "google_apis/gcm/base/fake_encryptor.h"
#include "google_apis/gcm/base/mcs_message.h"
#include "google_apis/gcm/base/mcs_util.h"
#include "google_apis/gcm/engine/checkin_request.h"
#include "google_apis/gcm/engine/connection_factory_impl.h"
#include "google_apis/gcm/engine/gcm_store_impl.h"
#include "google_apis/gcm/engine/gservices_settings.h"
#include "google_apis/gcm/engine/mcs_client.h"
#include "google_apis/gcm/monitoring/fake_gcm_stats_recorder.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/transport_security_state.h"
#include "net/log/file_net_log_observer.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/url_request/url_request_test_util.h"

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif

// This is a simple utility that initializes an mcs client and
// prints out any events.
namespace gcm {
namespace {

const net::BackoffEntry::Policy kDefaultBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential back-off in ms.
  15000,  // 15 seconds.

  // Factor by which the waiting time will be multiplied.
  2,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0.5,  // 50%.

  // Maximum amount of time we are willing to delay our request in ms.
  1000 * 60 * 5, // 5 minutes.

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

// Default values used to communicate with the check-in server.
const char kChromeVersion[] = "Chrome MCS Probe";

// The default server to communicate with.
const char kMCSServerHost[] = "mtalk.google.com";
const uint16_t kMCSServerPort = 5228;

// Command line switches.
const char kRMQFileName[] = "rmq_file";
const char kAndroidIdSwitch[] = "android_id";
const char kSecretSwitch[] = "secret";
const char kLogFileSwitch[] = "log-file";
const char kIgnoreCertSwitch[] = "ignore-certs";
const char kServerHostSwitch[] = "host";
const char kServerPortSwitch[] = "port";

void MessageReceivedCallback(const MCSMessage& message) {
  LOG(INFO) << "Received message with id "
            << GetPersistentId(message.GetProtobuf()) << " and tag "
            << static_cast<int>(message.tag());

  if (message.tag() == kDataMessageStanzaTag) {
    const mcs_proto::DataMessageStanza& data_message =
        reinterpret_cast<const mcs_proto::DataMessageStanza&>(
            message.GetProtobuf());
    DVLOG(1) << "  to: " << data_message.to();
    DVLOG(1) << "  from: " << data_message.from();
    DVLOG(1) << "  category: " << data_message.category();
    DVLOG(1) << "  sent: " << data_message.sent();
    for (int i = 0; i < data_message.app_data_size(); ++i) {
      DVLOG(1) << "  App data " << i << " "
               << data_message.app_data(i).key() << " : "
               << data_message.app_data(i).value();
    }
  }
}

void MessageSentCallback(int64_t user_serial_number,
                         const std::string& app_id,
                         const std::string& message_id,
                         MCSClient::MessageSendStatus status) {
  LOG(INFO) << "Message sent. Serial number: " << user_serial_number
            << " Application ID: " << app_id
            << " Message ID: " << message_id
            << " Message send status: " << status;
}

// Needed to use a real host resolver.
class MyTestURLRequestContext : public net::TestURLRequestContext {
 public:
  MyTestURLRequestContext() : TestURLRequestContext(true) {
    context_storage_.set_host_resolver(
        net::HostResolver::CreateDefaultResolver(NULL));
    context_storage_.set_transport_security_state(
        std::make_unique<net::TransportSecurityState>());
    Init();
  }

  ~MyTestURLRequestContext() override {}
};

class MyTestURLRequestContextGetter : public net::TestURLRequestContextGetter {
 public:
  explicit MyTestURLRequestContextGetter(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
      : TestURLRequestContextGetter(io_task_runner) {}

  net::TestURLRequestContext* GetURLRequestContext() override {
    // Construct |context_| lazily so it gets constructed on the right
    // thread (the IO thread).
    if (!context_)
      context_.reset(new MyTestURLRequestContext());
    return context_.get();
  }

 private:
  ~MyTestURLRequestContextGetter() override {}

  std::unique_ptr<MyTestURLRequestContext> context_;
};

// A cert verifier that access all certificates.
class MyTestCertVerifier : public net::CertVerifier {
 public:
  MyTestCertVerifier() {}
  ~MyTestCertVerifier() override {}

  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback& callback,
             std::unique_ptr<Request>* out_req,
             const net::NetLogWithSource& net_log) override {
    return net::OK;
  }
};

class MCSProbeAuthPreferences : public net::HttpAuthPreferences {
 public:
  MCSProbeAuthPreferences() {}
  ~MCSProbeAuthPreferences() override {}

  bool IsSupportedScheme(const std::string& scheme) const override {
    return scheme == std::string(net::kBasicAuthScheme);
  }
  bool NegotiateDisableCnameLookup() const override { return false; }
  bool NegotiateEnablePort() const override { return false; }
  bool CanUseDefaultCredentials(const GURL& auth_origin) const override {
    return false;
  }
  bool CanDelegate(const GURL& auth_origin) const override { return false; }
};

class MCSProbe {
 public:
  MCSProbe(
      const base::CommandLine& command_line,
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter);
  ~MCSProbe();

  void Start();

  uint64_t android_id() const { return android_id_; }
  uint64_t secret() const { return secret_; }

 private:
  void CheckIn();
  void InitializeNetworkState();
  void BuildNetworkSession();

  void LoadCallback(std::unique_ptr<GCMStore::LoadResult> load_result);
  void UpdateCallback(bool success);
  void ErrorCallback();
  void OnCheckInCompleted(
      net::HttpStatusCode response_code,
      const checkin_proto::AndroidCheckinResponse& checkin_response);
  void StartMCSLogin();

  base::DefaultClock clock_;

  base::CommandLine command_line_;

  base::FilePath gcm_store_path_;
  uint64_t android_id_;
  uint64_t secret_;
  std::string server_host_;
  int server_port_;

  // Network state.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  net::NetLog net_log_;
  std::unique_ptr<net::FileNetLogObserver> logger_;
  std::unique_ptr<net::HostResolver> host_resolver_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::ChannelIDService> system_channel_id_service_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
  std::unique_ptr<net::CTVerifier> cert_transparency_verifier_;
  std::unique_ptr<net::CTPolicyEnforcer> ct_policy_enforcer_;
  MCSProbeAuthPreferences http_auth_preferences_;
  std::unique_ptr<net::HttpAuthHandlerFactory> http_auth_handler_factory_;
  std::unique_ptr<net::HttpServerPropertiesImpl> http_server_properties_;
  std::unique_ptr<net::HttpNetworkSession> network_session_;
  std::unique_ptr<net::ProxyResolutionService> proxy_resolution_service_;

  FakeGCMStatsRecorder recorder_;
  std::unique_ptr<GCMStore> gcm_store_;
  std::unique_ptr<MCSClient> mcs_client_;
  std::unique_ptr<CheckinRequest> checkin_request_;

  std::unique_ptr<ConnectionFactoryImpl> connection_factory_;

  base::Thread file_thread_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

MCSProbe::MCSProbe(
    const base::CommandLine& command_line,
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter)
    : command_line_(command_line),
      gcm_store_path_(base::FilePath(FILE_PATH_LITERAL("gcm_store"))),
      android_id_(0),
      secret_(0),
      server_port_(0),
      url_request_context_getter_(url_request_context_getter),
      file_thread_("FileThread") {
  if (command_line.HasSwitch(kRMQFileName)) {
    gcm_store_path_ = command_line.GetSwitchValuePath(kRMQFileName);
  }
  if (command_line.HasSwitch(kAndroidIdSwitch)) {
    base::StringToUint64(command_line.GetSwitchValueASCII(kAndroidIdSwitch),
                         &android_id_);
  }
  if (command_line.HasSwitch(kSecretSwitch)) {
    base::StringToUint64(command_line.GetSwitchValueASCII(kSecretSwitch),
                         &secret_);
  }
  server_host_ = kMCSServerHost;
  if (command_line.HasSwitch(kServerHostSwitch)) {
    server_host_ = command_line.GetSwitchValueASCII(kServerHostSwitch);
  }
  server_port_ = kMCSServerPort;
  if (command_line.HasSwitch(kServerPortSwitch)) {
    base::StringToInt(command_line.GetSwitchValueASCII(kServerPortSwitch),
                      &server_port_);
  }
}

MCSProbe::~MCSProbe() {
  if (logger_)
    logger_->StopObserving(nullptr, base::OnceClosure());
  file_thread_.Stop();
}

void MCSProbe::Start() {
  file_thread_.Start();
  InitializeNetworkState();
  BuildNetworkSession();
  std::vector<GURL> endpoints(
      1, GURL("https://" +
              net::HostPortPair(server_host_, server_port_).ToString()));

  connection_factory_ = std::make_unique<ConnectionFactoryImpl>(
      endpoints, kDefaultBackoffPolicy, network_session_.get(), nullptr,
      &net_log_, &recorder_);
  gcm_store_ = std::make_unique<GCMStoreImpl>(
      gcm_store_path_, file_thread_.task_runner(),
      std::make_unique<FakeEncryptor>());

  mcs_client_ =
      std::make_unique<MCSClient>("probe", &clock_, connection_factory_.get(),
                                  gcm_store_.get(), &recorder_);
  run_loop_ = std::make_unique<base::RunLoop>();
  gcm_store_->Load(GCMStore::CREATE_IF_MISSING,
                   base::Bind(&MCSProbe::LoadCallback,
                              base::Unretained(this)));
  run_loop_->Run();
}

void MCSProbe::LoadCallback(std::unique_ptr<GCMStore::LoadResult> load_result) {
  DCHECK(load_result->success);
  if (android_id_ != 0 && secret_ != 0) {
    DVLOG(1) << "Presetting MCS id " << android_id_;
    load_result->device_android_id = android_id_;
    load_result->device_security_token = secret_;
    gcm_store_->SetDeviceCredentials(android_id_,
                                     secret_,
                                     base::Bind(&MCSProbe::UpdateCallback,
                                                base::Unretained(this)));
  } else {
    android_id_ = load_result->device_android_id;
    secret_ = load_result->device_security_token;
    DVLOG(1) << "Loaded MCS id " << android_id_;
  }
  mcs_client_->Initialize(
      base::Bind(&MCSProbe::ErrorCallback, base::Unretained(this)),
      base::Bind(&MessageReceivedCallback), base::Bind(&MessageSentCallback),
      std::move(load_result));

  if (!android_id_ || !secret_) {
    DVLOG(1) << "Checkin to generate new MCS credentials.";
    CheckIn();
    return;
  }

  StartMCSLogin();
}

void MCSProbe::UpdateCallback(bool success) {
}

void MCSProbe::InitializeNetworkState() {
  if (command_line_.HasSwitch(kLogFileSwitch)) {
    base::FilePath log_path = command_line_.GetSwitchValuePath(kLogFileSwitch);
    logger_ = net::FileNetLogObserver::CreateUnbounded(log_path, nullptr);
    net::NetLogCaptureMode capture_mode =
        net::NetLogCaptureMode::IncludeCookiesAndCredentials();
    logger_->StartObserving(&net_log_, capture_mode);
  }

  host_resolver_ = net::HostResolver::CreateDefaultResolver(&net_log_);

  if (command_line_.HasSwitch(kIgnoreCertSwitch)) {
    cert_verifier_ = std::make_unique<MyTestCertVerifier>();
  } else {
    cert_verifier_ = net::CertVerifier::CreateDefault();
  }
  system_channel_id_service_ = std::make_unique<net::ChannelIDService>(
      new net::DefaultChannelIDStore(nullptr));

  transport_security_state_ = std::make_unique<net::TransportSecurityState>();
  cert_transparency_verifier_ = std::make_unique<net::MultiLogCTVerifier>();
  ct_policy_enforcer_ = std::make_unique<net::DefaultCTPolicyEnforcer>();
  http_auth_handler_factory_ = net::HttpAuthHandlerRegistryFactory::Create(
      &http_auth_preferences_, host_resolver_.get());
  http_server_properties_ = std::make_unique<net::HttpServerPropertiesImpl>();
  proxy_resolution_service_ =
      net::ProxyResolutionService::CreateDirectWithNetLog(&net_log_);
}

void MCSProbe::BuildNetworkSession() {
  net::HttpNetworkSession::Params session_params;
  session_params.ignore_certificate_errors = true;
  session_params.testing_fixed_http_port = 0;
  session_params.testing_fixed_https_port = 0;

  net::HttpNetworkSession::Context session_context;
  session_context.host_resolver = host_resolver_.get();
  session_context.cert_verifier = cert_verifier_.get();
  session_context.channel_id_service = system_channel_id_service_.get();
  session_context.transport_security_state = transport_security_state_.get();
  session_context.cert_transparency_verifier =
      cert_transparency_verifier_.get();
  session_context.ct_policy_enforcer = ct_policy_enforcer_.get();
  session_context.ssl_config_service = new net::SSLConfigServiceDefaults();
  session_context.http_auth_handler_factory = http_auth_handler_factory_.get();
  session_context.http_server_properties = http_server_properties_.get();
  session_context.net_log = &net_log_;
  session_context.proxy_resolution_service = proxy_resolution_service_.get();

  network_session_ = std::make_unique<net::HttpNetworkSession>(session_params,
                                                               session_context);
}

void MCSProbe::ErrorCallback() {
  LOG(INFO) << "MCS error happened";
}

void MCSProbe::CheckIn() {
  LOG(INFO) << "Check-in request initiated.";
  checkin_proto::ChromeBuildProto chrome_build_proto;
  chrome_build_proto.set_platform(
      checkin_proto::ChromeBuildProto::PLATFORM_LINUX);
  chrome_build_proto.set_channel(
      checkin_proto::ChromeBuildProto::CHANNEL_CANARY);
  chrome_build_proto.set_chrome_version(kChromeVersion);

  CheckinRequest::RequestInfo request_info(0, 0,
                                           std::map<std::string, std::string>(),
                                           std::string(), chrome_build_proto);

  checkin_request_ = std::make_unique<CheckinRequest>(
      GServicesSettings().GetCheckinURL(), request_info, kDefaultBackoffPolicy,
      base::Bind(&MCSProbe::OnCheckInCompleted, base::Unretained(this)),
      url_request_context_getter_.get(), &recorder_);
  checkin_request_->Start();
}

void MCSProbe::OnCheckInCompleted(
    net::HttpStatusCode response_code,
    const checkin_proto::AndroidCheckinResponse& checkin_response) {
  bool success = response_code == net::HTTP_OK &&
                 checkin_response.has_android_id() &&
                 checkin_response.android_id() != 0UL &&
                 checkin_response.has_security_token() &&
                 checkin_response.security_token() != 0UL;
  LOG(INFO) << "Check-in request completion "
            << (success ? "success!" : "failure!");

  if (!success)
    return;

  android_id_ = checkin_response.android_id();
  secret_ = checkin_response.security_token();

  gcm_store_->SetDeviceCredentials(android_id_,
                                   secret_,
                                   base::Bind(&MCSProbe::UpdateCallback,
                                              base::Unretained(this)));

  StartMCSLogin();
}

void MCSProbe::StartMCSLogin() {
  LOG(INFO) << "MCS login initiated.";

  mcs_client_->Login(android_id_, secret_);
}

int MCSProbeMain(int argc, char* argv[]) {
  base::AtExitManager exit_manager;

  base::CommandLine::Init(argc, argv);
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

  base::MessageLoopForIO message_loop;
  base::TaskScheduler::CreateAndStartWithDefaultParams("MCSProbe");

  // For check-in and creating registration ids.
  const scoped_refptr<MyTestURLRequestContextGetter> context_getter =
      new MyTestURLRequestContextGetter(base::ThreadTaskRunnerHandle::Get());

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  MCSProbe mcs_probe(command_line, context_getter);
  mcs_probe.Start();

  base::RunLoop run_loop;
  run_loop.Run();

  base::TaskScheduler::GetInstance()->Shutdown();

  return 0;
}

}  // namespace
}  // namespace gcm

int main(int argc, char* argv[]) {
  return gcm::MCSProbeMain(argc, argv);
}
