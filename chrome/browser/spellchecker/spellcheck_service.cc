// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spellcheck_service.h"

#include <set>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "base/supports_user_data.h"
#include "base/synchronization/waitable_event.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_service.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/spellchecker/spellcheck_hunspell_dictionary.h"
#include "chrome/common/constants.mojom.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/spellcheck/browser/spellcheck_host_metrics.h"
#include "components/spellcheck/browser/spellcheck_platform.h"
#include "components/spellcheck/browser/spelling_service_client.h"
#include "components/spellcheck/common/spellcheck.mojom.h"
#include "components/spellcheck/common/spellcheck_common.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

// TODO(rlp): I do not like globals, but keeping these for now during
// transition.
// An event used by browser tests to receive status events from this class and
// its derived classes.
base::WaitableEvent* g_status_event = NULL;
SpellcheckService::EventType g_status_type =
    SpellcheckService::BDICT_NOTINITIALIZED;

SpellcheckService::SpellcheckService(content::BrowserContext* context)
    : context_(context),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  pref_change_registrar_.Init(prefs);
  StringListPrefMember dictionaries_pref;
  dictionaries_pref.Init(spellcheck::prefs::kSpellCheckDictionaries, prefs);
  std::string first_of_dictionaries;

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
  // Ensure that the renderer always knows the platform spellchecking language.
  // This language is used for initialization of the text iterator. If the
  // iterator is not initialized, then the context menu does not show spellcheck
  // suggestions.
  //
  // No migration is necessary, because the spellcheck language preference is
  // not user visible or modifiable in Chrome on Mac.
  dictionaries_pref.SetValue(std::vector<std::string>(
      1, spellcheck_platform::GetSpellCheckerLanguage()));
  first_of_dictionaries = dictionaries_pref.GetValue().front();
#else
  // Migrate preferences from single-language to multi-language schema.
  StringPrefMember single_dictionary_pref;
  single_dictionary_pref.Init(spellcheck::prefs::kSpellCheckDictionary, prefs);
  std::string single_dictionary = single_dictionary_pref.GetValue();

  if (!dictionaries_pref.GetValue().empty())
    first_of_dictionaries = dictionaries_pref.GetValue().front();

  if (first_of_dictionaries.empty() && !single_dictionary.empty()) {
    first_of_dictionaries = single_dictionary;
    dictionaries_pref.SetValue(
        std::vector<std::string>(1, first_of_dictionaries));
  }

  single_dictionary_pref.SetValue("");
#endif  // BUILDFLAG(USE_BROWSER_SPELLCHECKER)

  pref_change_registrar_.Add(
      spellcheck::prefs::kSpellCheckDictionaries,
      base::BindRepeating(&SpellcheckService::OnSpellCheckDictionariesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      spellcheck::prefs::kSpellCheckForcedDictionaries,
      base::BindRepeating(&SpellcheckService::OnSpellCheckDictionariesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      spellcheck::prefs::kSpellCheckUseSpellingService,
      base::BindRepeating(&SpellcheckService::OnUseSpellingServiceChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kAcceptLanguages,
      base::BindRepeating(&SpellcheckService::OnAcceptLanguagesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      spellcheck::prefs::kSpellCheckEnable,
      base::BindRepeating(&SpellcheckService::InitForAllRenderers,
                          base::Unretained(this)));

  custom_dictionary_.reset(new SpellcheckCustomDictionary(context_->GetPath()));
  custom_dictionary_->AddObserver(this);
  custom_dictionary_->Load();

  registrar_.Add(this,
                 content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllSources());

  LoadHunspellDictionaries();
}

SpellcheckService::~SpellcheckService() {
  // Remove pref observers
  pref_change_registrar_.RemoveAll();
}

base::WeakPtr<SpellcheckService> SpellcheckService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

#if !defined(OS_MACOSX)
// static
void SpellcheckService::GetDictionaries(base::SupportsUserData* browser_context,
                                        std::vector<Dictionary>* dictionaries) {
  PrefService* prefs = user_prefs::UserPrefs::Get(browser_context);
  std::set<std::string> spellcheck_dictionaries;
  for (const auto& value :
       *prefs->GetList(spellcheck::prefs::kSpellCheckDictionaries)) {
    std::string dictionary;
    if (value.GetAsString(&dictionary))
      spellcheck_dictionaries.insert(dictionary);
  }

  dictionaries->clear();
  std::vector<std::string> accept_languages =
      base::SplitString(prefs->GetString(prefs::kAcceptLanguages), ",",
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& accept_language : accept_languages) {
    Dictionary dictionary;
    dictionary.language =
        spellcheck::GetCorrespondingSpellCheckLanguage(accept_language);
    if (dictionary.language.empty())
      continue;

    dictionary.used_for_spellcheck =
        spellcheck_dictionaries.count(dictionary.language) > 0;
    dictionaries->push_back(dictionary);
  }
}
#endif  // !OS_MACOSX

// static
bool SpellcheckService::SignalStatusEvent(
    SpellcheckService::EventType status_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!g_status_event)
    return false;
  g_status_type = status_type;
  g_status_event->Signal();
  return true;
}

void SpellcheckService::StartRecordingMetrics(bool spellcheck_enabled) {
  metrics_ = std::make_unique<SpellCheckHostMetrics>();
  metrics_->RecordEnabledStats(spellcheck_enabled);
  OnUseSpellingServiceChanged();
}

void SpellcheckService::InitForRenderer(
    const service_manager::Identity& renderer_identity) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::BrowserContext* context =
      content::BrowserContext::GetBrowserContextForServiceUserId(
          renderer_identity.user_id());
  if (SpellcheckServiceFactory::GetForContext(context) != this)
    return;

  const PrefService* prefs = user_prefs::UserPrefs::Get(context);
  std::vector<spellcheck::mojom::SpellCheckBDictLanguagePtr> dictionaries;

  for (const auto& hunspell_dictionary : hunspell_dictionaries_) {
    dictionaries.push_back(spellcheck::mojom::SpellCheckBDictLanguage::New(
        hunspell_dictionary->GetDictionaryFile().Duplicate(),
        hunspell_dictionary->GetLanguage()));
  }

  bool enable = prefs->GetBoolean(spellcheck::prefs::kSpellCheckEnable) &&
                !dictionaries.empty();

  std::vector<std::string> custom_words;
  if (enable) {
    custom_words.assign(custom_dictionary_->GetWords().begin(),
                        custom_dictionary_->GetWords().end());
  }

  spellcheck::mojom::SpellCheckerPtr spellchecker;
  ChromeService::GetInstance()->connector()->BindInterface(
      service_manager::Identity(chrome::mojom::kRendererServiceName,
                                renderer_identity.user_id(),
                                renderer_identity.instance()),
      &spellchecker);
  spellchecker->Initialize(std::move(dictionaries), custom_words, enable);
}

SpellCheckHostMetrics* SpellcheckService::GetMetrics() const {
  return metrics_.get();
}

SpellcheckCustomDictionary* SpellcheckService::GetCustomDictionary() {
  return custom_dictionary_.get();
}

void SpellcheckService::LoadHunspellDictionaries() {
  hunspell_dictionaries_.clear();

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  DCHECK(prefs);

  const base::ListValue* user_dictionaries =
      prefs->GetList(spellcheck::prefs::kSpellCheckDictionaries);
  const base::ListValue* forced_dictionaries =
      prefs->GetList(spellcheck::prefs::kSpellCheckForcedDictionaries);

  // Merge both lists of dictionaries. Use a set to avoid duplicates.
  std::set<std::string> dictionaries;
  for (const auto& dictionary_value : user_dictionaries->GetList())
    dictionaries.insert(dictionary_value.GetString());
  for (const auto& dictionary_value : forced_dictionaries->GetList())
    dictionaries.insert(dictionary_value.GetString());

  for (const auto& dictionary : dictionaries) {
    hunspell_dictionaries_.push_back(
        std::make_unique<SpellcheckHunspellDictionary>(dictionary, context_,
                                                       this));
    hunspell_dictionaries_.back()->AddObserver(this);
    hunspell_dictionaries_.back()->Load();
  }
}

const std::vector<std::unique_ptr<SpellcheckHunspellDictionary>>&
SpellcheckService::GetHunspellDictionaries() {
  return hunspell_dictionaries_;
}

bool SpellcheckService::LoadExternalDictionary(std::string language,
                                               std::string locale,
                                               std::string path,
                                               DictionaryFormat format) {
  return false;
}

bool SpellcheckService::UnloadExternalDictionary(
    const std::string& /* path */) {
  return false;
}

void SpellcheckService::Observe(int type,
                                const content::NotificationSource& source,
                                const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_RENDERER_PROCESS_CREATED, type);
  InitForRenderer(content::Source<content::RenderProcessHost>(source)
                      .ptr()
                      ->GetChildIdentity());
}

void SpellcheckService::OnCustomDictionaryLoaded() {
  InitForAllRenderers();
}

void SpellcheckService::OnCustomDictionaryChanged(
    const SpellcheckCustomDictionary::Change& change) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto process_hosts(content::RenderProcessHost::AllHostsIterator());

  const std::vector<std::string> additions(change.to_add().begin(),
                                           change.to_add().end());
  const std::vector<std::string> deletions(change.to_remove().begin(),
                                           change.to_remove().end());
  while (!process_hosts.IsAtEnd()) {
    service_manager::Identity renderer_identity =
        process_hosts.GetCurrentValue()->GetChildIdentity();
    spellcheck::mojom::SpellCheckerPtr spellchecker;
    ChromeService::GetInstance()->connector()->BindInterface(
        service_manager::Identity(chrome::mojom::kRendererServiceName,
                                  renderer_identity.user_id(),
                                  renderer_identity.instance()),
        &spellchecker);
    spellchecker->CustomDictionaryChanged(additions, deletions);
    process_hosts.Advance();
  }
}

void SpellcheckService::OnHunspellDictionaryInitialized(
    const std::string& language) {
  InitForAllRenderers();
}

void SpellcheckService::OnHunspellDictionaryDownloadBegin(
    const std::string& language) {
}

void SpellcheckService::OnHunspellDictionaryDownloadSuccess(
    const std::string& language) {
}

void SpellcheckService::OnHunspellDictionaryDownloadFailure(
    const std::string& language) {
}

// static
void SpellcheckService::AttachStatusEvent(base::WaitableEvent* status_event) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  g_status_event = status_event;
}

// static
SpellcheckService::EventType SpellcheckService::GetStatusEvent() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return g_status_type;
}

void SpellcheckService::InitForAllRenderers() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (content::RenderProcessHost::iterator i(
          content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    content::RenderProcessHost* process = i.GetCurrentValue();
    if (process && process->GetProcess().Handle())
      InitForRenderer(process->GetChildIdentity());
  }
}

void SpellcheckService::OnSpellCheckDictionariesChanged() {
  // If there are hunspell dictionaries, then fire off notifications to the
  // renderers after the dictionaries are finished loading.
  LoadHunspellDictionaries();

  // If there are no hunspell dictionaries to load, then immediately let the
  // renderers know the new state.
  if (hunspell_dictionaries_.empty())
    InitForAllRenderers();
}

void SpellcheckService::OnUseSpellingServiceChanged() {
  bool enabled = pref_change_registrar_.prefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService);
  if (metrics_)
    metrics_->RecordSpellingServiceStats(enabled);
}

void SpellcheckService::OnAcceptLanguagesChanged() {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  std::vector<std::string> accept_languages =
      base::SplitString(prefs->GetString(prefs::kAcceptLanguages), ",",
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::transform(accept_languages.begin(), accept_languages.end(),
                 accept_languages.begin(),
                 &spellcheck::GetCorrespondingSpellCheckLanguage);

  StringListPrefMember dictionaries_pref;
  dictionaries_pref.Init(spellcheck::prefs::kSpellCheckDictionaries, prefs);
  std::vector<std::string> dictionaries = dictionaries_pref.GetValue();
  std::vector<std::string> filtered_dictionaries;

  for (const auto& dictionary : dictionaries) {
    if (base::ContainsValue(accept_languages, dictionary)) {
      filtered_dictionaries.push_back(dictionary);
    }
  }

  dictionaries_pref.SetValue(filtered_dictionaries);
}
