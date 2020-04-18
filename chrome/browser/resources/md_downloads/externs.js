// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Externs for objects sent from C++ to JS for chrome://downloads.
 * @externs
 */

// eslint-disable-next-line no-var
var downloads = {};

/**
 * The type of the download object. The definition is based on
 * MdDownloadsDOMHandler::CreateDownloadItemValue()
 * @typedef {{by_ext_id: string,
 *            by_ext_name: string,
 *            danger_type: string,
 *            date_string: string,
 *            file_externally_removed: boolean,
 *            file_name: string,
 *            file_path: string,
 *            file_url: string,
 *            id: string,
 *            last_reason_text: string,
 *            otr: boolean,
 *            percent: number,
 *            progress_status_text: string,
 *            resume: boolean,
 *            retry: boolean,
 *            since_string: string,
 *            started: number,
 *            state: string,
 *            total: number,
 *            url: string}}
 */
downloads.Data;
