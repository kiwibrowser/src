// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mmap = new (function() {

this.COL_START = 0;
this.COL_END = 1;
this.COL_LEN = 2;
this.COL_PROT = 3;
this.COL_RSS = 4;
this.COL_PRIV_DIRTY = 5;
this.COL_PRIV_CLEAN = 6;
this.COL_SHARED_DIRTY = 7;
this.COL_SHARED_CLEAN = 8;
this.COL_FILE = 9;
this.COL_OFFSET = 10;
this.COL_RESIDENT = 11;
this.SHOW_COLUMNS = [this.COL_START, this.COL_END, this.COL_LEN, this.COL_PROT,
    this.COL_RSS, this.COL_PRIV_DIRTY, this.COL_PRIV_CLEAN,
    this.COL_SHARED_DIRTY, this.COL_SHARED_CLEAN, this.COL_FILE];

this.PAGE_SIZE = 4096;

this.mapsData_ = null;
this.mapsTable_ = null;
this.mapsFilter_ = null;
this.shouldUpdateProfileAfterDump_ = false;

this.onDomReady_ = function() {
  $('#mm-lookup-addr').on('change', this.lookupAddress.bind(this));
  $('#mm-filter-file').on('change', this.applyMapsTableFilters_.bind(this));
  $('#mm-filter-prot').on('change', this.applyMapsTableFilters_.bind(this));
  $('#mm-filter-clear').on('click', this.resetMapsTableFilters_.bind(this));

  // Create the mmaps table.
  this.mapsTable_ = new google.visualization.Table($('#mm-table')[0]);
  google.visualization.events.addListener(
      this.mapsTable_, 'select', this.onMmapTableRowSelect_.bind(this));
  $('#mm-table').on('dblclick', this.onMmapTableDblClick_.bind(this));
};

this.dumpMmaps = function(targetProcUri, updateProfile) {
  if (!targetProcUri)
    return;
  this.shouldUpdateProfileAfterDump_ = !!updateProfile;
  webservice.ajaxRequest('/dump/mmap/' + targetProcUri,
                         this.onDumpAjaxResponse_.bind(this));
  rootUi.showDialog('Dumping memory maps for ' + targetProcUri + '...');
};

this.dumpMmapsFromStorage = function(archiveName, snapshot) {
  webservice.ajaxRequest('/storage/' + archiveName + '/' + snapshot + '/mmaps',
                         this.onDumpAjaxResponse_.bind(this));
  rootUi.showDialog('Loading memory maps from archive ...');
};

this.onDumpAjaxResponse_ = function(data) {
  $('#mm-filter-file').val('');
  $('#mm-filter-prot').val('');
  this.mapsData_ = new google.visualization.DataTable(data.table);
  this.mapsFilter_ = new google.visualization.DataView(this.mapsData_);
  this.mapsFilter_.setColumns(this.SHOW_COLUMNS);
  this.applyMapsTableFilters_();
  rootUi.hideDialog();
  if (this.shouldUpdateProfileAfterDump_)
    profiler.profileCachedMmapDump(data.id);
  shouldUpdateProfileAfterDump_ = false;
};

this.applyMapsTableFilters_ = function() {
  // Filters the rows according to the user-provided file and prot regexps.
  if (!this.mapsFilter_)
    return;

  var fileRx = $('#mm-filter-file').val();
  var protRx = $('#mm-filter-prot').val();
  var rows = [];
  var totPrivDirty = 0;
  var totPrivClean = 0;
  var totSharedDirty = 0;
  var totSharedClean = 0;

  for (var row = 0; row < this.mapsData_.getNumberOfRows(); ++row) {
     mappedFile = this.mapsData_.getValue(row, this.COL_FILE);
     protFlags = this.mapsData_.getValue(row, this.COL_PROT);
     if (!mappedFile.match(fileRx) || !protFlags.match(protRx))
      continue;
    rows.push(row);
    totPrivDirty += this.mapsData_.getValue(row, this.COL_PRIV_DIRTY);
    totPrivClean += this.mapsData_.getValue(row, this.COL_PRIV_CLEAN);
    totSharedDirty += this.mapsData_.getValue(row,this.COL_SHARED_DIRTY);
    totSharedClean += this.mapsData_.getValue(row, this.COL_SHARED_CLEAN);
  }

  this.mapsFilter_.setRows(rows);
  $('#mm-totals-priv-dirty').text(totPrivDirty);
  $('#mm-totals-priv-clean').text(totPrivClean);
  $('#mm-totals-shared-dirty').text(totSharedDirty);
  $('#mm-totals-shared-clean').text(totSharedClean);
  this.redraw();
};

this.resetMapsTableFilters_ = function() {
  $('#mm-filter-file').val('');
  $('#mm-filter-prot').val('');
  this.applyMapsTableFilters_();
};

this.onMmapTableRowSelect_ = function() {
  // Update the memory totals for the selected rows.
  if (!this.mapsFilter_)
    return;

  var totPrivDirty = 0;
  var totPrivClean = 0;
  var totSharedDirty = 0;
  var totSharedClean = 0;

  this.mapsTable_.getSelection().forEach(function(sel) {
    var row = sel.row;
    totPrivDirty += this.mapsFilter_.getValue(row, this.COL_PRIV_DIRTY);
    totPrivClean += this.mapsFilter_.getValue(row, this.COL_PRIV_CLEAN);
    totSharedDirty += this.mapsFilter_.getValue(row,this.COL_SHARED_DIRTY);
    totSharedClean += this.mapsFilter_.getValue(row, this.COL_SHARED_CLEAN);
  }, this);
  $('#mm-selected-priv-dirty').text(totPrivDirty);
  $('#mm-selected-priv-clean').text(totPrivClean);
  $('#mm-selected-shared-dirty').text(totSharedDirty);
  $('#mm-selected-shared-clean').text(totSharedClean);
};

this.onMmapTableDblClick_ = function() {
  // Show resident pages for the selected mapping.
  var PAGES_PER_ROW = 16;

  if (!this.mapsData_)
    return;

  var sel = this.mapsTable_.getSelection();
  if (sel.length == 0)
    return;

  // |sel| returns the row index in the current view, which might be filtered.
  // Need to walk back in the mapsFilter_.getViewRows to get the actual row
  // index in the original table.
  var row = this.mapsFilter_.getViewRows()[sel[0].row];
  var arr = JSON.parse(this.mapsData_.getValue(row, this.COL_RESIDENT));
  var table = $('<table class="mm-resident-table"/>');
  var curRow = $('<tr/>');
  table.append(curRow);

  for (var i = 0; i < arr.length; ++i) {
    for (var j = 0; j < 8; ++j) {
      var pageIdx = i * 8 + j;
      var resident = !!(arr[i] & (1 << j));
      if (pageIdx % PAGES_PER_ROW == 0) {
        curRow = $('<tr/>');
        table.append(curRow);
      }
      var hexAddr = (pageIdx * this.PAGE_SIZE).toString(16);
      var cell = $('<td/>').text(hexAddr);
      if (resident)
        cell.addClass('resident')
      curRow.append(cell);
    }
  }
  rootUi.showDialog(table, 'Resident page list');
};

this.redraw = function() {
  if (!this.mapsFilter_)
    return;
  this.mapsTable_.draw(this.mapsFilter_);
};

this.lookupAddress = function() {
  // Looks up the user-provided address in the mmap table and highlights the
  // row containing the map (if found).
  if (!this.mapsData_)
    return;

  addr = parseInt($('#mm-lookup-addr').val(), 16);
  $('#mm-lookup-offset').val('');
  if (!addr)
    return;

  this.resetMapsTableFilters_();

  var lbound = 0;
  var ubound = this.mapsData_.getNumberOfRows() - 1;
  while (lbound <= ubound) {
    var row = ((lbound + ubound) / 2) >> 0;
    var start = parseInt(this.mapsData_.getValue(row, this.COL_START), 16);
    var end = parseInt(this.mapsData_.getValue(row, this.COL_END), 16);
    if (addr < start){
      ubound = row - 1;
    }
    else if (addr > end) {
      lbound = row + 1;
    }
    else {
      $('#mm-lookup-offset').val((addr - start).toString(16));
      this.mapsTable_.setSelection([{row: row, column: null}]);
      // Scroll to row.
      $('#wrapper').scrollTop(
          $('#mm-table .google-visualization-table-tr-sel').offset().top);
      break;
    }
  }
};

$(document).ready(this.onDomReady_.bind(this));

})();