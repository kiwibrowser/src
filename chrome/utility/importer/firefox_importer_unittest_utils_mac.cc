// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/importer/firefox_importer_unittest_utils.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/posix/global_descriptors.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/importer/firefox_importer_utils.h"
#include "chrome/utility/importer/firefox_importer_unittest_utils_mac.mojom.h"
#include "content/public/common/content_descriptors.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/embedder/descriptors.h"
#include "testing/multiprocess_func_list.h"

namespace {

constexpr char kMojoChannelToken[] = "mojo-channel-token";

// Launch the child process:
// |nss_path| - path to the NSS directory holding the decryption libraries.
// |mojo_handle| - platform handle for Mojo transport.
// |mojo_channel_token| - token for creating the Mojo pipe.
base::Process LaunchNSSDecrypterChildProcess(
    const base::FilePath& nss_path,
    mojo::edk::ScopedInternalPlatformHandle mojo_handle,
    const std::string& mojo_channel_token) {
  base::CommandLine cl(*base::CommandLine::ForCurrentProcess());
  cl.AppendSwitchASCII(switches::kTestChildProcess, "NSSDecrypterChildProcess");
  cl.AppendSwitchASCII(kMojoChannelToken, mojo_channel_token);

  // Set env variable needed for FF encryption libs to load.
  // See "chrome/utility/importer/nss_decryptor_mac.mm" for an explanation of
  // why we need this.
  base::LaunchOptions options;
  options.environ["DYLD_FALLBACK_LIBRARY_PATH"] = nss_path.value();
  options.fds_to_remap.push_back(std::pair<int, int>(
      mojo_handle.get().handle, service_manager::kMojoIPCChannel +
                                    base::GlobalDescriptors::kBaseDescriptor));

  return base::LaunchProcess(cl.argv(), options);
}

//---------------------------- Child Process -----------------------

// Class to listen on the client side of the mojo pipe, it calls through
// to the NSSDecryptor and sends back a reply.
class FFDecryptorClientListener
    : public firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptor {
 public:
  explicit FFDecryptorClientListener(
      firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptorRequest
          request)
      : binding_(this, std::move(request)) {}

  void SetQuitClosure(base::Closure quit_closure) {
    binding_.set_connection_error_handler(std::move(quit_closure));
  }

  void Init(const base::FilePath& dll_path,
            const base::FilePath& db_path,
            InitCallback callback) override {
    std::move(callback).Run(decryptor_.Init(dll_path, db_path));
  }

  void Decrypt(const std::string& crypt, DecryptCallback callback) override {
    base::string16 unencrypted_str = decryptor_.Decrypt(crypt);
    std::move(callback).Run(unencrypted_str);
  }

  void ParseSignons(const base::FilePath& sqlite_file,
                    ParseSignonsCallback callback) override {
    std::vector<autofill::PasswordForm> forms;
    decryptor_.ReadAndParseSignons(sqlite_file, &forms);
    std::move(callback).Run(forms);
  }

 private:
  NSSDecryptor decryptor_;
  mojo::Binding<FirefoxDecryptor> binding_;

  DISALLOW_COPY_AND_ASSIGN(FFDecryptorClientListener);
};

}  // namespace

//----------------------- Server --------------------

// Class to communicate on the server side of the mojo pipe.
// Method calls are sent over mojo and replies are read back into class
// variables.
// This class needs to be called on a single thread.
class FFDecryptorServerChannelListener {
 public:
  explicit FFDecryptorServerChannelListener(
      firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptorPtr decryptor)
      : decryptor_(std::move(decryptor)) {}

  void InitDecryptor(const base::FilePath& dll_path,
                     const base::FilePath& db_path) {
    base::RunLoop run_loop;
    got_result_ = false;
    decryptor_->Init(
        dll_path, db_path,
        base::BindOnce(&FFDecryptorServerChannelListener::InitDecryptorReply,
                       base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  void Decrypt(const std::string& crypt) {
    base::RunLoop run_loop;
    got_result_ = false;
    decryptor_->Decrypt(
        crypt, base::BindOnce(&FFDecryptorServerChannelListener::DecryptReply,
                              base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  void ParseSignons(const base::FilePath& signons_path) {
    base::RunLoop run_loop;
    got_result_ = false;
    decryptor_->ParseSignons(
        signons_path,
        base::BindOnce(&FFDecryptorServerChannelListener::ParseSignonsReply,
                       base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Results of the calls.
  base::string16 result_string_;
  std::vector<autofill::PasswordForm> result_vector_;
  bool result_bool_;
  // True if mojo call succeeded and data in above variables is valid.
  bool got_result_;

 private:
  void InitDecryptorReply(base::Closure quit_closure, bool result) {
    result_bool_ = result;
    got_result_ = true;
    quit_closure.Run();
  }

  void DecryptReply(base::Closure quit_closure, const base::string16& text) {
    result_string_ = text;
    got_result_ = true;
    quit_closure.Run();
  }

  void ParseSignonsReply(base::Closure quit_closure,
                         const std::vector<autofill::PasswordForm>& forms) {
    result_vector_ = forms;
    got_result_ = true;
    quit_closure.Run();
  }

  firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptorPtr decryptor_;

  DISALLOW_COPY_AND_ASSIGN(FFDecryptorServerChannelListener);
};

FFUnitTestDecryptorProxy::FFUnitTestDecryptorProxy() {
}

bool FFUnitTestDecryptorProxy::Setup(const base::FilePath& nss_path) {
  // Create a new message loop and spawn the child process.
  message_loop_ = std::make_unique<base::MessageLoopForIO>();

  mojo::edk::OutgoingBrokerClientInvitation invitation;
  std::string token = mojo::edk::GenerateRandomToken();
  mojo::ScopedMessagePipeHandle parent_pipe =
      invitation.AttachMessagePipe(token);
  firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptorPtr decryptor(
      firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptorPtrInfo(
          std::move(parent_pipe), 0));
  listener_ =
      std::make_unique<FFDecryptorServerChannelListener>(std::move(decryptor));

  // Spawn child and set up mojo connection.
  mojo::edk::PlatformChannelPair channel_pair;
  child_process_ = LaunchNSSDecrypterChildProcess(
      nss_path, channel_pair.PassClientHandle(), token);
  if (child_process_.IsValid()) {
    invitation.Send(
        child_process_.Handle(),
        mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                    channel_pair.PassServerHandle()));
  }
  return child_process_.IsValid();
}

FFUnitTestDecryptorProxy::~FFUnitTestDecryptorProxy() {
  listener_.reset();

  if (child_process_.IsValid()) {
    int exit_code;
    child_process_.WaitForExitWithTimeout(base::TimeDelta::FromSeconds(5),
                                          &exit_code);
  }
}

bool FFUnitTestDecryptorProxy::DecryptorInit(const base::FilePath& dll_path,
                                             const base::FilePath& db_path) {
  listener_->InitDecryptor(dll_path, db_path);
  if (listener_->got_result_)
    return listener_->result_bool_;
  return false;
}

base::string16 FFUnitTestDecryptorProxy::Decrypt(const std::string& crypt) {
  listener_->Decrypt(crypt);
  if (listener_->got_result_)
    return listener_->result_string_;
  return base::string16();
}

std::vector<autofill::PasswordForm> FFUnitTestDecryptorProxy::ParseSignons(
    const base::FilePath& signons_path) {
  listener_->ParseSignons(signons_path);
  if (listener_->got_result_)
    return listener_->result_vector_;
  return std::vector<autofill::PasswordForm>();
}

// Entry function in child process.
MULTIPROCESS_TEST_MAIN(NSSDecrypterChildProcess) {
  base::MessageLoopForIO main_message_loop;

  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
      mojo::edk::ConnectionParams(
          mojo::edk::TransportProtocol::kLegacy,
          mojo::edk::ScopedInternalPlatformHandle(
              mojo::edk::InternalPlatformHandle(
                  service_manager::kMojoIPCChannel +
                  base::GlobalDescriptors::kBaseDescriptor))));
  mojo::ScopedMessagePipeHandle mojo_handle = invitation->ExtractMessagePipe(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kMojoChannelToken));

  firefox_importer_unittest_utils_mac::mojom::FirefoxDecryptorRequest request(
      std::move(mojo_handle));
  FFDecryptorClientListener listener(std::move(request));
  base::RunLoop run_loop;
  listener.SetQuitClosure(run_loop.QuitClosure());
  run_loop.Run();

  return 0;
}
