// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @typedef {{min: number, max: number, count: number}} */
var Bucket;

/** @typedef {{name: string, buckets: !Array<Bucket>}} */
var Histogram;

var TaskSchedulerInternals = {
  /**
   * Updates the histograms on the page.
   * @param {!Array<!Histogram>} histograms Array of histogram objects.
   */
  updateHistograms: function(histograms) {
    var histogramContainer = $('histogram-container');
    for (var i in histograms) {
      var histogram = histograms[i];
      var title = document.createElement('div');
      title.textContent = histogram.name;
      histogramContainer.appendChild(title);
      if (histogram.buckets.length > 0) {
        histogramContainer.appendChild(
            TaskSchedulerInternals.createHistogramTable(histogram.buckets));
      } else {
        var unavailable = document.createElement('div');
        unavailable.textContent = 'No Data Recorded';
        histogramContainer.appendChild(unavailable);
      }
    }
  },

  /**
   * Returns a table representation of the histogram buckets.
   * @param {Object} buckets The histogram buckets.
   * @return {Object} A table element representation of the histogram buckets.
   */
  createHistogramTable: function(buckets) {
    var table = document.createElement('table');
    var headerRow = document.createElement('tr');
    var dataRow = document.createElement('tr');
    for (var i in buckets) {
      var bucket = buckets[i];
      var header = document.createElement('th');
      header.textContent = `${bucket.min}-${bucket.max}`;
      headerRow.appendChild(header);
      var data = document.createElement('td');
      data.textContent = bucket.count;
      dataRow.appendChild(data);
    }
    table.appendChild(headerRow);
    table.appendChild(dataRow);
    return table;
  },

  /**
   * Handles callback from onGetTaskSchedulerData.
   * @param {Object} data Dictionary containing all task scheduler metrics.
   */
  onGetTaskSchedulerData: function(data) {
    $('status').textContent =
        data.instantiated ? 'Instantiated' : 'Not Instantiated';
    $('details').hidden = !data.instantiated;
    if (!data.instantiated)
      return;

    TaskSchedulerInternals.updateHistograms(data.histograms);
  }
};

document.addEventListener('DOMContentLoaded', function() {
  chrome.send('getTaskSchedulerData');
});
