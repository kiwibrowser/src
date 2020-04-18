// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

nheap = new (function() {

this.COL_TOTAL = 0;
this.COL_RESIDENT = 1;
this.COL_STACKTRACE = 3;


this.nheapData_ = null;
this.nheapTable_ = null;
this.nheapFilter_ = null;

this.onDomReady_ = function() {
  // Create the mmaps table.
  this.nheapTable_ = new google.visualization.Table($('#nheap-table')[0]);
  google.visualization.events.addListener(
      this.nheapTable_, 'select', this.onNheapTableRowSelect_.bind(this));
  $('#nheap-filter').on('change', this.applyTableFilters_.bind(this));
};

this.dumpNheapFromStorage = function(archiveName, snapshot) {
  webservice.ajaxRequest('/storage/' + archiveName + '/' + snapshot + '/nheap',
                         this.onDumpAjaxResponse_.bind(this));
  rootUi.showDialog('Loading native heap allocs from archive ...');
  this.resetTableFilters_();
};

this.onDumpAjaxResponse_ = function(data) {
  this.nheapData_ = new google.visualization.DataTable(data);  // TODO remove .table form mmap
  this.nheapFilter_ = new google.visualization.DataView(this.nheapData_);
  this.applyTableFilters_();
  rootUi.hideDialog();
};

this.resetTableFilters_ = function() {
  $('#nheap-filter').val('');
}

this.applyTableFilters_ = function() {
  // Filters the rows according to the user-provided file and prot regexps.
  if (!this.nheapFilter_)
    return;

  var rx = $('#nheap-filter').val();
  var rows = [];
  var total_allocated = 0;
  var total_resident = 0;

  for (var row = 0; row < this.nheapData_.getNumberOfRows(); ++row) {
     stackTrace = this.nheapData_.getValue(row, this.COL_STACKTRACE);
     if (!stackTrace.match(rx))
      continue;
    rows.push(row);
    total_allocated += this.nheapData_.getValue(row, this.COL_TOTAL);
    total_resident += this.nheapData_.getValue(row, this.COL_RESIDENT);
  }

  $('#nheap-total-allocated').val(Math.floor(total_allocated / 1024) + ' KB');
  $('#nheap-total-resident').val(Math.floor(total_resident / 1024) + ' KB');
  this.nheapFilter_.setRows(rows);
  this.redraw();
};

this.onNheapTableRowSelect_ = function() {
  if (!this.nheapFilter_)
    return;

  var total_allocated = 0;
  var total_resident = 0;

  this.nheapTable_.getSelection().forEach(function(sel) {
    var row = sel.row;
    total_allocated += this.nheapFilter_.getValue(row, this.COL_TOTAL);
    total_resident += this.nheapFilter_.getValue(row, this.COL_RESIDENT);
  }, this);

  $('#nheap-selected-allocated').val(Math.floor(total_allocated / 1024) +
                                     ' KB');
  $('#nheap-selected-resident').val(Math.floor(total_resident / 1024) + ' KB');
};

this.redraw = function() {
  if (!this.nheapFilter_)
    return;
  this.nheapTable_.draw(this.nheapFilter_, {allowHtml: true,
                                            page: 'enable',
                                            pageSize: 25});
};

$(document).ready(this.onDomReady_.bind(this));

})();