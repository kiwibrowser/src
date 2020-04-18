// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Constants use by chrome://sys-internals.
 */

cr.define('SysInternals', function() {
  /**
   * The page update period, in milliseconds.
   * @type {number}
   */
  const UPDATE_PERIOD = 1000;

  const /** !Array<string> */ UNITS_NUMBER_PER_SECOND = ['/s', 'K/s', 'M/s'];

  /** @const {number} */
  const /** number */ UNITBASE_NUMBER_PER_SECOND = 1000;

  const /** !Array<string> */ UNITS_MEMORY =
      ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];

  const /** number */ UNITBASE_MEMORY = 1024;

  /** @type {number} - The precision of the number on the info page. */
  const INFO_PAGE_PRECISION = 2;

  const /** !Array<string> */ CPU_COLOR_SET = [
    '#2fa2ff', '#ff93e2', '#a170d0', '#fe6c6c', '#2561a4', '#15b979', '#fda941',
    '#79dbcd'
  ];

  const /** !Array<string> */ MEMORY_COLOR_SET =
      ['#fa4e30', '#8d6668', '#73418c', '#41205e'];

  /** @type {!Array<string>} - Note: 4th and 5th colors use black menu text. */
  const ZRAM_COLOR_SET =
      ['#9cabd4', '#4a4392', '#dcfaff', '#fff9c9', '#ffa3ab'];

  /** @enum {string} */
  const PAGE_HASH = {
    INFO: '',
    CPU: '#CPU',
    MEMORY: '#Memory',
    ZRAM: '#Zram',
  };

  return {
    CPU_COLOR_SET: CPU_COLOR_SET,
    INFO_PAGE_PRECISION: INFO_PAGE_PRECISION,
    MEMORY_COLOR_SET: MEMORY_COLOR_SET,
    PAGE_HASH: PAGE_HASH,
    UNITBASE_MEMORY: UNITBASE_MEMORY,
    UNITBASE_NUMBER_PER_SECOND: UNITBASE_NUMBER_PER_SECOND,
    UNITS_MEMORY: UNITS_MEMORY,
    UNITS_NUMBER_PER_SECOND: UNITS_NUMBER_PER_SECOND,
    UPDATE_PERIOD: UPDATE_PERIOD,
    ZRAM_COLOR_SET: ZRAM_COLOR_SET,
  };
});
