// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/keyword_web_data_service.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "components/search_engines/keyword_table.h"
#include "components/search_engines/template_url_data.h"
#include "components/webdata/common/web_data_results.h"
#include "components/webdata/common/web_database_service.h"

using base::Bind;

WDKeywordsResult::WDKeywordsResult()
  : default_search_provider_id(0),
    builtin_keyword_version(0) {
}

WDKeywordsResult::WDKeywordsResult(const WDKeywordsResult& other) = default;

WDKeywordsResult::~WDKeywordsResult() {}

KeywordWebDataService::BatchModeScoper::BatchModeScoper(
    KeywordWebDataService* service)
    : service_(service) {
  if (service_)
    service_->AdjustBatchModeLevel(true);
}

KeywordWebDataService::BatchModeScoper::~BatchModeScoper() {
  if (service_)
    service_->AdjustBatchModeLevel(false);
}

KeywordWebDataService::KeywordWebDataService(
    scoped_refptr<WebDatabaseService> wdbs,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    const ProfileErrorCallback& callback)
    : WebDataServiceBase(wdbs, callback, ui_task_runner),
      batch_mode_level_(0) {}

void KeywordWebDataService::AddKeyword(const TemplateURLData& data) {
  if (batch_mode_level_) {
    queued_keyword_operations_.push_back(
        KeywordTable::Operation(KeywordTable::ADD, data));
  } else {
    AdjustBatchModeLevel(true);
    AddKeyword(data);
    AdjustBatchModeLevel(false);
  }
}

void KeywordWebDataService::RemoveKeyword(TemplateURLID id) {
  if (batch_mode_level_) {
    TemplateURLData data;
    data.id = id;
    queued_keyword_operations_.push_back(
        KeywordTable::Operation(KeywordTable::REMOVE, data));
  } else {
    AdjustBatchModeLevel(true);
    RemoveKeyword(id);
    AdjustBatchModeLevel(false);
  }
}

void KeywordWebDataService::UpdateKeyword(const TemplateURLData& data) {
  if (batch_mode_level_) {
    queued_keyword_operations_.push_back(
        KeywordTable::Operation(KeywordTable::UPDATE, data));
  } else {
    AdjustBatchModeLevel(true);
    UpdateKeyword(data);
    AdjustBatchModeLevel(false);
  }
}

WebDataServiceBase::Handle KeywordWebDataService::GetKeywords(
    WebDataServiceConsumer* consumer) {
  return wdbs_->ScheduleDBTaskWithResult(
      FROM_HERE, Bind(&KeywordWebDataService::GetKeywordsImpl, this), consumer);
}

void KeywordWebDataService::SetDefaultSearchProviderID(TemplateURLID id) {
  wdbs_->ScheduleDBTask(
      FROM_HERE,
      Bind(&KeywordWebDataService::SetDefaultSearchProviderIDImpl, this, id));
}

void KeywordWebDataService::SetBuiltinKeywordVersion(int version) {
  wdbs_->ScheduleDBTask(
      FROM_HERE,
      Bind(&KeywordWebDataService::SetBuiltinKeywordVersionImpl,
           this, version));
}

KeywordWebDataService::~KeywordWebDataService() {
  DCHECK(!batch_mode_level_);
}

void KeywordWebDataService::AdjustBatchModeLevel(bool entering_batch_mode) {
  if (entering_batch_mode) {
    ++batch_mode_level_;
  } else {
    DCHECK(batch_mode_level_);
    --batch_mode_level_;
    if (!batch_mode_level_ && !queued_keyword_operations_.empty()) {
      wdbs_->ScheduleDBTask(
          FROM_HERE,
          Bind(&KeywordWebDataService::PerformKeywordOperationsImpl, this,
               queued_keyword_operations_));
      queued_keyword_operations_.clear();
    }
  }
}

WebDatabase::State KeywordWebDataService::PerformKeywordOperationsImpl(
    const KeywordTable::Operations& operations,
    WebDatabase* db) {
  return KeywordTable::FromWebDatabase(db)->PerformOperations(operations) ?
      WebDatabase::COMMIT_NEEDED : WebDatabase::COMMIT_NOT_NEEDED;
}

std::unique_ptr<WDTypedResult> KeywordWebDataService::GetKeywordsImpl(
    WebDatabase* db) {
  std::unique_ptr<WDTypedResult> result_ptr;
  WDKeywordsResult result;
  if (KeywordTable::FromWebDatabase(db)->GetKeywords(&result.keywords)) {
    result.default_search_provider_id =
        KeywordTable::FromWebDatabase(db)->GetDefaultSearchProviderID();
    result.builtin_keyword_version =
        KeywordTable::FromWebDatabase(db)->GetBuiltinKeywordVersion();
    result_ptr.reset(new WDResult<WDKeywordsResult>(KEYWORDS_RESULT, result));
  }
  return result_ptr;
}

WebDatabase::State KeywordWebDataService::SetDefaultSearchProviderIDImpl(
    TemplateURLID id,
    WebDatabase* db) {
  return KeywordTable::FromWebDatabase(db)->SetDefaultSearchProviderID(id) ?
      WebDatabase::COMMIT_NEEDED : WebDatabase::COMMIT_NOT_NEEDED;
}

WebDatabase::State KeywordWebDataService::SetBuiltinKeywordVersionImpl(
    int version,
    WebDatabase* db) {
  return KeywordTable::FromWebDatabase(db)->SetBuiltinKeywordVersion(version) ?
      WebDatabase::COMMIT_NEEDED : WebDatabase::COMMIT_NOT_NEEDED;
}
