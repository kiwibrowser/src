// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Changes to analytics reporting structures can have disruptive effects on the
// analytics history of the Files app (e.g. making it hard or impossible to
// detect trending).
//
// In general, treat changes to analytics like histogram changes, i.e. make
// additive changes, don't remove or rename existing Dimensions, Events, Labels,
// etc.
//
// Changes to this file will need to be reviewed by someone familiar with the
// analytics system.

// namespace
var metrics = metrics || metricsBase;

/** @enum {string} */
metrics.Categories = {
  ACQUISITION: 'Acquisition',
  MANAGEMENT: 'Management',
  INTERNALS: 'Internals'
};

/**
 * The values of these enums come from the analytics console.
 * @private @enum {number}
 */
metrics.Dimension_ = {
  CONSUMER_TYPE: 1,
  SESSION_TYPE: 2,
  MACHINE_USE: 3
};

/**
 * Enumeration of known FSPs used to qualify "providers"
 * "screens" on analytics. All FSPs NOT present in this list
 * will be reported to analytics as 'provided-unknown'.
 *
 * NOTE: When an unknown provider is encountered, a separate event will be
 * sent to analytics with the id. Consulation of that event will provided
 * an indication when a provider is popular enough to be added to the
 * whitelist.
 *
 * These look like extension ids, but are actually provider ids which may
 * but don't have to be extension ids.
 *
 * @enum {string}
 */
metrics.FileSystemProviders = {
  oedeeodfidgoollimchfdnbmhcpnklnd: 'ZipUnpacker',
  hlffpaajmfllggclnjppbblobdhokjhe: 'File System for Dropbox (YT)',
  jbfdfcehgafdbfpniaimfbfomafoadgo: 'File System for OneDrive (YT)',
  gbheifiifcfekkamhepkeogobihicgmn: 'SFTP File System (YT)',
  dikonaebkejmpbpcnnmfaeopkaenicgf: 'Box for Chrome OS',
  iibcngmpkgghccnakicfmgajlkhnohep: 'TED Talks (FB)',
  hmckflbfniicjijmdoffagjkpnjgbieh: 'WebDAV File System (YT)',
  ibfbhbegfkamboeglpnianlggahglbfi: 'Cloud Storage (FB)',
  pmnllmkmjilbojkpgplbdmckghmaocjh: 'Scan (FB)',
  mfhnnfciefdpolbelmfkpmhhmlkehbdf: 'File System for SMB/CIFS (YT)',
  plmanjiaoflhcilcfdnjeffklbgejmje: 'Add MY Documents (KA)',
  mljpablpddhocfbnokacjggdbmafjnon: 'Wicked Good Unarchiver (MF)',
  ndjpildffkeodjdaeebdhnncfhopkajk: 'Network File Share for Chrome OS',
  gmhmnhjihabohahcllfgjooaoecglhpi: 'LanFolder',
  dmboannefpncccogfdikhmhpmdnddgoe: 'ZipArchiver',

  /**
   * Native Providers.
   */
  '@smb': 'Native Network File Share (SMB)',
};

/**
 * Returns a new "screen" name for a provided file system type. Returns
 * 'unknown' for unknown providers.
 * @param {string|undefined} providerId The FSP provider ID.
 * @return {string} Name or 'unknown' if extension is unrecognized.
 */
metrics.getFileSystemProviderName = function(providerId) {
  return metrics.FileSystemProviders[providerId] || 'unknown';
};

/**
 * @enum {!analytics.EventBuilder.Dimension}
 */
metrics.Dimensions = {
  CONSUMER_TYPE_MANAGER: {
    index: metrics.Dimension_.CONSUMER_TYPE,
    value: 'Manage'
  },
  CONSUMER_TYPE_IMPORTER: {
    index: metrics.Dimension_.CONSUMER_TYPE,
    value: 'Import'
  },
  SESSION_TYPE_MANAGE: {
    index: metrics.Dimension_.SESSION_TYPE,
    value: 'Manage'
  },
  SESSION_TYPE_IMPORT: {
    index: metrics.Dimension_.SESSION_TYPE,
    value: 'Import'
  },
  MACHINE_USE_SINGLE: {
    index: metrics.Dimension_.MACHINE_USE,
    value: 'Single'
  },
  MACHINE_USE_MULTIPLE: {
    index: metrics.Dimension_.MACHINE_USE,
    value: 'Multiple'
  }
};

// namespace
metrics.event = metrics.event || {};

/**
 * Base event builders for files app.
 * @private @enum {!analytics.EventBuilder}
 */
metrics.event.Builders_ = {
  IMPORT: analytics.EventBuilder.builder()
      .category(metrics.Categories.ACQUISITION),
  INTERNALS: analytics.EventBuilder.builder()
      .category(metrics.Categories.INTERNALS),
  MANAGE: analytics.EventBuilder.builder()
      .category(metrics.Categories.MANAGEMENT)
};

/** @enum {!analytics.EventBuilder} */
metrics.Management = {
  WINDOW_CREATED: metrics.event.Builders_.MANAGE
      .action('Window Created')
      .dimension(metrics.Dimensions.SESSION_TYPE_MANAGE)
      .dimension(metrics.Dimensions.CONSUMER_TYPE_MANAGER)
};

/** @enum {!analytics.EventBuilder} */
metrics.ImportEvents = {
  DEVICE_YANKED: metrics.event.Builders_.IMPORT
      .action('Device Yanked'),

  ERRORS: metrics.event.Builders_.IMPORT
      .action('Import Error Count'),

  FILES_DEDUPLICATED: metrics.event.Builders_.IMPORT
      .action('Files Deduplicated'),

  FILES_IMPORTED: metrics.event.Builders_.IMPORT
      .action('Files Imported'),

  HISTORY_LOADED: metrics.event.Builders_.IMPORT
      .action('History Loaded'),

  IMPORT_CANCELLED: metrics.event.Builders_.IMPORT
      .action('Import Cancelled'),

  MEGABYTES_IMPORTED: metrics.event.Builders_.IMPORT
      .action('Megabytes Imported'),

  STARTED: metrics.event.Builders_.IMPORT
      .action('Import Started')
      .dimension(metrics.Dimensions.SESSION_TYPE_IMPORT)
      .dimension(metrics.Dimensions.CONSUMER_TYPE_IMPORTER)
};

/** @enum {!analytics.EventBuilder} */
metrics.Internals = {
  UNRECOGNIZED_FILE_SYSTEM_PROVIDER: metrics.event.Builders_.INTERNALS
      .action('Unrecognized File System Provider')
};

// namespace
metrics.timing = metrics.timing || {};

/** @enum {string} */
metrics.timing.Variables = {
  COMPUTE_HASH: 'Compute Content Hash',
  SEARCH_BY_HASH: 'Search By Hash',
  HISTORY_LOAD: 'History Load',
  EXTRACT_THUMBNAIL_FROM_RAW: 'Extract Thumbnail From RAW'
};
