// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/messaging/native_message_host.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/arc/extensions/arc_support_message_host.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/constants.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "remoting/host/chromoting_host_context.h"
#include "remoting/host/it2me/it2me_native_messaging_host.h"
#include "remoting/host/policy_watcher.h"
#include "ui/events/system_input_injector.h"
#include "ui/gfx/native_widget_types.h"
#include "url/gurl.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace extensions {

namespace {

// A simple NativeMessageHost that mimics the implementation of
// chrome/test/data/native_messaging/native_hosts/echo.py. It is currently
// used for testing by ExtensionApiTest::NativeMessagingBasic.

const char* const kEchoHostOrigins[] = {
    // ScopedTestNativeMessagingHost::kExtensionId
    "chrome-extension://knldjmfmopnpolahpmmgbagdohdnhkik/"};

class EchoHost : public NativeMessageHost {
 public:
  static std::unique_ptr<NativeMessageHost> Create() {
    return std::unique_ptr<NativeMessageHost>(new EchoHost());
  }

  EchoHost() : message_number_(0), client_(NULL) {}

  void Start(Client* client) override { client_ = client; }

  void OnMessage(const std::string& request_string) override {
    std::unique_ptr<base::Value> request_value =
        base::JSONReader::Read(request_string);
    std::unique_ptr<base::DictionaryValue> request(
        static_cast<base::DictionaryValue*>(request_value.release()));
    if (request_string.find("stopHostTest") != std::string::npos) {
      client_->CloseChannel(kNativeHostExited);
    } else if (request_string.find("bigMessageTest") != std::string::npos) {
      client_->CloseChannel(kHostInputOutputError);
    } else {
      ProcessEcho(*request);
    }
  };

  scoped_refptr<base::SingleThreadTaskRunner> task_runner() const override {
    return base::ThreadTaskRunnerHandle::Get();
  };

 private:
  void ProcessEcho(const base::DictionaryValue& request) {
    base::DictionaryValue response;
    response.SetInteger("id", ++message_number_);
    response.Set("echo", request.CreateDeepCopy());
    response.SetString("caller_url", kEchoHostOrigins[0]);
    std::string response_string;
    base::JSONWriter::Write(response, &response_string);
    client_->PostMessageFromNativeHost(response_string);
  }

  int message_number_;
  Client* client_;

  DISALLOW_COPY_AND_ASSIGN(EchoHost);
};

struct BuiltInHost {
  const char* const name;
  const char* const* const allowed_origins;
  int allowed_origins_count;
  std::unique_ptr<NativeMessageHost> (*create_function)();
};

#if defined(USE_OZONE)
class OzoneSystemInputInjectorAdaptor : public ui::SystemInputInjectorFactory {
 public:
  std::unique_ptr<ui::SystemInputInjector> CreateSystemInputInjector()
      override {
    return ui::OzonePlatform::GetInstance()->CreateSystemInputInjector();
  }
};

base::LazyInstance<OzoneSystemInputInjectorAdaptor>::Leaky
    g_ozone_system_input_injector_adaptor = LAZY_INSTANCE_INITIALIZER;
#endif

ui::SystemInputInjectorFactory* GetInputInjector() {
  ui::SystemInputInjectorFactory* system = ui::GetSystemInputInjectorFactory();
  if (system)
    return system;

#if defined(USE_OZONE)
  return g_ozone_system_input_injector_adaptor.Pointer();
#endif

  return nullptr;
}

std::unique_ptr<NativeMessageHost> CreateIt2MeHost() {
  std::unique_ptr<remoting::It2MeHostFactory> host_factory(
      new remoting::It2MeHostFactory());
  std::unique_ptr<remoting::ChromotingHostContext> context =
      remoting::ChromotingHostContext::CreateForChromeOS(
          base::WrapRefCounted(g_browser_process->system_request_context()),
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::IO),
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::UI),
          base::CreateSingleThreadTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND}),
          GetInputInjector());
  std::unique_ptr<remoting::PolicyWatcher> policy_watcher =
      remoting::PolicyWatcher::CreateWithPolicyService(
          g_browser_process->policy_service());
  std::unique_ptr<NativeMessageHost> host(
      new remoting::It2MeNativeMessagingHost(
          /*needs_elevation=*/false, std::move(policy_watcher),
          std::move(context), std::move(host_factory)));
  return host;
}

// If you modify the list of allowed_origins, don't forget to update
// remoting/host/it2me/com.google.chrome.remote_assistance.json.jinja2
// to keep the two lists in sync.
// TODO(kelvinp): Load the native messaging manifest as a resource file into
// chrome and fetch the list of allowed_origins from the manifest (see
// crbug/424743).
const char* const kRemotingIt2MeOrigins[] = {
    "chrome-extension://ljacajndfccfgnfohlgkdphmbnpkjflk/",
    "chrome-extension://gbchcmhmhahfdphkhkmpfmihenigjmpp/",
    "chrome-extension://kgngmbheleoaphbjbaiobfdepmghbfah/",
    "chrome-extension://odkaodonbgfohohmklejpjiejmcipmib/",
    "chrome-extension://dokpleeekgeeiehdhmdkeimnkmoifgdd/",
    "chrome-extension://ajoainacpilcemgiakehflpbkbfipojk/",
    "chrome-extension://hmboipgjngjoiaeicfdifdoeacilalgc/",
    "chrome-extension://inomeogfingihgjfjlpeplalcfajhgai/",
    "chrome-extension://hpodccmdligbeohchckkeajbfohibipg/"};

static const BuiltInHost kBuiltInHost[] = {
    {"com.google.chrome.test.echo",  // ScopedTestNativeMessagingHost::kHostName
     kEchoHostOrigins, arraysize(kEchoHostOrigins), &EchoHost::Create},
    {"com.google.chrome.remote_assistance", kRemotingIt2MeOrigins,
     arraysize(kRemotingIt2MeOrigins), &CreateIt2MeHost},
    {arc::ArcSupportMessageHost::kHostName,
     arc::ArcSupportMessageHost::kHostOrigin, 1,
     &arc::ArcSupportMessageHost::Create},
};

bool MatchesSecurityOrigin(const BuiltInHost& host,
                           const std::string& extension_id) {
  GURL origin(std::string(kExtensionScheme) + "://" + extension_id);
  for (int i = 0; i < host.allowed_origins_count; i++) {
    URLPattern allowed_origin(URLPattern::SCHEME_ALL, host.allowed_origins[i]);
    if (allowed_origin.MatchesSecurityOrigin(origin)) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::unique_ptr<NativeMessageHost> NativeMessageHost::Create(
    gfx::NativeView native_view,
    const std::string& source_extension_id,
    const std::string& native_host_name,
    bool allow_user_level,
    std::string* error) {
  for (unsigned int i = 0; i < arraysize(kBuiltInHost); i++) {
    const BuiltInHost& host = kBuiltInHost[i];
    std::string name(host.name);
    if (name == native_host_name) {
      if (MatchesSecurityOrigin(host, source_extension_id)) {
        return (*host.create_function)();
      }
      *error = kForbiddenError;
      return nullptr;
    }
  }
  *error = kNotFoundError;
  return nullptr;
}

}  // namespace extensions
