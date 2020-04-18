// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/text/bytes_formatting.h"

#include "base/i18n/number_formatting.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

namespace ui {

namespace {

// Byte suffix string constants. These both must match the DataUnits enum.
const int kByteStrings[] = {
  IDS_APP_BYTES,
  IDS_APP_KIBIBYTES,
  IDS_APP_MEBIBYTES,
  IDS_APP_GIBIBYTES,
  IDS_APP_TEBIBYTES,
  IDS_APP_PEBIBYTES
};

const int kSpeedStrings[] = {
  IDS_APP_BYTES_PER_SECOND,
  IDS_APP_KIBIBYTES_PER_SECOND,
  IDS_APP_MEBIBYTES_PER_SECOND,
  IDS_APP_GIBIBYTES_PER_SECOND,
  IDS_APP_TEBIBYTES_PER_SECOND,
  IDS_APP_PEBIBYTES_PER_SECOND
};

base::string16 FormatBytesInternal(int64_t bytes,
                                   DataUnits units,
                                   bool show_units,
                                   const int* const suffix) {
  DCHECK(units >= DATA_UNITS_BYTE && units <= DATA_UNITS_PEBIBYTE);
  if (bytes < 0) {
    NOTREACHED() << "Negative bytes value";
    return base::string16();
  }

  // Put the quantity in the right units.
  double unit_amount = static_cast<double>(bytes);
  for (int i = 0; i < units; ++i)
    unit_amount /= 1024.0;

  int fractional_digits = 0;
  if (bytes != 0 && units != DATA_UNITS_BYTE && unit_amount < 100)
    fractional_digits = 1;

  base::string16 result = base::FormatDouble(unit_amount, fractional_digits);

  if (show_units)
    result = l10n_util::GetStringFUTF16(suffix[units], result);

  return result;
}

}  // namespace

DataUnits GetByteDisplayUnits(int64_t bytes) {
  // The byte thresholds at which we display amounts. A byte count is displayed
  // in unit U when kUnitThresholds[U] <= bytes < kUnitThresholds[U+1].
  // This must match the DataUnits enum.
  static const int64_t kUnitThresholds[] = {
      0,                // DATA_UNITS_BYTE,
      3 * (1LL << 10),  // DATA_UNITS_KIBIBYTE,
      2 * (1LL << 20),  // DATA_UNITS_MEBIBYTE,
      1LL << 30,        // DATA_UNITS_GIBIBYTE,
      1LL << 40,        // DATA_UNITS_TEBIBYTE,
      1LL << 50         // DATA_UNITS_PEBIBYTE,
  };

  if (bytes < 0) {
    NOTREACHED() << "Negative bytes value";
    return DATA_UNITS_BYTE;
  }

  int unit_index = arraysize(kUnitThresholds);
  while (--unit_index > 0) {
    if (bytes >= kUnitThresholds[unit_index])
      break;
  }

  DCHECK(unit_index >= DATA_UNITS_BYTE && unit_index <= DATA_UNITS_PEBIBYTE);
  return DataUnits(unit_index);
}

base::string16 FormatBytesWithUnits(int64_t bytes,
                                    DataUnits units,
                                    bool show_units) {
  return FormatBytesInternal(bytes, units, show_units, kByteStrings);
}

base::string16 FormatSpeedWithUnits(int64_t bytes,
                                    DataUnits units,
                                    bool show_units) {
  return FormatBytesInternal(bytes, units, show_units, kSpeedStrings);
}

base::string16 FormatBytes(int64_t bytes) {
  return FormatBytesWithUnits(bytes, GetByteDisplayUnits(bytes), true);
}

base::string16 FormatSpeed(int64_t bytes) {
  return FormatSpeedWithUnits(bytes, GetByteDisplayUnits(bytes), true);
}

}  // namespace ui
