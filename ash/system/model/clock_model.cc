// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/model/clock_model.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/date/clock_observer.h"
#include "ash/system/tray/system_tray_controller.h"
#include "chromeos/dbus/dbus_thread_manager.h"

namespace ash {

ClockModel::ClockModel() : hour_clock_type_(base::GetHourClockType()) {
  chromeos::DBusThreadManager::Get()->GetSystemClockClient()->AddObserver(this);
  chromeos::system::TimezoneSettings::GetInstance()->AddObserver(this);
  can_set_time_ =
      chromeos::DBusThreadManager::Get()->GetSystemClockClient()->CanSetTime();
}

ClockModel::~ClockModel() {
  chromeos::DBusThreadManager::Get()->GetSystemClockClient()->RemoveObserver(
      this);
  chromeos::system::TimezoneSettings::GetInstance()->RemoveObserver(this);
}

void ClockModel::AddObserver(ClockObserver* observer) {
  observers_.AddObserver(observer);
}

void ClockModel::RemoveObserver(ClockObserver* observer) {
  observers_.RemoveObserver(observer);
}

void ClockModel::SetUse24HourClock(bool use_24_hour) {
  hour_clock_type_ = use_24_hour ? base::k24HourClock : base::k12HourClock;
  NotifyDateFormatChanged();
}

bool ClockModel::IsLoggedIn() {
  return Shell::Get()->session_controller()->login_status() ==
         LoginStatus::NOT_LOGGED_IN;
}

void ClockModel::ShowDateSettings() {
  Shell::Get()->system_tray_controller()->ShowDateSettings();
}

void ClockModel::ShowSetTimeDialog() {
  Shell::Get()->system_tray_controller()->ShowSetTimeDialog();
}

void ClockModel::NotifyRefreshClock() {
  for (auto& observer : observers_)
    observer.Refresh();
}

void ClockModel::NotifyDateFormatChanged() {
  for (auto& observer : observers_)
    observer.OnDateFormatChanged();
}

void ClockModel::NotifySystemClockTimeUpdated() {
  for (auto& observer : observers_)
    observer.OnSystemClockTimeUpdated();
}

void ClockModel::NotifySystemClockCanSetTimeChanged(bool can_set_time) {
  for (auto& observer : observers_)
    observer.OnSystemClockCanSetTimeChanged(can_set_time);
}

void ClockModel::SystemClockUpdated() {
  NotifySystemClockTimeUpdated();
}

void ClockModel::SystemClockCanSetTimeChanged(bool can_set_time) {
  can_set_time_ = can_set_time;
  NotifySystemClockCanSetTimeChanged(can_set_time_);
}

void ClockModel::TimezoneChanged(const icu::TimeZone& timezone) {
  NotifyRefreshClock();
}

}  // namespace ash
