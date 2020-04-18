// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('downloads', function() {
  /**
   * Explains why a download is in DANGEROUS state.
   * @enum {string}
   */
  const DangerType = {
    NOT_DANGEROUS: 'NOT_DANGEROUS',
    DANGEROUS_FILE: 'DANGEROUS_FILE',
    DANGEROUS_URL: 'DANGEROUS_URL',
    DANGEROUS_CONTENT: 'DANGEROUS_CONTENT',
    UNCOMMON_CONTENT: 'UNCOMMON_CONTENT',
    DANGEROUS_HOST: 'DANGEROUS_HOST',
    POTENTIALLY_UNWANTED: 'POTENTIALLY_UNWANTED',
  };

  /**
   * The states a download can be in. These correspond to states defined in
   * DownloadsDOMHandler::CreateDownloadItemValue
   * @enum {string}
   */
  const States = {
    IN_PROGRESS: 'IN_PROGRESS',
    CANCELLED: 'CANCELLED',
    COMPLETE: 'COMPLETE',
    PAUSED: 'PAUSED',
    DANGEROUS: 'DANGEROUS',
    INTERRUPTED: 'INTERRUPTED',
  };

  return {
    DangerType: DangerType,
    States: States,
  };
});
