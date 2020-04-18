// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

storage = new (function() {

this.table_ = null;
this.tableData_ = null;
this.profileMenu_ = null;

this.onDomReady_ = function() {
  // Create the menus for mmap and nheap profiling.
  this.profileMenu_ = $('#storage-profile-menu');
  this.profileMenu_
      .mouseleave(this.profileMenu_.hide.bind(
          this.profileMenu_, {duration: 0}))
      .hide();

  // Initialize the toolbar.
  $('#storage-profile').button({icons:{primary: 'ui-icon-image'}})
      .click(this.profileMmapForSelectedSnapshots.bind(this, null))
      .mouseenter(this.showProfileMenu_.bind(this));

  $('#storage-dump-mmaps').button({icons:{primary: 'ui-icon-calculator'}})
      .click(this.dumpMmapForSelectedSnapshot_.bind(this));
  $('#storage-dump-nheap').button({icons:{primary: 'ui-icon-calculator'}})
      .click(this.dumpNheapForSelectedSnapshot_.bind(this));
  if (window.DISABLE_NATIVE_TRACING) {
    $('#storage-dump-nheap').hide();
  }

  // Create the table.
  this.table_ = new google.visualization.Table($('#storage-table')[0]);
  $('#storage-table').on('contextmenu', this.showProfileMenu_.bind(this));
};

this.reload = function() {
  webservice.ajaxRequest('/storage/list', this.onListAjaxResponse_.bind(this));
}

this.onListAjaxResponse_ = function(data) {
  if (window.DISABLE_NATIVE_TRACING) {
    // Do not show the native heap column if native tracing is disabled.
    data.cols.pop();
  }
  this.tableData_ = new google.visualization.DataTable(data);
  this.redraw();
};

this.profileMmapForSelectedSnapshots = function(ruleset) {
  // Generates a mmap profile for the selected snapshots.
  var sel = this.table_.getSelection();
  if (!sel.length || !this.tableData_) {
    rootUi.showDialog('No snapshots selected!');
    return;
  }
  var archiveName = null;
  var snapshots = [];

  for (var i = 0; i < sel.length; ++i) {
    var row = sel[i].row;
    var curArchive = this.tableData_.getValue(row, 0);
    if (archiveName && curArchive != archiveName) {
      rootUi.showDialog(
          'All the selected snapshots must belong to the same archive!');
      return;
    }
    archiveName = curArchive;
    snapshots.push(this.tableData_.getValue(row, 1));
  }
  profiler.profileArchivedMmaps(archiveName, snapshots, ruleset);
  rootUi.showTab('prof');
};

this.dumpMmapForSelectedSnapshot_ = function() {
  var sel = this.table_.getSelection();
  if (sel.length != 1) {
    rootUi.showDialog('Please select only one snapshot.')
    return;
  }

  var row = sel[0].row;
  mmap.dumpMmapsFromStorage(this.tableData_.getValue(row, 0),
                            this.tableData_.getValue(row, 1))
  rootUi.showTab('mm');
};

this.dumpNheapForSelectedSnapshot_ = function() {
  var sel = this.table_.getSelection();
  if (sel.length != 1) {
    rootUi.showDialog('Please select only one snapshot.')
    return;
  }

  var row = sel[0].row;
  if (!this.checkHasNativeHapDump_(row))
    return;
  nheap.dumpNheapFromStorage(this.tableData_.getValue(row, 0),
                             this.tableData_.getValue(row, 1))
  rootUi.showTab('nheap');
};

this.profileNativeForSelectedSnapshots = function(ruleset) {
  // Generates a native heap profile for the selected snapshots.
  var sel = this.table_.getSelection();
  if (!sel.length || !this.tableData_) {
    rootUi.showDialog('No snapshots selected!');
    return;
  }
  var archiveName = null;
  var snapshots = [];

  for (var i = 0; i < sel.length; ++i) {
    var row = sel[i].row;
    var curArchive = this.tableData_.getValue(row, 0);
    if (archiveName && curArchive != archiveName) {
      rootUi.showDialog(
          'All the selected snapshots must belong to the same archive!');
      return;
    }
    if (!this.checkHasNativeHapDump_(row))
      return;
    archiveName = curArchive;
    snapshots.push(this.tableData_.getValue(row, 1));
  }
  profiler.profileArchivedNHeaps(archiveName, snapshots, ruleset);
  rootUi.showTab('prof');
};

this.checkHasNativeHapDump_ = function(row) {
  if (!this.tableData_.getValue(row, 3)) {
    rootUi.showDialog('The selected snapshot doesn\'t have a heap dump!');
    return false;
  }
  return true;
}

this.rebuildMenu_ = function() {
  this.profileMenu_.empty();

  this.profileMenu_.append(
      $('<li/>').addClass('header').text('Memory map rules'));
  profiler.rulesets['mmap'].forEach(function(rule) {
    this.profileMenu_.append(
        $('<li/>').text(rule).click(
            this.profileMmapForSelectedSnapshots.bind(this, rule)));
  }, this);

  if (!window.DISABLE_NATIVE_TRACING) {
    this.profileMenu_.append(
        $('<li/>').addClass('header').text('Native heap rules'));
    profiler.rulesets['nheap'].forEach(function(rule) {
      this.profileMenu_.append(
          $('<li/>').text(rule).click(
              this.profileNativeForSelectedSnapshots.bind(this, rule)));
    }, this);
  }

  this.profileMenu_.menu();
};

this.showProfileMenu_ = function(evt) {
  console.log(evt);
  var pos;
  if (evt.type == 'contextmenu')
    pos = {my: "left top", at: "left bottom", of: evt};
  else
    pos = {my: "left top", at: "left bottom", of: evt.target};
  this.profileMenu_.show({duration: 0}).position(pos);
  evt.preventDefault();
}

this.redraw = function() {
  this.rebuildMenu_();
  if (!this.tableData_)
    return;
  this.table_.draw(this.tableData_);
};

$(document).ready(this.onDomReady_.bind(this));

})();
