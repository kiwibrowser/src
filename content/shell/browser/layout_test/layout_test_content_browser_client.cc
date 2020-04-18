// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_content_browser_client.h"

#include "base/single_thread_task_runner.h"
#include "base/strings/pattern.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_package_context.h"
#include "content/shell/browser/layout_test/blink_test_controller.h"
#include "content/shell/browser/layout_test/fake_bluetooth_chooser.h"
#include "content/shell/browser/layout_test/layout_test_bluetooth_fake_adapter_setter_impl.h"
#include "content/shell/browser/layout_test/layout_test_browser_context.h"
#include "content/shell/browser/layout_test/layout_test_browser_main_parts.h"
#include "content/shell/browser/layout_test/layout_test_message_filter.h"
#include "content/shell/browser/layout_test/layout_test_notification_manager.h"
#include "content/shell/browser/layout_test/mojo_layout_test_helper.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/shell/common/layout_test/layout_test_switches.h"
#include "content/shell/common/shell_messages.h"
#include "content/shell/renderer/layout_test/blink_test_helpers.h"
#include "content/test/mock_clipboard_host.h"
#include "device/bluetooth/test/fake_bluetooth.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/mojom/web_package/web_package_internals.mojom.h"

namespace content {
namespace {

LayoutTestContentBrowserClient* g_layout_test_browser_client;

void BindLayoutTestHelper(mojom::MojoLayoutTestHelperRequest request,
                          RenderFrameHost* render_frame_host) {
  MojoLayoutTestHelper::Create(std::move(request));
}

class WebPackageInternalsImpl : public blink::test::mojom::WebPackageInternals {
 public:
  explicit WebPackageInternalsImpl(WebPackageContext* web_package_context)
      : web_package_context_(web_package_context) {}
  ~WebPackageInternalsImpl() override = default;

  static void Create(WebPackageContext* web_package_context,
                     blink::test::mojom::WebPackageInternalsRequest request) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    mojo::MakeStrongBinding(
        std::make_unique<WebPackageInternalsImpl>(web_package_context),
        std::move(request));
  }

 private:
  void SetSignedExchangeVerificationTime(
      base::Time time,
      SetSignedExchangeVerificationTimeCallback callback) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    web_package_context_->SetSignedExchangeVerificationTimeForTesting(time);
    std::move(callback).Run();
  }

  WebPackageContext* web_package_context_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageInternalsImpl);
};

}  // namespace

LayoutTestContentBrowserClient::LayoutTestContentBrowserClient() {
  DCHECK(!g_layout_test_browser_client);

  layout_test_notification_manager_.reset(new LayoutTestNotificationManager());

  g_layout_test_browser_client = this;
}

LayoutTestContentBrowserClient::~LayoutTestContentBrowserClient() {
  g_layout_test_browser_client = nullptr;
}

LayoutTestContentBrowserClient* LayoutTestContentBrowserClient::Get() {
  return g_layout_test_browser_client;
}

LayoutTestBrowserContext*
LayoutTestContentBrowserClient::GetLayoutTestBrowserContext() {
  return static_cast<LayoutTestBrowserContext*>(browser_context());
}

void LayoutTestContentBrowserClient::SetPopupBlockingEnabled(
    bool block_popups) {
  block_popups_ = block_popups;
}

void LayoutTestContentBrowserClient::ResetMockClipboardHost() {
  if (mock_clipboard_host_)
    mock_clipboard_host_->Reset();
}

std::unique_ptr<FakeBluetoothChooser>
LayoutTestContentBrowserClient::GetNextFakeBluetoothChooser() {
  return std::move(next_fake_bluetooth_chooser_);
}

LayoutTestNotificationManager*
LayoutTestContentBrowserClient::GetLayoutTestNotificationManager() {
  return layout_test_notification_manager_.get();
}

void LayoutTestContentBrowserClient::RenderProcessWillLaunch(
    RenderProcessHost* host,
    service_manager::mojom::ServiceRequest* service_request) {
  ShellContentBrowserClient::RenderProcessWillLaunch(host, service_request);

  StoragePartition* partition =
      BrowserContext::GetDefaultStoragePartition(browser_context());
  host->AddFilter(new LayoutTestMessageFilter(
      host->GetID(), partition->GetDatabaseTracker(),
      partition->GetQuotaManager(), partition->GetURLRequestContext(),
      partition->GetNetworkContext()));
}

void LayoutTestContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    RenderProcessHost* render_process_host) {
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner =
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI);
  registry->AddInterface(
      base::BindRepeating(&LayoutTestBluetoothFakeAdapterSetterImpl::Create),
      ui_task_runner);

  registry->AddInterface(base::BindRepeating(&bluetooth::FakeBluetooth::Create),
                         ui_task_runner);
  // This class outlives |render_process_host|, which owns |registry|. Since
  // any binders will not be called after |registry| is deleted
  // and |registry| is outlived by this class, it is safe to use
  // base::Unretained in all binders.
  registry->AddInterface(
      base::BindRepeating(
          &LayoutTestContentBrowserClient::CreateFakeBluetoothChooser,
          base::Unretained(this)),
      ui_task_runner);
  registry->AddInterface(base::BindRepeating(
      &WebPackageInternalsImpl::Create,
      base::Unretained(
          render_process_host->GetStoragePartition()->GetWebPackageContext())));
  registry->AddInterface(base::BindRepeating(&MojoLayoutTestHelper::Create));
  registry->AddInterface(
      base::BindRepeating(&LayoutTestContentBrowserClient::BindClipboardHost,
                          base::Unretained(this)),
      ui_task_runner);
}

void LayoutTestContentBrowserClient::BindClipboardHost(
    blink::mojom::ClipboardHostRequest request) {
  if (!mock_clipboard_host_) {
    mock_clipboard_host_ =
        std::make_unique<MockClipboardHost>(browser_context());
  }
  mock_clipboard_host_->Bind(std::move(request));
}

void LayoutTestContentBrowserClient::OverrideWebkitPrefs(
    RenderViewHost* render_view_host,
    WebPreferences* prefs) {
  if (BlinkTestController::Get())
    BlinkTestController::Get()->OverrideWebkitPrefs(prefs);
}

void LayoutTestContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  command_line->AppendSwitch(switches::kRunWebTests);
  ShellContentBrowserClient::AppendExtraCommandLineSwitches(command_line,
                                                            child_process_id);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAlwaysUseComplexText)) {
    command_line->AppendSwitch(switches::kAlwaysUseComplexText);
  }
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFontAntialiasing)) {
    command_line->AppendSwitch(switches::kEnableFontAntialiasing);
  }
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kStableReleaseMode)) {
    command_line->AppendSwitch(switches::kStableReleaseMode);
  }
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableLeakDetection)) {
    command_line->AppendSwitchASCII(
        switches::kEnableLeakDetection,
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kEnableLeakDetection));
  }
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableDisplayCompositorPixelDump)) {
    command_line->AppendSwitch(switches::kEnableDisplayCompositorPixelDump);
  }
}

BrowserMainParts* LayoutTestContentBrowserClient::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  set_browser_main_parts(new LayoutTestBrowserMainParts(parameters));
  return shell_browser_main_parts();
}

void LayoutTestContentBrowserClient::GetQuotaSettings(
    BrowserContext* context,
    StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  // The 1GB limit is intended to give a large headroom to tests that need to
  // build up a large data set and issue many concurrent reads or writes.
  std::move(callback).Run(storage::GetHardCodedSettings(1024 * 1024 * 1024));
}

bool LayoutTestContentBrowserClient::DoesSiteRequireDedicatedProcess(
    BrowserContext* browser_context,
    const GURL& effective_site_url) {
  if (ShellContentBrowserClient::DoesSiteRequireDedicatedProcess(
          browser_context, effective_site_url))
    return true;
  url::Origin origin = url::Origin::Create(effective_site_url);
  return base::MatchPattern(origin.Serialize(), "*oopif.test");
}

PlatformNotificationService*
LayoutTestContentBrowserClient::GetPlatformNotificationService() {
  return layout_test_notification_manager_.get();
}

bool LayoutTestContentBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const GURL& source_origin,
    content::mojom::WindowContainerType container_type,
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    bool* no_javascript_access) {
  *no_javascript_access = false;
  return !block_popups_ || user_gesture;
}

void LayoutTestContentBrowserClient::ExposeInterfacesToFrame(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry) {
  registry->AddInterface(base::Bind(&BindLayoutTestHelper));
}

scoped_refptr<LoginDelegate>
LayoutTestContentBrowserClient::CreateLoginDelegate(
    net::AuthChallengeInfo* auth_info,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    bool is_main_frame,
    const GURL& url,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  return nullptr;
}

// private
void LayoutTestContentBrowserClient::CreateFakeBluetoothChooser(
    mojom::FakeBluetoothChooserRequest request) {
  DCHECK(!next_fake_bluetooth_chooser_);
  next_fake_bluetooth_chooser_ =
      FakeBluetoothChooser::Create(std::move(request));
}

}  // namespace content
