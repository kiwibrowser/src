// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/test_safe_browsing_service.h"

#include "base/strings/string_util.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/ping_manager.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "components/safe_browsing/db/database_manager.h"
#include "components/safe_browsing/db/test_database_manager.h"
#include "components/safe_browsing/db/v4_feature_list.h"

namespace safe_browsing {

// TestSafeBrowsingService functions:
TestSafeBrowsingService::TestSafeBrowsingService(
    V4FeatureList::V4UsageStatus v4_usage_status)
    : SafeBrowsingService(v4_usage_status),
      protocol_manager_delegate_disabled_(false),
      serialized_download_report_(base::EmptyString()) {}

TestSafeBrowsingService::~TestSafeBrowsingService() {}

SafeBrowsingProtocolConfig TestSafeBrowsingService::GetProtocolConfig() const {
  if (protocol_config_)
    return *protocol_config_;
  return SafeBrowsingService::GetProtocolConfig();
}

V4ProtocolConfig TestSafeBrowsingService::GetV4ProtocolConfig() const {
  if (v4_protocol_config_)
    return *v4_protocol_config_;
  return SafeBrowsingService::GetV4ProtocolConfig();
}

std::string TestSafeBrowsingService::serilized_download_report() {
  return serialized_download_report_;
}

void TestSafeBrowsingService::ClearDownloadReport() {
  serialized_download_report_.clear();
}

void TestSafeBrowsingService::SetDatabaseManager(
    TestSafeBrowsingDatabaseManager* database_manager) {
  database_manager_ = database_manager;
  // Since TestSafeBrowsingDatabaseManager does not implement
  // SafeBrowsingProtocolManagerDelegate, when it is used we need to disable
  // protocol_manager_delegate.
  protocol_manager_delegate_disabled_ = true;
}

void TestSafeBrowsingService::SetUIManager(
    TestSafeBrowsingUIManager* ui_manager) {
  ui_manager->SetSafeBrowsingService(this);
  ui_manager_ = ui_manager;
}

SafeBrowsingDatabaseManager* TestSafeBrowsingService::CreateDatabaseManager() {
  if (database_manager_)
    return database_manager_.get();
  return SafeBrowsingService::CreateDatabaseManager();
}

SafeBrowsingUIManager* TestSafeBrowsingService::CreateUIManager() {
  if (ui_manager_)
    return ui_manager_.get();
  return SafeBrowsingService::CreateUIManager();
}

SafeBrowsingProtocolManagerDelegate*
TestSafeBrowsingService::GetProtocolManagerDelegate() {
  if (protocol_manager_delegate_disabled_)
    return nullptr;
  return SafeBrowsingService::GetProtocolManagerDelegate();
}

void TestSafeBrowsingService::SendSerializedDownloadReport(
    const std::string& report) {
  serialized_download_report_ = report;
}

void TestSafeBrowsingService::SetProtocolConfig(
    SafeBrowsingProtocolConfig* protocol_config) {
  protocol_config_.reset(protocol_config);
}

void TestSafeBrowsingService::SetV4ProtocolConfig(
    V4ProtocolConfig* v4_protocol_config) {
  v4_protocol_config_.reset(v4_protocol_config);
}

// TestSafeBrowsingServiceFactory functions:
TestSafeBrowsingServiceFactory::TestSafeBrowsingServiceFactory(
    V4FeatureList::V4UsageStatus v4_usage_status)
    : test_safe_browsing_service_(nullptr),
      test_protocol_config_(nullptr),
      v4_usage_status_(v4_usage_status) {}

TestSafeBrowsingServiceFactory::~TestSafeBrowsingServiceFactory() {}

SafeBrowsingService*
TestSafeBrowsingServiceFactory::CreateSafeBrowsingService() {
  // Instantiate TestSafeBrowsingService.
  test_safe_browsing_service_ = new TestSafeBrowsingService(v4_usage_status_);
  // Plug-in test member clases accordingly.
  if (test_ui_manager_)
    test_safe_browsing_service_->SetUIManager(test_ui_manager_.get());
  if (test_database_manager_) {
    test_safe_browsing_service_->SetDatabaseManager(
        test_database_manager_.get());
  }
  if (test_protocol_config_)
    test_safe_browsing_service_->SetProtocolConfig(test_protocol_config_);
  return test_safe_browsing_service_;
}

TestSafeBrowsingService*
TestSafeBrowsingServiceFactory::test_safe_browsing_service() {
  return test_safe_browsing_service_;
}

void TestSafeBrowsingServiceFactory::SetTestUIManager(
    TestSafeBrowsingUIManager* ui_manager) {
  test_ui_manager_ = ui_manager;
}

void TestSafeBrowsingServiceFactory::SetTestDatabaseManager(
    TestSafeBrowsingDatabaseManager* database_manager) {
  test_database_manager_ = database_manager;
}

void TestSafeBrowsingServiceFactory::SetTestProtocolConfig(
    const SafeBrowsingProtocolConfig& protocol_config) {
  test_protocol_config_ = new SafeBrowsingProtocolConfig(protocol_config);
}

// TestSafeBrowsingUIManager functions:
TestSafeBrowsingUIManager::TestSafeBrowsingUIManager()
    : SafeBrowsingUIManager(nullptr) {}

TestSafeBrowsingUIManager::TestSafeBrowsingUIManager(
    const scoped_refptr<SafeBrowsingService>& service)
    : SafeBrowsingUIManager(service) {}

void TestSafeBrowsingUIManager::SetSafeBrowsingService(
    SafeBrowsingService* sb_service) {
  sb_service_ = sb_service;
}

void TestSafeBrowsingUIManager::SendSerializedThreatDetails(
    const std::string& serialized) {
  details_.push_back(serialized);
}

std::list<std::string>* TestSafeBrowsingUIManager::GetThreatDetails() {
  return &details_;
}

TestSafeBrowsingUIManager::~TestSafeBrowsingUIManager() {}
}  // namespace safe_browsing
