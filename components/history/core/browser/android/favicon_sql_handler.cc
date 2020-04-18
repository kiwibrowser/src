// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/android/favicon_sql_handler.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "components/history/core/browser/thumbnail_database.h"

using base::Time;

namespace history {

namespace {

// The interesting columns of this handler.
const HistoryAndBookmarkRow::ColumnID kInterestingColumns[] = {
  HistoryAndBookmarkRow::FAVICON};

}  // namespace

FaviconSQLHandler::FaviconSQLHandler(ThumbnailDatabase* thumbnail_db)
    : SQLHandler(kInterestingColumns, arraysize(kInterestingColumns)),
      thumbnail_db_(thumbnail_db) {
}

FaviconSQLHandler::~FaviconSQLHandler() {
}

bool FaviconSQLHandler::Update(const HistoryAndBookmarkRow& row,
                               const TableIDRows& ids_set) {
  if (!row.favicon_valid())
    return Delete(ids_set);

  // If the image_data will be updated, it is not reasonable to find if the
  // icon is already in database, just create a new favicon.
  // TODO(pkotwicz): Pass in real pixel size.
  favicon_base::FaviconID favicon_id = thumbnail_db_->AddFavicon(
      GURL(), favicon_base::IconType::kFavicon, row.favicon(),
      FaviconBitmapType::ON_VISIT, Time::Now(), gfx::Size());

  if (!favicon_id)
    return false;

  std::vector<favicon_base::FaviconID> favicon_ids;
  for (TableIDRows::const_iterator i = ids_set.begin();
       i != ids_set.end(); ++i) {
    // Remove all icon mappings to favicons of type kFavicon.
    std::vector<IconMapping> icon_mappings;
    thumbnail_db_->GetIconMappingsForPageURL(
        i->url, {favicon_base::IconType::kFavicon}, &icon_mappings);
    for (std::vector<IconMapping>::const_iterator m = icon_mappings.begin();
         m != icon_mappings.end(); ++m) {
      if (!thumbnail_db_->DeleteIconMapping(m->mapping_id))
        return false;

      // Keep the old icon for deleting it later if possible.
      favicon_ids.push_back(m->icon_id);
    }
    // Add the icon mapping.
    if (!thumbnail_db_->AddIconMapping(i->url, favicon_id))
      return false;
  }
  // As we update the favicon, Let's remove unused favicons if any.
  if (!favicon_ids.empty() && !DeleteUnusedFavicon(favicon_ids))
    return false;

  return true;
}

bool FaviconSQLHandler::Delete(const TableIDRows& ids_set) {
  std::vector<favicon_base::FaviconID> favicon_ids;
  for (TableIDRows::const_iterator i = ids_set.begin();
       i != ids_set.end(); ++i) {
    // Since the URL was deleted, we delete all types of icon mappings.
    std::vector<IconMapping> icon_mappings;
    thumbnail_db_->GetIconMappingsForPageURL(i->url, &icon_mappings);
    for (std::vector<IconMapping>::const_iterator m = icon_mappings.begin();
         m != icon_mappings.end(); ++m) {
      favicon_ids.push_back(m->icon_id);
    }
    if (!thumbnail_db_->DeleteIconMappings(i->url))
      return false;
  }

  if (favicon_ids.empty())
    return true;

  if (!DeleteUnusedFavicon(favicon_ids))
    return false;

  return true;
}

bool FaviconSQLHandler::Insert(HistoryAndBookmarkRow* row) {
  if (!row->is_value_set_explicitly(HistoryAndBookmarkRow::FAVICON) ||
      !row->favicon_valid())
    return true;

  DCHECK(row->is_value_set_explicitly(HistoryAndBookmarkRow::URL));

  // Is it a problem to give a empty URL?
  // TODO(pkotwicz): Pass in real pixel size.
  favicon_base::FaviconID id = thumbnail_db_->AddFavicon(
      GURL(), favicon_base::IconType::kFavicon, row->favicon(),
      FaviconBitmapType::ON_VISIT, Time::Now(), gfx::Size());
  if (!id)
    return false;
  return thumbnail_db_->AddIconMapping(row->url(), id);
}

bool FaviconSQLHandler::DeleteUnusedFavicon(
    const std::vector<favicon_base::FaviconID>& ids) {
  for (std::vector<favicon_base::FaviconID>::const_iterator i = ids.begin();
       i != ids.end();
       ++i) {
    if (!thumbnail_db_->HasMappingFor(*i) && !thumbnail_db_->DeleteFavicon(*i))
      return false;
  }
  return true;
}

}  // namespace history.
