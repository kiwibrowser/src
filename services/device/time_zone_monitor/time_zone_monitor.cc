// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/time_zone_monitor/time_zone_monitor.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "third_party/icu/source/common/unicode/unistr.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

#if defined(OS_ANDROID)
#include "base/android/timezone_utils.h"  // nogncheck
#endif

namespace device {

TimeZoneMonitor::TimeZoneMonitor() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

TimeZoneMonitor::~TimeZoneMonitor() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void TimeZoneMonitor::Bind(device::mojom::TimeZoneMonitorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void TimeZoneMonitor::NotifyClients() {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(OS_CHROMEOS)
  // On CrOS, ICU's default tz is already set to a new zone. No
  // need to redetect it with detectHostTimeZone().
  std::unique_ptr<icu::TimeZone> new_zone(icu::TimeZone::createDefault());
#else
#if defined(OS_ANDROID)
  base::string16 timezone_id = base::android::GetDefaultTimeZoneId();
  std::unique_ptr<icu::TimeZone> new_zone(icu::TimeZone::createTimeZone(
      icu::UnicodeString(FALSE, timezone_id.data(), timezone_id.length())));
#else
  std::unique_ptr<icu::TimeZone> new_zone(icu::TimeZone::detectHostTimeZone());
#endif
#if defined(OS_LINUX)
  // We get here multiple times on Linux per a single tz change, but
  // want to update the ICU default zone and notify renderer only once.
  std::unique_ptr<icu::TimeZone> current_zone(icu::TimeZone::createDefault());
  if (*current_zone == *new_zone) {
    VLOG(1) << "timezone already updated";
    return;
  }
#endif
#endif  // OS_CHROMEOS
  icu::UnicodeString zone_id;
  std::string zone_id_str;
  new_zone->getID(zone_id).toUTF8String(zone_id_str);
  VLOG(1) << "timezone reset to " << zone_id_str;

#if !defined(OS_CHROMEOS)
  icu::TimeZone::adoptDefault(new_zone.release());
#endif

  clients_.ForAllPtrs(
      [&zone_id_str](device::mojom::TimeZoneMonitorClient* client) {
        client->OnTimeZoneChange(zone_id_str);
      });
}

void TimeZoneMonitor::AddClient(
    device::mojom::TimeZoneMonitorClientPtr client) {
  clients_.AddPtr(std::move(client));
}

}  // namespace device
