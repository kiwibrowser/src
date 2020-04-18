// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_snippets/contextual_content_suggestions_service_factory.h"

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/ntp_snippets/contextual/contextual_content_suggestions_service.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_debugging_reporter.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_features.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_fetcher_impl.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "components/ntp_snippets/remote/cached_image_fetcher.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/service_manager_connection.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/chrome_feature_list.h"
#endif

using contextual_suggestions::ContextualSuggestionsFetcherImpl;
using contextual_suggestions::ContextualContentSuggestionsService;

using ntp_snippets::CachedImageFetcher;
using ntp_snippets::RemoteSuggestionsDatabase;

namespace {

bool IsContextualContentSuggestionsEnabled() {
#if defined(OS_ANDROID)
  return base::FeatureList::IsEnabled(
      contextual_suggestions::kContextualSuggestionsBottomSheet);
#else
  return false;
#endif  // OS_ANDROID
}

}  // namespace

// static
ContextualContentSuggestionsServiceFactory*
ContextualContentSuggestionsServiceFactory::GetInstance() {
  return base::Singleton<ContextualContentSuggestionsServiceFactory>::get();
}

// static
ContextualContentSuggestionsService*
ContextualContentSuggestionsServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<ContextualContentSuggestionsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
ContextualContentSuggestionsService*
ContextualContentSuggestionsServiceFactory::GetForProfileIfExists(
    Profile* profile) {
  return static_cast<ContextualContentSuggestionsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

ContextualContentSuggestionsServiceFactory::
    ContextualContentSuggestionsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ContextualContentSuggestionsService",
          BrowserContextDependencyManager::GetInstance()) {
}

ContextualContentSuggestionsServiceFactory::
    ~ContextualContentSuggestionsServiceFactory() = default;

KeyedService*
ContextualContentSuggestionsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(!profile->IsOffTheRecord());
  if (!IsContextualContentSuggestionsEnabled()) {
    return nullptr;
  }

  PrefService* pref_service = profile->GetPrefs();
  scoped_refptr<net::URLRequestContextGetter> request_context =
      profile->GetRequestContext();
  content::StoragePartition* storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(context);
  auto contextual_suggestions_fetcher =
      std::make_unique<ContextualSuggestionsFetcherImpl>(
          storage_partition->GetURLLoaderFactoryForBrowserProcess(),
          g_browser_process->GetApplicationLocale());
  const base::FilePath::CharType kDatabaseFolder[] =
      FILE_PATH_LITERAL("contextualSuggestionsDatabase");
  base::FilePath database_dir(profile->GetPath().Append(kDatabaseFolder));
  auto contextual_suggestions_database =
      std::make_unique<RemoteSuggestionsDatabase>(database_dir);
  auto cached_image_fetcher =
      std::make_unique<ntp_snippets::CachedImageFetcher>(
          std::make_unique<image_fetcher::ImageFetcherImpl>(
              std::make_unique<suggestions::ImageDecoderImpl>(),
              request_context.get()),
          pref_service, contextual_suggestions_database.get());
  auto reporter_provider = std::make_unique<
      contextual_suggestions::ContextualSuggestionsReporterProvider>(
      std::make_unique<
          contextual_suggestions::ContextualSuggestionsDebuggingReporter>());
  auto* service = new ContextualContentSuggestionsService(
      std::move(contextual_suggestions_fetcher),
      std::move(cached_image_fetcher),
      std::move(contextual_suggestions_database), std::move(reporter_provider));

  return service;
}
