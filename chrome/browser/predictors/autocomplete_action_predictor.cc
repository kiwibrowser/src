// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/autocomplete_action_predictor.h"

#include <math.h>
#include <stddef.h>

#include "base/bind.h"
#include "base/guid.h"
#include "base/i18n/case_conversion.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/predictors/autocomplete_action_predictor_factory.h"
#include "chrome/browser/predictors/predictor_database.h"
#include "chrome/browser/predictors/predictor_database_factory.h"
#include "chrome/browser/prerender/prerender_field_trial.h"
#include "chrome/browser/prerender/prerender_handle.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/prerender/prerender_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/in_memory_database.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_log.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"

namespace {

const float kConfidenceCutoff[] = {
  0.8f,
  0.5f
};

static_assert(arraysize(kConfidenceCutoff) ==
              predictors::AutocompleteActionPredictor::LAST_PREDICT_ACTION,
              "kConfidenceCutoff count should match LAST_PREDICT_ACTION");

const size_t kMinimumUserTextLength = 1;
const int kMinimumNumberOfHits = 3;

enum DatabaseAction {
  DATABASE_ACTION_ADD,
  DATABASE_ACTION_UPDATE,
  DATABASE_ACTION_DELETE_SOME,
  DATABASE_ACTION_DELETE_ALL,
  DATABASE_ACTION_COUNT
};

}  // namespace

namespace predictors {

const int AutocompleteActionPredictor::kMaximumDaysToKeepEntry = 14;

AutocompleteActionPredictor::AutocompleteActionPredictor(Profile* profile)
    : profile_(profile),
      main_profile_predictor_(NULL),
      incognito_predictor_(NULL),
      initialized_(false),
      history_service_observer_(this) {
  if (profile_->IsOffTheRecord()) {
    main_profile_predictor_ = AutocompleteActionPredictorFactory::GetForProfile(
        profile_->GetOriginalProfile());
    DCHECK(main_profile_predictor_);
    main_profile_predictor_->incognito_predictor_ = this;
    if (main_profile_predictor_->initialized_)
      CopyFromMainProfile();
  } else {
    // Request the in-memory database from the history to force it to load so
    // it's available as soon as possible.
    history::HistoryService* history_service =
        HistoryServiceFactory::GetForProfile(
            profile_, ServiceAccessType::EXPLICIT_ACCESS);
    if (history_service)
      history_service->InMemoryDatabase();

    table_ =
        PredictorDatabaseFactory::GetForProfile(profile_)->autocomplete_table();

    // Observe all main frame loads so we can wait for the first to complete
    // before accessing DB sequence of the AutocompleteActionPredictorTable and
    // IO thread to build the local cache.
    notification_registrar_.Add(this,
                                content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
                                content::NotificationService::AllSources());
  }
}

AutocompleteActionPredictor::~AutocompleteActionPredictor() {
  if (main_profile_predictor_)
    main_profile_predictor_->incognito_predictor_ = NULL;
  else if (incognito_predictor_)
    incognito_predictor_->main_profile_predictor_ = NULL;
  if (prerender_handle_.get())
    prerender_handle_->OnCancel();
}

void AutocompleteActionPredictor::RegisterTransitionalMatches(
    const base::string16& user_text,
    const AutocompleteResult& result) {
  if (user_text.length() < kMinimumUserTextLength)
    return;
  const base::string16 lower_user_text(base::i18n::ToLower(user_text));

  // Merge this in to an existing match if we already saw |user_text|
  std::vector<TransitionalMatch>::iterator match_it =
      std::find(transitional_matches_.begin(), transitional_matches_.end(),
                lower_user_text);

  if (match_it == transitional_matches_.end()) {
    TransitionalMatch transitional_match;
    transitional_match.user_text = lower_user_text;
    match_it = transitional_matches_.insert(transitional_matches_.end(),
                                            transitional_match);
  }

  for (const auto& i : result) {
    if (!base::ContainsValue(match_it->urls, i.destination_url))
      match_it->urls.push_back(i.destination_url);
  }
}

void AutocompleteActionPredictor::ClearTransitionalMatches() {
  transitional_matches_.clear();
}

void AutocompleteActionPredictor::CancelPrerender() {
  // If the prerender has already been abandoned, leave it to its own timeout;
  // this normally gets called immediately after OnOmniboxOpenedUrl.
  if (prerender_handle_ && !prerender_handle_->IsAbandoned()) {
    prerender_handle_->OnCancel();
    prerender_handle_.reset();
  }
}

void AutocompleteActionPredictor::StartPrerendering(
    const GURL& url,
    content::SessionStorageNamespace* session_storage_namespace,
    const gfx::Size& size) {
  // Only cancel the old prerender after starting the new one, so if the URLs
  // are the same, the underlying prerender will be reused.
  std::unique_ptr<prerender::PrerenderHandle> old_prerender_handle =
      std::move(prerender_handle_);
  prerender::PrerenderManager* prerender_manager =
      prerender::PrerenderManagerFactory::GetForBrowserContext(profile_);
  if (prerender_manager) {
    prerender_handle_ = prerender_manager->AddPrerenderFromOmnibox(
        url, session_storage_namespace, size);
  }
  if (old_prerender_handle)
    old_prerender_handle->OnCancel();
}

AutocompleteActionPredictor::Action
    AutocompleteActionPredictor::RecommendAction(
        const base::string16& user_text,
        const AutocompleteMatch& match) const {
  bool is_in_db = false;
  const double confidence = CalculateConfidence(user_text, match, &is_in_db);
  DCHECK(confidence >= 0.0 && confidence <= 1.0);

  UMA_HISTOGRAM_BOOLEAN("AutocompleteActionPredictor.MatchIsInDb", is_in_db);

  if (is_in_db) {
    // Multiple enties with the same URL are fine as the confidence may be
    // different.
    tracked_urls_.push_back(std::make_pair(match.destination_url, confidence));
    UMA_HISTOGRAM_COUNTS_100("AutocompleteActionPredictor.Confidence",
                             confidence * 100);
  }

  // Map the confidence to an action.
  Action action = ACTION_NONE;
  for (int i = 0; i < LAST_PREDICT_ACTION; ++i) {
    if (confidence >= kConfidenceCutoff[i]) {
      action = static_cast<Action>(i);
      break;
    }
  }

  // Downgrade prerender to preconnect if this is a search match or if omnibox
  // prerendering is disabled. There are cases when Instant will not handle a
  // search suggestion and in those cases it would be good to prerender the
  // search results, however search engines have not been set up to correctly
  // handle being prerendered and until they are we should avoid it.
  // http://crbug.com/117495
  if (action == ACTION_PRERENDER &&
      (AutocompleteMatch::IsSearchType(match.type) ||
       !prerender::IsOmniboxEnabled(profile_))) {
    action = ACTION_PRECONNECT;
  }

  return action;
}

// static
bool AutocompleteActionPredictor::IsPreconnectable(
    const AutocompleteMatch& match) {
  return AutocompleteMatch::IsSearchType(match.type);
}

bool AutocompleteActionPredictor::IsPrerenderAbandonedForTesting() {
  return prerender_handle_ && prerender_handle_->IsAbandoned();
}

void AutocompleteActionPredictor::OnOmniboxOpenedUrl(const OmniboxLog& log) {
  if (!initialized_)
    return;

  // TODO(dominich): The body of this method doesn't need to be run
  // synchronously. Investigate posting it as a task to be run later.

  if (log.text.length() < kMinimumUserTextLength)
    return;

  // Do not attempt to learn from omnibox interactions where the omnibox
  // dropdown is closed.  In these cases the user text (|log.text|) that we
  // learn from is either empty or effectively identical to the destination
  // string.  In either case, it can't teach us much.  Also do not attempt
  // to learn from paste-and-go actions even if the popup is open because
  // the paste-and-go destination has no relation to whatever text the user
  // may have typed.
  if (!log.is_popup_open || log.is_paste_and_go)
    return;

  // Abandon the current prerender. If it is to be used, it will be used very
  // soon, so use the lower timeout.
  if (prerender_handle_) {
    prerender_handle_->OnNavigateAway();
    // Don't release |prerender_handle_| so it is canceled if it survives to the
    // next StartPrerendering call.
  }

  UMA_HISTOGRAM_BOOLEAN(
      "Prerender.OmniboxNavigationsCouldPrerender",
      prerender::IsOmniboxEnabled(profile_));

  const AutocompleteMatch& match = log.result.match_at(log.selected_index);
  const GURL& opened_url = match.destination_url;
  const base::string16 lower_user_text(base::i18n::ToLower(log.text));

  // Traverse transitional matches for those that have a user_text that is a
  // prefix of |lower_user_text|.
  std::vector<AutocompleteActionPredictorTable::Row> rows_to_add;
  std::vector<AutocompleteActionPredictorTable::Row> rows_to_update;

  for (std::vector<TransitionalMatch>::const_iterator it =
        transitional_matches_.begin(); it != transitional_matches_.end();
        ++it) {
    if (!base::StartsWith(lower_user_text, it->user_text,
                          base::CompareCase::SENSITIVE))
      continue;

    // Add entries to the database for those matches.
    for (std::vector<GURL>::const_iterator url_it = it->urls.begin();
          url_it != it->urls.end(); ++url_it) {
      DCHECK(it->user_text.length() >= kMinimumUserTextLength);
      const DBCacheKey key = { it->user_text, *url_it };
      const bool is_hit = (*url_it == opened_url);

      AutocompleteActionPredictorTable::Row row;
      row.user_text = key.user_text;
      row.url = key.url;

      DBCacheMap::iterator it = db_cache_.find(key);
      if (it == db_cache_.end()) {
        row.id = base::GenerateGUID();
        row.number_of_hits = is_hit ? 1 : 0;
        row.number_of_misses = is_hit ? 0 : 1;

        rows_to_add.push_back(row);
      } else {
        DCHECK(db_id_cache_.find(key) != db_id_cache_.end());
        row.id = db_id_cache_.find(key)->second;
        row.number_of_hits = it->second.number_of_hits + (is_hit ? 1 : 0);
        row.number_of_misses = it->second.number_of_misses + (is_hit ? 0 : 1);

        rows_to_update.push_back(row);
      }
    }
  }
  if (rows_to_add.size() > 0 || rows_to_update.size() > 0)
    AddAndUpdateRows(rows_to_add, rows_to_update);

  ClearTransitionalMatches();

  // Check against tracked urls and log accuracy for the confidence we
  // predicted.
  for (std::vector<std::pair<GURL, double> >::const_iterator it =
       tracked_urls_.begin(); it != tracked_urls_.end();
       ++it) {
    if (opened_url == it->first) {
      UMA_HISTOGRAM_COUNTS_100("AutocompleteActionPredictor.AccurateCount",
                               it->second * 100);
    }
  }
  tracked_urls_.clear();
}

void AutocompleteActionPredictor::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME, type);
  CreateLocalCachesFromDatabase();
  notification_registrar_.Remove(
      this, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
      content::NotificationService::AllSources());
}

void AutocompleteActionPredictor::CreateLocalCachesFromDatabase() {
  // Create local caches using the database as loaded. We will garbage collect
  // rows from the caches and the database once the history service is
  // available.
  auto rows =
      std::make_unique<std::vector<AutocompleteActionPredictorTable::Row>>();
  auto* rows_ptr = rows.get();
  table_->GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&AutocompleteActionPredictorTable::GetAllRows, table_,
                     rows_ptr),
      base::BindOnce(&AutocompleteActionPredictor::CreateCaches, AsWeakPtr(),
                     std::move(rows)));
}

void AutocompleteActionPredictor::DeleteAllRows() {
  DCHECK(initialized_);

  db_cache_.clear();
  db_id_cache_.clear();

  if (table_.get()) {
    table_->GetTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&AutocompleteActionPredictorTable::DeleteAllRows,
                       table_));
  }

  UMA_HISTOGRAM_ENUMERATION("AutocompleteActionPredictor.DatabaseAction",
                            DATABASE_ACTION_DELETE_ALL, DATABASE_ACTION_COUNT);
}

void AutocompleteActionPredictor::DeleteRowsFromCaches(
    const history::URLRows& rows,
    std::vector<AutocompleteActionPredictorTable::Row::Id>* id_list) {
  DCHECK(initialized_);
  DCHECK(id_list);

  for (DBCacheMap::iterator it = db_cache_.begin(); it != db_cache_.end();) {
    if (std::find_if(rows.begin(), rows.end(),
                     history::URLRow::URLRowHasURL(it->first.url)) !=
        rows.end()) {
      const DBIdCacheMap::iterator id_it = db_id_cache_.find(it->first);
      DCHECK(id_it != db_id_cache_.end());
      id_list->push_back(id_it->second);
      db_id_cache_.erase(id_it);
      db_cache_.erase(it++);
    } else {
      ++it;
    }
  }
}

void AutocompleteActionPredictor::AddAndUpdateRows(
    const AutocompleteActionPredictorTable::Rows& rows_to_add,
    const AutocompleteActionPredictorTable::Rows& rows_to_update) {
  if (!initialized_)
    return;

  for (AutocompleteActionPredictorTable::Rows::const_iterator it =
       rows_to_add.begin(); it != rows_to_add.end(); ++it) {
    const DBCacheKey key = { it->user_text, it->url };
    DBCacheValue value = { it->number_of_hits, it->number_of_misses };

    DCHECK(db_cache_.find(key) == db_cache_.end());

    db_cache_[key] = value;
    db_id_cache_[key] = it->id;
    UMA_HISTOGRAM_ENUMERATION("AutocompleteActionPredictor.DatabaseAction",
                              DATABASE_ACTION_ADD, DATABASE_ACTION_COUNT);
  }
  for (AutocompleteActionPredictorTable::Rows::const_iterator it =
       rows_to_update.begin(); it != rows_to_update.end(); ++it) {
    const DBCacheKey key = { it->user_text, it->url };

    DBCacheMap::iterator db_it = db_cache_.find(key);
    DCHECK(db_it != db_cache_.end());
    DCHECK(db_id_cache_.find(key) != db_id_cache_.end());

    db_it->second.number_of_hits = it->number_of_hits;
    db_it->second.number_of_misses = it->number_of_misses;
    UMA_HISTOGRAM_ENUMERATION("AutocompleteActionPredictor.DatabaseAction",
                              DATABASE_ACTION_UPDATE, DATABASE_ACTION_COUNT);
  }

  if (table_.get()) {
    table_->GetTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&AutocompleteActionPredictorTable::AddAndUpdateRows,
                       table_, rows_to_add, rows_to_update));
  }
}

void AutocompleteActionPredictor::CreateCaches(
    std::unique_ptr<std::vector<AutocompleteActionPredictorTable::Row>> rows) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(!profile_->IsOffTheRecord());
  DCHECK(!initialized_);
  DCHECK(db_cache_.empty());
  DCHECK(db_id_cache_.empty());

  for (std::vector<AutocompleteActionPredictorTable::Row>::const_iterator it =
       rows->begin(); it != rows->end(); ++it) {
    const DBCacheKey key = { it->user_text, it->url };
    const DBCacheValue value = { it->number_of_hits, it->number_of_misses };
    db_cache_[key] = value;
    db_id_cache_[key] = it->id;
  }

  // If the history service is ready, delete any old or invalid entries.
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(profile_,
                                           ServiceAccessType::EXPLICIT_ACCESS);
  if (history_service) {
    TryDeleteOldEntries(history_service);
    history_service_observer_.Add(history_service);
  }
}

void AutocompleteActionPredictor::TryDeleteOldEntries(
    history::HistoryService* service) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(!profile_->IsOffTheRecord());
  DCHECK(!initialized_);

  if (!service)
    return;

  history::URLDatabase* url_db = service->InMemoryDatabase();
  if (!url_db)
    return;

  DeleteOldEntries(url_db);
}

void AutocompleteActionPredictor::DeleteOldEntries(
    history::URLDatabase* url_db) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(!profile_->IsOffTheRecord());
  DCHECK(!initialized_);
  DCHECK(table_.get());

  std::vector<AutocompleteActionPredictorTable::Row::Id> ids_to_delete;
  DeleteOldIdsFromCaches(url_db, &ids_to_delete);

  table_->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AutocompleteActionPredictorTable::DeleteRows,
                                table_, ids_to_delete));

  FinishInitialization();
  if (incognito_predictor_)
    incognito_predictor_->CopyFromMainProfile();
}

void AutocompleteActionPredictor::DeleteOldIdsFromCaches(
    history::URLDatabase* url_db,
    std::vector<AutocompleteActionPredictorTable::Row::Id>* id_list) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(!profile_->IsOffTheRecord());
  DCHECK(url_db);
  DCHECK(id_list);

  for (DBCacheMap::iterator it = db_cache_.begin(); it != db_cache_.end();) {
    history::URLRow url_row;

    if ((url_db->GetRowForURL(it->first.url, &url_row) == 0) ||
        ((base::Time::Now() - url_row.last_visit()).InDays() >
         kMaximumDaysToKeepEntry)) {
      const DBIdCacheMap::iterator id_it = db_id_cache_.find(it->first);
      DCHECK(id_it != db_id_cache_.end());
      id_list->push_back(id_it->second);
      db_id_cache_.erase(id_it);
      db_cache_.erase(it++);
    } else {
      ++it;
    }
  }
}

void AutocompleteActionPredictor::CopyFromMainProfile() {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(profile_->IsOffTheRecord());
  DCHECK(!initialized_);
  DCHECK(main_profile_predictor_);
  DCHECK(main_profile_predictor_->initialized_);

  db_cache_ = main_profile_predictor_->db_cache_;
  db_id_cache_ = main_profile_predictor_->db_id_cache_;
  FinishInitialization();
}

void AutocompleteActionPredictor::FinishInitialization() {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(!initialized_);
  initialized_ = true;
}

double AutocompleteActionPredictor::CalculateConfidence(
    const base::string16& user_text,
    const AutocompleteMatch& match,
    bool* is_in_db) const {
  const DBCacheKey key = { user_text, match.destination_url };

  *is_in_db = false;
  if (user_text.length() < kMinimumUserTextLength)
    return 0.0;

  const DBCacheMap::const_iterator iter = db_cache_.find(key);
  if (iter == db_cache_.end())
    return 0.0;

  *is_in_db = true;
  return CalculateConfidenceForDbEntry(iter);
}

double AutocompleteActionPredictor::CalculateConfidenceForDbEntry(
    DBCacheMap::const_iterator iter) const {
  const DBCacheValue& value = iter->second;
  if (value.number_of_hits < kMinimumNumberOfHits)
    return 0.0;

  const double number_of_hits = static_cast<double>(value.number_of_hits);
  return number_of_hits / (number_of_hits + value.number_of_misses);
}

void AutocompleteActionPredictor::Shutdown() {
  history_service_observer_.RemoveAll();
}

void AutocompleteActionPredictor::OnURLsDeleted(
    history::HistoryService* history_service,
    const history::DeletionInfo& deletion_info) {
  DCHECK(initialized_);

  if (deletion_info.IsAllHistory()) {
    DeleteAllRows();
    return;
  }

  std::vector<AutocompleteActionPredictorTable::Row::Id> id_list;
  DeleteRowsFromCaches(deletion_info.deleted_rows(), &id_list);

  if (!deletion_info.is_from_expiration() && history_service) {
    auto* url_db = history_service->InMemoryDatabase();
    if (url_db)
      DeleteOldIdsFromCaches(url_db, &id_list);
  }

  if (table_.get()) {
    table_->GetTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&AutocompleteActionPredictorTable::DeleteRows,
                                  table_, std::move(id_list)));
  }

  UMA_HISTOGRAM_ENUMERATION("AutocompleteActionPredictor.DatabaseAction",
                            DATABASE_ACTION_DELETE_SOME, DATABASE_ACTION_COUNT);
}

void AutocompleteActionPredictor::OnHistoryServiceLoaded(
    history::HistoryService* history_service) {
  if (!initialized_)
    TryDeleteOldEntries(history_service);
}

AutocompleteActionPredictor::TransitionalMatch::TransitionalMatch() {
}

AutocompleteActionPredictor::TransitionalMatch::TransitionalMatch(
    const TransitionalMatch& other) = default;

AutocompleteActionPredictor::TransitionalMatch::~TransitionalMatch() {
}

}  // namespace predictors
