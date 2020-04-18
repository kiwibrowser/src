// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CLOUD_DEVICES_COMMON_PRINTER_DESCRIPTION_H_
#define COMPONENTS_CLOUD_DEVICES_COMMON_PRINTER_DESCRIPTION_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/logging.h"
#include "components/cloud_devices/common/description_items.h"

// Defines printer options, CDD and CJT items.
// https://developers.google.com/cloud-print/docs/cdd

namespace cloud_devices {

namespace printer {

typedef std::string ContentType;

enum DocumentSheetBack { NORMAL, ROTATED, MANUAL_TUMBLE, FLIPPED };

enum PwgDocumentTypeSupported {
  SGRAY_8 = 22,
  SRGB_8 = 23,
};

struct PwgRasterConfig {
  PwgRasterConfig();
  ~PwgRasterConfig();

  std::vector<PwgDocumentTypeSupported> document_types_supported;
  DocumentSheetBack document_sheet_back;
  bool reverse_order_streaming;
  bool rotate_all_pages;
};

enum ColorType {
  STANDARD_COLOR,
  STANDARD_MONOCHROME,
  CUSTOM_COLOR,
  CUSTOM_MONOCHROME,
  AUTO_COLOR,
};

struct Color {
  Color();
  explicit Color(ColorType type);

  bool IsValid() const;
  bool operator==(const Color& other) const;
  bool operator!=(const Color& other) const { return !(*this == other); }

  ColorType type;
  std::string vendor_id;
  std::string custom_display_name;
};

enum DuplexType {
  NO_DUPLEX,
  LONG_EDGE,
  SHORT_EDGE,
};

enum OrientationType {
  PORTRAIT,
  LANDSCAPE,
  AUTO_ORIENTATION,
};

enum MarginsType {
  NO_MARGINS,
  STANDARD_MARGINS,
  CUSTOM_MARGINS,
};

struct Margins {
  Margins();
  Margins(MarginsType type,
          int32_t top_um,
          int32_t right_um,
          int32_t bottom_um,
          int32_t left_um);

  bool operator==(const Margins& other) const;
  bool operator!=(const Margins& other) const { return !(*this == other); }

  MarginsType type;
  int32_t top_um;
  int32_t right_um;
  int32_t bottom_um;
  int32_t left_um;
};

struct Dpi {
  Dpi();
  Dpi(int32_t horizontal, int32_t vertical);

  bool IsValid() const;
  bool operator==(const Dpi& other) const;
  bool operator!=(const Dpi& other) const { return !(*this == other); }

  int32_t horizontal;
  int32_t vertical;
};

enum FitToPageType {
  NO_FITTING,
  FIT_TO_PAGE,
  GROW_TO_PAGE,
  SHRINK_TO_PAGE,
  FILL_PAGE,
};

enum MediaType {
  CUSTOM_MEDIA,

  // North American standard sheet media names.
  NA_INDEX_3X5,
  NA_PERSONAL,
  NA_MONARCH,
  NA_NUMBER_9,
  NA_INDEX_4X6,
  NA_NUMBER_10,
  NA_A2,
  NA_NUMBER_11,
  NA_NUMBER_12,
  NA_5X7,
  NA_INDEX_5X8,
  NA_NUMBER_14,
  NA_INVOICE,
  NA_INDEX_4X6_EXT,
  NA_6X9,
  NA_C5,
  NA_7X9,
  NA_EXECUTIVE,
  NA_GOVT_LETTER,
  NA_GOVT_LEGAL,
  NA_QUARTO,
  NA_LETTER,
  NA_FANFOLD_EUR,
  NA_LETTER_PLUS,
  NA_FOOLSCAP,
  NA_LEGAL,
  NA_SUPER_A,
  NA_9X11,
  NA_ARCH_A,
  NA_LETTER_EXTRA,
  NA_LEGAL_EXTRA,
  NA_10X11,
  NA_10X13,
  NA_10X14,
  NA_10X15,
  NA_11X12,
  NA_EDP,
  NA_FANFOLD_US,
  NA_11X15,
  NA_LEDGER,
  NA_EUR_EDP,
  NA_ARCH_B,
  NA_12X19,
  NA_B_PLUS,
  NA_SUPER_B,
  NA_C,
  NA_ARCH_C,
  NA_D,
  NA_ARCH_D,
  NA_ASME_F,
  NA_WIDE_FORMAT,
  NA_E,
  NA_ARCH_E,
  NA_F,

  // Chinese standard sheet media size names.
  ROC_16K,
  ROC_8K,
  PRC_32K,
  PRC_1,
  PRC_2,
  PRC_4,
  PRC_5,
  PRC_8,
  PRC_6,
  PRC_3,
  PRC_16K,
  PRC_7,
  OM_JUURO_KU_KAI,
  OM_PA_KAI,
  OM_DAI_PA_KAI,
  PRC_10,

  // ISO standard sheet media size names.
  ISO_A10,
  ISO_A9,
  ISO_A8,
  ISO_A7,
  ISO_A6,
  ISO_A5,
  ISO_A5_EXTRA,
  ISO_A4,
  ISO_A4_TAB,
  ISO_A4_EXTRA,
  ISO_A3,
  ISO_A4X3,
  ISO_A4X4,
  ISO_A4X5,
  ISO_A4X6,
  ISO_A4X7,
  ISO_A4X8,
  ISO_A4X9,
  ISO_A3_EXTRA,
  ISO_A2,
  ISO_A3X3,
  ISO_A3X4,
  ISO_A3X5,
  ISO_A3X6,
  ISO_A3X7,
  ISO_A1,
  ISO_A2X3,
  ISO_A2X4,
  ISO_A2X5,
  ISO_A0,
  ISO_A1X3,
  ISO_A1X4,
  ISO_2A0,
  ISO_A0X3,
  ISO_B10,
  ISO_B9,
  ISO_B8,
  ISO_B7,
  ISO_B6,
  ISO_B6C4,
  ISO_B5,
  ISO_B5_EXTRA,
  ISO_B4,
  ISO_B3,
  ISO_B2,
  ISO_B1,
  ISO_B0,
  ISO_C10,
  ISO_C9,
  ISO_C8,
  ISO_C7,
  ISO_C7C6,
  ISO_C6,
  ISO_C6C5,
  ISO_C5,
  ISO_C4,
  ISO_C3,
  ISO_C2,
  ISO_C1,
  ISO_C0,
  ISO_DL,
  ISO_RA2,
  ISO_SRA2,
  ISO_RA1,
  ISO_SRA1,
  ISO_RA0,
  ISO_SRA0,

  // Japanese standard sheet media size names.
  JIS_B10,
  JIS_B9,
  JIS_B8,
  JIS_B7,
  JIS_B6,
  JIS_B5,
  JIS_B4,
  JIS_B3,
  JIS_B2,
  JIS_B1,
  JIS_B0,
  JIS_EXEC,
  JPN_CHOU4,
  JPN_HAGAKI,
  JPN_YOU4,
  JPN_CHOU2,
  JPN_CHOU3,
  JPN_OUFUKU,
  JPN_KAHU,
  JPN_KAKU2,

  // Other metric standard sheet media size names.
  OM_SMALL_PHOTO,
  OM_ITALIAN,
  OM_POSTFIX,
  OM_LARGE_PHOTO,
  OM_FOLIO,
  OM_FOLIO_SP,
  OM_INVITE,
};

struct Media {
  Media();

  explicit Media(MediaType type);

  Media(MediaType type, int32_t width_um, int32_t height_um);

  Media(const std::string& custom_display_name,
        const std::string& vendor_id,
        int32_t width_um,
        int32_t height_um);

  Media(const Media& other);

  bool MatchBySize();

  bool IsValid() const;
  bool operator==(const Media& other) const;
  bool operator!=(const Media& other) const { return !(*this == other); }

  MediaType type;
  int32_t width_um;
  int32_t height_um;
  bool is_continuous_feed;
  std::string custom_display_name;
  std::string vendor_id;
};

struct Interval {
  Interval();
  Interval(int32_t start, int32_t end);
  explicit Interval(int32_t start);

  bool operator==(const Interval& other) const;
  bool operator!=(const Interval& other) const { return !(*this == other); }

  int32_t start;
  int32_t end;
};

typedef std::vector<Interval> PageRange;

class ContentTypeTraits;
class PwgRasterConfigTraits;
class ColorTraits;
class DuplexTraits;
class OrientationTraits;
class MarginsTraits;
class DpiTraits;
class FitToPageTraits;
class MediaTraits;
class CopiesTraits;
class PageRangeTraits;
class CollateTraits;

typedef ListCapability<ContentType, ContentTypeTraits> ContentTypesCapability;
typedef ValueCapability<PwgRasterConfig, PwgRasterConfigTraits>
    PwgRasterConfigCapability;
typedef SelectionCapability<Color, ColorTraits> ColorCapability;
typedef SelectionCapability<DuplexType, DuplexTraits> DuplexCapability;
typedef SelectionCapability<OrientationType, OrientationTraits>
    OrientationCapability;
typedef SelectionCapability<Margins, MarginsTraits> MarginsCapability;
typedef SelectionCapability<Dpi, DpiTraits> DpiCapability;
typedef SelectionCapability<FitToPageType, FitToPageTraits> FitToPageCapability;
typedef SelectionCapability<Media, MediaTraits> MediaCapability;
typedef EmptyCapability<class CopiesTraits> CopiesCapability;
typedef EmptyCapability<class PageRangeTraits> PageRangeCapability;
typedef BooleanCapability<class CollateTraits> CollateCapability;
typedef BooleanCapability<class ReverseTraits> ReverseCapability;

typedef TicketItem<PwgRasterConfig, PwgRasterConfigTraits>
    PwgRasterConfigTicketItem;
typedef TicketItem<Color, ColorTraits> ColorTicketItem;
typedef TicketItem<DuplexType, DuplexTraits> DuplexTicketItem;
typedef TicketItem<OrientationType, OrientationTraits> OrientationTicketItem;
typedef TicketItem<Margins, MarginsTraits> MarginsTicketItem;
typedef TicketItem<Dpi, DpiTraits> DpiTicketItem;
typedef TicketItem<FitToPageType, FitToPageTraits> FitToPageTicketItem;
typedef TicketItem<Media, MediaTraits> MediaTicketItem;
typedef TicketItem<int32_t, CopiesTraits> CopiesTicketItem;
typedef TicketItem<PageRange, PageRangeTraits> PageRangeTicketItem;
typedef TicketItem<bool, CollateTraits> CollateTicketItem;
typedef TicketItem<bool, ReverseTraits> ReverseTicketItem;

}  // namespace printer

}  // namespace cloud_devices

#endif  // COMPONENTS_CLOUD_DEVICES_COMMON_PRINTER_DESCRIPTION_H_
