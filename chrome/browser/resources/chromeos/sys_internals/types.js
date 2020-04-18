// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Typedef use by chrome://sys-internals.
 */

/**
 * For info page.
 * @typedef {{
 *   core: number,
 *   idle: number,
 *   kernel: number,
 *   usage: number,
 *   user: number,
 * }}
 */
var GeneralCpuType;

/**
 * For info page.
 * @typedef {{
 *   swapTotal: number,
 *   swapUsed: number,
 *   total: number,
 *   used: number,
 * }}
 */
var GeneralMemoryType;

/**
 * For info page.
 * @typedef {{
 *   compr: number,
 *   comprRatio: number,
 *   orig: number,
 *   total: number,
 * }}
 */
var GeneralZramType;

/**
 * @typedef {{
 *   cpu: GeneralCpuType,
 *   memory: GeneralMemoryType,
 *   zram: GeneralZramType,
 * }}
 */
var GeneralInfoType;

/**
 * @typedef {Array<!LineChart.DataSeries>|null}
 */
var CpuDataSeriesSet;

/**
 * @typedef {{
 *   memUsed: !LineChart.DataSeries,
 *   swapUsed: !LineChart.DataSeries,
 *   pswpin: !LineChart.DataSeries,
 *   pswpout: !LineChart.DataSeries
 * }}
 */
var MemoryDataSeriesSet;

/**
 * @typedef {{
 *   origDataSize: !LineChart.DataSeries,
 *   comprDataSize: !LineChart.DataSeries,
 *   memUsedTotal: !LineChart.DataSeries,
 *   numReads: !LineChart.DataSeries,
 *   numWrites: !LineChart.DataSeries
 * }}
 */
var ZramDataSeriesSet;

/**
 * @typedef {{
 *   cpus: CpuDataSeriesSet,
 *   memory: MemoryDataSeriesSet,
 *   zram: ZramDataSeriesSet,
 * }}
 */
var DataSeriesSet;

/**
 * @typedef {{value: number, timestamp: number}}
 */
var CounterType;
