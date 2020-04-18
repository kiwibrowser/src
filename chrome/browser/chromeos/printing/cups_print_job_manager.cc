// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/cups_print_job_manager.h"

#include <algorithm>

#include "chrome/browser/chromeos/printing/cups_print_job_notification_manager.h"

namespace chromeos {

CupsPrintJobManager::CupsPrintJobManager(Profile* profile) : profile_(profile) {
  notification_manager_.reset(
      new CupsPrintJobNotificationManager(profile, this));
}

CupsPrintJobManager::~CupsPrintJobManager() {
  notification_manager_.reset();
  profile_ = nullptr;
}

void CupsPrintJobManager::Shutdown() {}

void CupsPrintJobManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CupsPrintJobManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CupsPrintJobManager::NotifyJobCreated(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobCreated(job);
}

void CupsPrintJobManager::NotifyJobStarted(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobStarted(job);
}

void CupsPrintJobManager::NotifyJobUpdated(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobUpdated(job);
}

void CupsPrintJobManager::NotifyJobResumed(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobResumed(job);
}

void CupsPrintJobManager::NotifyJobSuspended(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobSuspended(job);
}

void CupsPrintJobManager::NotifyJobCanceled(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobCancelled(job);
}

void CupsPrintJobManager::NotifyJobError(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobError(job);
}

void CupsPrintJobManager::NotifyJobDone(base::WeakPtr<CupsPrintJob> job) {
  for (Observer& observer : observers_)
    observer.OnPrintJobDone(job);
}

}  // namespace chromeos
