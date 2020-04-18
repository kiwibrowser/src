// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media/media_engagement_ui.h"

#include <memory>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/media_engagement_score.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "media/base/media_switches.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace {

namespace {

// This is the component ID for the MEI Preload component.
const char kPreloadComponentID[] = "aemomkdncapdnfajjbbcbdebjljbpmpj";

}  // namespace

// Implementation of media::mojom::MediaEngagementScoreDetailsProvider that
// retrieves engagement details from the MediaEngagementService.
class MediaEngagementScoreDetailsProviderImpl
    : public media::mojom::MediaEngagementScoreDetailsProvider {
 public:
  MediaEngagementScoreDetailsProviderImpl(
      Profile* profile,
      mojo::InterfaceRequest<media::mojom::MediaEngagementScoreDetailsProvider>
          request)
      : profile_(profile), binding_(this, std::move(request)) {
    DCHECK(profile_);
    service_ = MediaEngagementService::Get(profile_);
  }

  ~MediaEngagementScoreDetailsProviderImpl() override {}

  // media::mojom::MediaEngagementScoreDetailsProvider overrides:
  void GetMediaEngagementScoreDetails(
      media::mojom::MediaEngagementScoreDetailsProvider::
          GetMediaEngagementScoreDetailsCallback callback) override {
    std::move(callback).Run(service_->GetAllScoreDetails());
  }

  void GetMediaEngagementConfig(
      media::mojom::MediaEngagementScoreDetailsProvider::
          GetMediaEngagementConfigCallback callback) override {
    std::move(callback).Run(media::mojom::MediaEngagementConfig::New(
        MediaEngagementScore::GetScoreMinVisits(),
        MediaEngagementScore::GetHighScoreLowerThreshold(),
        MediaEngagementScore::GetHighScoreUpperThreshold(),
        base::FeatureList::IsEnabled(media::kRecordMediaEngagementScores),
        base::FeatureList::IsEnabled(
            media::kMediaEngagementBypassAutoplayPolicies),
        base::FeatureList::IsEnabled(media::kPreloadMediaEngagementData),
        media::GetEffectiveAutoplayPolicy(
            *base::CommandLine::ForCurrentProcess()),
        GetPreloadVersion()));
  }

 private:
  const std::string GetPreloadVersion() {
    component_updater::ComponentUpdateService* cus =
        g_browser_process->component_updater();
    std::vector<component_updater::ComponentInfo> info = cus->GetComponents();

    for (const auto& component : info) {
      if (component.id == kPreloadComponentID)
        return component.version.GetString();
    }

    return std::string();
  }

  Profile* profile_;

  MediaEngagementService* service_;

  mojo::Binding<media::mojom::MediaEngagementScoreDetailsProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementScoreDetailsProviderImpl);
};

}  // namespace

MediaEngagementUI::MediaEngagementUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  // Setup the data source behind chrome://media-engagement.
  std::unique_ptr<content::WebUIDataSource> source(
      content::WebUIDataSource::Create(chrome::kChromeUIMediaEngagementHost));
  source->AddResourcePath("media-engagement.js", IDR_MEDIA_ENGAGEMENT_JS);
  source->AddResourcePath(
      "chrome/browser/media/media_engagement_score_details.mojom.js",
      IDR_MEDIA_ENGAGEMENT_MOJO_JS);
  source->AddResourcePath("url/mojom/url.mojom.js", IDR_URL_MOJO_JS);
  source->SetDefaultResource(IDR_MEDIA_ENGAGEMENT_HTML);
  source->UseGzip();
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source.release());
  AddHandlerToRegistry(base::BindRepeating(
      &MediaEngagementUI::BindMediaEngagementScoreDetailsProvider,
      base::Unretained(this)));
}

MediaEngagementUI::~MediaEngagementUI() = default;

void MediaEngagementUI::BindMediaEngagementScoreDetailsProvider(
    media::mojom::MediaEngagementScoreDetailsProviderRequest request) {
  ui_handler_ = std::make_unique<MediaEngagementScoreDetailsProviderImpl>(
      Profile::FromWebUI(web_ui()), std::move(request));
}
