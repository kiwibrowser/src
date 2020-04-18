// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module handles the UI for the profiling. A memory profile is obtained
// asking the webservice to profile some data sources (through /profile/create)
// as, for instance, some mmaps dumps or some heap traces.
// Regardless of the data source, a profile consists of three main concepts:
// 1. A tree of buckets, each one having a name and a set of 1+ values (see
//    /classification/rules.py,results.py), one value per metric (see below).
// 2. A set of snapshots, identifying the times when the dump was taken. All the
//    snapshots have the same shape and the same nodes (but their values can
//    obviously differ).
// 3. A set of metrics, identifying the cardinality (how many) and the semantic
//    (what do they mean) of the values of the nodes in the results tree.
//
// From a graphical viewpoint a profile is displayed using two charts:
// - A tree (organizational) chart which shows, for a given snapshot and metric,
//   the taxonomy of the buckets and their corresponding value.
// - A time series (scattered area) chart which shows, for a given metric and a
//   given bucket, its evolution over time (and of its direct children).

profiler = new (function() {

this.rulesets = {'nheap': [], 'mmap': []};
this.treeData_ = null;
this.treeChart_ = null;
this.timeSeriesData_ = null;
this.timeSeriesChart_ = null;
this.isRedrawing_ = false;
this.profileId_ = null;  // The profile id retrieved on /ajax/profile/create.
this.times_ = [];  // Snapshot times: [0,4,8] -> 3 snapshots x 4 sec.
this.metrics_ = [];  // Keys in the result tree, e.g., ['RSS', 'PSS'].
this.curTime_ = null;  // Time of the snapshot currently displayed.
this.curMetric_ = null;  // Index (rel. to |metrics_|) currently displayed.
this.curBucket_ = null;  // Index (rel. to the tree) currently displayed.

this.onDomReady_ = function() {
  this.treeChart_ = new google.visualization.OrgChart($('#prof-tree_chart')[0]);
  this.timeSeriesChart_ = new google.visualization.SteppedAreaChart(
      $('#prof-time_chart')[0]);

  // Setup the UI event listeners to trigger the onUiParamsChange_ event.
  google.visualization.events.addListener(this.treeChart_, 'select',
                                          this.onUiParamsChange_.bind(this));
  $('#prof-metric').on('change', this.onUiParamsChange_.bind(this));
  $('#prof-time').slider({range: 'max', min: 0, max: 0, value: 0,
                          change: this.onUiParamsChange_.bind(this)});

  // Load the available profiler rules.
  webservice.ajaxRequest('/profile/rules',
                         this.OnRulesAjaxResponse_.bind(this));
};

this.profileCachedMmapDump = function(mmapDumpId, ruleset) {
  // Creates a profile using the data grabbed during a recent mmap dump.
  // This is used to get a quick overview (only one snapshot), of the memory
  // without doing a full periodic trace first.
  ruleset = ruleset || this.rulesets['mmap'][0];
  webservice.ajaxRequest('/profile/create',  // This is a POST request.
                         this.onProfileAjaxResponse_.bind(this, ruleset),
                         null,  // use the default error handler.
                         {type: 'mmap',
                          source: 'cache',
                          id: mmapDumpId,
                          ruleset: ruleset});
};

this.profileArchivedMmaps = function(archiveName, snapshots, ruleset) {
  ruleset = ruleset || this.rulesets['mmap'][0];
  // Creates a mmap profile using the data from the storage.
  webservice.ajaxRequest('/profile/create',  // This is a POST request.
                         this.onProfileAjaxResponse_.bind(this, ruleset),
                         null,  // use the default error handler.
                         {type: 'mmap',
                          source: 'archive',
                          archive: archiveName,
                          snapshots: snapshots,
                          ruleset: ruleset});
};

this.profileArchivedNHeaps = function(archiveName, snapshots, ruleset) {
  // Creates a native-heap profile using the data from the storage.
  ruleset = ruleset || this.rulesets['nheap'][0];
  webservice.ajaxRequest('/profile/create',  // This is a POST request.
                         this.onProfileAjaxResponse_.bind(this, ruleset),
                         null,  // use the default error handler.
                         {type: 'nheap',
                          source: 'archive',
                          archive: archiveName,
                          snapshots: snapshots,
                          ruleset: ruleset});
};

this.OnRulesAjaxResponse_ = function(data) {
  // This AJAX response contains essentially the directory listing of the
  // memory_inspector/classification_rules/ folder.
  console.assert('nheap' in data && 'mmap' in data);
  this.rulesets = data;
};

this.onProfileAjaxResponse_ = function(ruleset, data) {
  // This AJAX response contains a summary of the profile requested via the
  // /profile endpoint, which consists of:
  // - The number of snapshots (and their corresponding time) in an array.
  //   e.g., [0, 3 ,6] indicates that the profile contains three snapshots taken
  //   respectively at T=0, T=3 and T=6 sec.
  // - A list of profile metrics, e.g., ['RSS', 'P. Dirty'] indicates that every
  //   node in the result tree is a 2-tuple.
  // After this response, the concrete data for the charts can be fetched using
  // the /ajax/profile/{ID}/tree and /ajax/profile/{ID}/time_serie endpoints.
  this.profileId_ = data.id;
  this.times_ = data.times;  // An array of integers.
  this.metrics_ = data.metrics;  // An array of strings.
  this.curBucket_ = data.rootBucket;  // URI of the bucket, e.g., Total/Libs/.
  this.curTime_ = data.times[0];
  this.curMetric_ = 0;

  // Populate the rules label with the ruleset used for generating this profile.
  $('#prof-ruleset').text(ruleset);

  // Populate the "metrics" select box.
  $('#prof-metric').empty();
  this.metrics_.forEach(function(metric) {
    $('#prof-metric').append($('<option/>').text(metric));
  }, this);

  // Setup the bounds of the snapshots slider.
  $('#prof-time').slider('option', 'max', this.times_.length - 1);

  // Fetch the actual chart data (via /profile/{ID}/...) and redraw the charts.
  this.updateCharts();
};

this.onUiParamsChange_ = function() {
  // Triggered whenever any of the UI params (the metric select, the snapshot
  // slider or the selected bucket in the tree) changes.
  this.curMetric_ = $('#prof-metric').prop('selectedIndex');
  this.curTime_ = this.times_[$('#prof-time').slider('value')];
  $('#prof-time_label').text(this.curTime_);
  var selBucket = this.treeChart_.getSelection();
  if (selBucket.length)
    this.curBucket_ = this.treeData_.getValue(selBucket[0].row, 0);
  this.updateCharts();
};

this.updateCharts = function() {
  if (!this.profileId_)
    return;

  var profileUri = '/profile/' + this.profileId_;
  webservice.ajaxRequest(
      profileUri +'/tree/' + this.curMetric_ + '/' + this.curTime_,
      this.onTreeAjaxResponse_.bind(this));
  webservice.ajaxRequest(
      profileUri +'/time_serie/' + this.curMetric_ + '/' + this.curBucket_,
      this.onTimeSerieAjaxResponse_.bind(this));
};

this.onTreeAjaxResponse_ = function(data) {
  this.treeData_ = new google.visualization.DataTable(data);
  this.redrawTree_();
};

this.onTimeSerieAjaxResponse_ = function(data) {
  this.timeSeriesData_ = new google.visualization.DataTable(data);
  this.redrawTimeSerie_();
};

this.redrawTree_ = function() {
  // isRedrawing_ is used here to break the avalanche chain that would be caused
  // by redraw changing the node selection, triggering in turn another redraw.
  if (!this.treeData_ || this.isRedrawing_)
    return;

  this.isRedrawing_ = true;
  var savedSelection = this.treeChart_.getSelection();
  this.treeChart_.draw(this.treeData_, {allowHtml: true});

  // "If we want things to stay as they are, things will have to change."
  // (work around GChart bug, as if we didn't have enough problems on our own).
  this.treeChart_.setSelection([{row: null, column: null}]);
  this.treeChart_.setSelection(savedSelection);
  this.isRedrawing_ = false;
};

this.redrawTimeSerie_ = function() {
  if (!this.timeSeriesData_)
    return;

  var metric = this.metrics_[this.curMetric_];
  this.timeSeriesChart_.draw(this.timeSeriesData_, {
      title: metric + ' over time for ' + this.curBucket_,
      isStacked: true,
      hAxis: {title: 'Time [sec.]'},
      vAxis: {title: this.metrics_[this.curMetric_] + ' [KB]'}});
};

this.redraw = function() {
  this.redrawTree_();
  this.redrawTimeSerie_();
};

$(document).ready(this.onDomReady_.bind(this));

})();