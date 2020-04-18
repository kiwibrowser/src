// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/metrics/notification_metrics_logger_factory.h"

#include "chrome/browser/notifications/metrics/notification_metrics_logger.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
NotificationMetricsLogger*
NotificationMetricsLoggerFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<NotificationMetricsLogger*>(
      GetInstance()->GetServiceForBrowserContext(browser_context,
                                                 true /* create */));
}

// static
NotificationMetricsLoggerFactory*
NotificationMetricsLoggerFactory::GetInstance() {
  return base::Singleton<NotificationMetricsLoggerFactory>::get();
}

NotificationMetricsLoggerFactory::NotificationMetricsLoggerFactory()
    : BrowserContextKeyedServiceFactory(
          "NotificationMetricsLogger",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* NotificationMetricsLoggerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NotificationMetricsLogger();
}

content::BrowserContext*
NotificationMetricsLoggerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
