// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/content/client/shell_main_delegate.h"

#include "ash/components/quick_launch/public/mojom/constants.mojom.h"
#include "ash/components/quick_launch/quick_launch_application.h"
#include "ash/components/shortcut_viewer/public/mojom/constants.mojom.h"
#include "ash/components/shortcut_viewer/shortcut_viewer_application.h"
#include "ash/components/tap_visualizer/public/mojom/constants.mojom.h"
#include "ash/components/tap_visualizer/tap_visualizer_app.h"
#include "ash/shell/content/client/shell_content_browser_client.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "components/services/font/font_service_app.h"
#include "components/services/font/public/interfaces/constants.mojom.h"
#include "content/public/common/content_switches.h"
#include "content/public/utility/content_utility_client.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/ui/ime/test_ime_driver/public/mojom/constants.mojom.h"
#include "services/ui/ime/test_ime_driver/test_ime_application.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/base/resource/resource_bundle.h"

namespace ash {
namespace shell {
namespace {

std::unique_ptr<service_manager::Service> CreateFontService() {
  return std::make_unique<font_service::FontServiceApp>();
}

std::unique_ptr<service_manager::Service> CreateQuickLaunch() {
  return std::make_unique<quick_launch::QuickLaunchApplication>();
}

std::unique_ptr<service_manager::Service> CreateShortcutViewer() {
  return std::make_unique<
      keyboard_shortcut_viewer::ShortcutViewerApplication>();
}

std::unique_ptr<service_manager::Service> CreateTapVisualizer() {
  return std::make_unique<tap_visualizer::TapVisualizerApp>();
}

std::unique_ptr<service_manager::Service> CreateTestImeDriver() {
  return std::make_unique<ui::test::TestIMEApplication>();
}

class ShellContentUtilityClient : public content::ContentUtilityClient {
 public:
  ShellContentUtilityClient() = default;
  ~ShellContentUtilityClient() override = default;

  // ContentUtilityClient:
  void RegisterServices(StaticServiceMap* services) override {
    {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateFontService);
      (*services)[font_service::mojom::kServiceName] = info;
    }
    {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateQuickLaunch);
      (*services)[quick_launch::mojom::kServiceName] = info;
    }
    {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateShortcutViewer);
      (*services)[shortcut_viewer::mojom::kServiceName] = info;
    }
    {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateTapVisualizer);
      (*services)[tap_visualizer::mojom::kServiceName] = info;
    }
    {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateTestImeDriver);
      (*services)[test_ime_driver::mojom::kServiceName] = info;
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellContentUtilityClient);
};

}  // namespace

ShellMainDelegate::ShellMainDelegate() = default;

ShellMainDelegate::~ShellMainDelegate() = default;

bool ShellMainDelegate::BasicStartupComplete(int* exit_code) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);

  content::SetContentClient(&content_client_);

  return false;
}

void ShellMainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
  ui::InitializeInputMethodForTesting();
}

content::ContentBrowserClient* ShellMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new ShellContentBrowserClient);
  return browser_client_.get();
}

void ShellMainDelegate::InitializeResourceBundle() {
  // Load ash resources and strings; not 'common' (Chrome) resources.
  base::FilePath path;
  base::PathService::Get(base::DIR_MODULE, &path);
  base::FilePath ash_test_strings =
      path.Append(FILE_PATH_LITERAL("ash_test_strings.pak"));

  ui::ResourceBundle::InitSharedInstanceWithPakPath(ash_test_strings);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (ui::ResourceBundle::IsScaleFactorSupported(ui::SCALE_FACTOR_100P)) {
    base::FilePath ash_test_resources_100 = path.Append(
        FILE_PATH_LITERAL("ash_test_resources_with_content_100_percent.pak"));
    rb.AddDataPackFromPath(ash_test_resources_100, ui::SCALE_FACTOR_100P);
  }
  if (ui::ResourceBundle::IsScaleFactorSupported(ui::SCALE_FACTOR_200P)) {
    base::FilePath ash_test_resources_200 =
        path.Append(FILE_PATH_LITERAL("ash_test_resources_200_percent.pak"));
    rb.AddDataPackFromPath(ash_test_resources_200, ui::SCALE_FACTOR_200P);
  }
}

content::ContentUtilityClient* ShellMainDelegate::CreateContentUtilityClient() {
  utility_client_ = std::make_unique<ShellContentUtilityClient>();
  return utility_client_.get();
}

}  // namespace shell
}  // namespace ash
