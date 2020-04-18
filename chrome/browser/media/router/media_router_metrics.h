// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_METRICS_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_METRICS_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "chrome/browser/media/router/discovery/dial/safe_dial_device_description_parser.h"
#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "media/base/container_names.h"

class GURL;

namespace media_router {

enum class SinkIconType;

// NOTE: Do not renumber enums as that would confuse interpretation of
// previously logged data. When making changes, also update the enum list
// in tools/metrics/histograms/histograms.xml to keep it in sync.

// NOTE: For metrics specific to the Media Router component extension, see
// mojo/media_router_mojo_metrics.h.

// Where the user clicked to open the Media Router dialog.
enum class MediaRouterDialogOpenOrigin {
  TOOLBAR = 0,
  OVERFLOW_MENU = 1,
  CONTEXTUAL_MENU = 2,
  PAGE = 3,

  // NOTE: Add entries only immediately above this line.
  TOTAL_COUNT = 4
};

// The possible outcomes from a route creation response.
enum class MediaRouterRouteCreationOutcome {
  SUCCESS = 0,
  FAILURE_NO_ROUTE = 1,
  FAILURE_INVALID_SINK = 2,

  // Note: Add entries only immediately above this line.
  TOTAL_COUNT = 3,
};

// The possible actions a user can take while interacting with the Media Router
// dialog.
enum class MediaRouterUserAction {
  CHANGE_MODE = 0,
  START_LOCAL = 1,
  STOP_LOCAL = 2,
  CLOSE = 3,
  STATUS_REMOTE = 4,
  REPLACE_LOCAL_ROUTE = 5,

  // Note: Add entries only immediately above this line.
  TOTAL_COUNT = 6
};

enum class PresentationUrlType {
  kOther,
  kCast,            // cast:
  kCastDial,        // cast-dial:
  kCastLegacy,      // URLs that start with |kLegacyCastPresentationUrlPrefix|.
  kDial,            // dial:
  kHttp,            // http:
  kHttps,           // https:
  kRemotePlayback,  // remote-playback:
  // Add new types only immediately above this line. Remember to also update
  // tools/metrics/histograms/enums.xml.
  kPresentationUrlTypeCount
};

class MediaRouterMetrics {
 public:
  MediaRouterMetrics();
  ~MediaRouterMetrics();

  // UMA histogram names.
  static const char kHistogramDialParsingError[];
  static const char kHistogramIconClickLocation[];
  static const char kHistogramMediaRouterCastingSource[];
  static const char kHistogramMediaRouterFileFormat[];
  static const char kHistogramMediaRouterFileSize[];
  static const char kHistogramMediaSinkType[];
  static const char kHistogramPresentationUrlType[];
  static const char kHistogramRouteCreationOutcome[];
  static const char kHistogramUiDialogPaint[];
  static const char kHistogramUiDialogLoadedWithData[];
  static const char kHistogramUiFirstAction[];

  // Records where the user clicked to open the Media Router dialog.
  static void RecordMediaRouterDialogOrigin(
      MediaRouterDialogOpenOrigin origin);

  // Records the duration it takes for the Media Router dialog to open and
  // finish painting after a user clicks to open the dialog.
  static void RecordMediaRouterDialogPaint(
      const base::TimeDelta delta);

  // Records the duration it takes for the Media Router dialog to load its
  // initial data after a user clicks to open the dialog.
  static void RecordMediaRouterDialogLoaded(
      const base::TimeDelta delta);

  // Records the first action the user took after the Media Router dialog
  // opened.
  static void RecordMediaRouterInitialUserAction(
      MediaRouterUserAction action);

  // Records the outcome in a create route response.
  static void RecordRouteCreationOutcome(
      MediaRouterRouteCreationOutcome outcome);

  // Records casting source.
  static void RecordMediaRouterCastingSource(MediaCastMode source);

  // Records the format of a cast file.
  static void RecordMediaRouterFileFormat(
      media::container_names::MediaContainerName format);

  // Records the size of a cast file.
  static void RecordMediaRouterFileSize(int64_t size);

  // Records why DIAL device description resolution failed.
  static void RecordDialParsingError(
      SafeDialDeviceDescriptionParser::ParsingError parsing_error);

  // Records the type of Presentation URL used by a web page.
  static void RecordPresentationUrlType(const GURL& url);

  // Records the type of the sink that media is being Cast to.
  static void RecordMediaSinkType(SinkIconType sink_icon_type);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_METRICS_H_
