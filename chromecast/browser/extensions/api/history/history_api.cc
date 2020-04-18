// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "chromecast/browser/extensions/api/history/history_api.h"

namespace extensions {
namespace cast {
namespace api {

using HistoryItemList = std::vector<api::history::HistoryItem>;
using VisitItemList = std::vector<api::history::VisitItem>;

ExtensionFunction::ResponseAction HistoryGetVisitsFunction::Run() {
  return RespondNow(
      ArgumentList(history::GetVisits::Results::Create(VisitItemList())));
}

ExtensionFunction::ResponseAction HistorySearchFunction::Run() {
  return RespondNow(
      ArgumentList(history::Search::Results::Create(HistoryItemList())));
}

ExtensionFunction::ResponseAction HistoryAddUrlFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction HistoryDeleteUrlFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction HistoryDeleteRangeFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction HistoryDeleteAllFunction::Run() {
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace cast
}  // namespace extensions
