// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

settings = new (function() {

this.onDomReady_ = function() {
  $('#settings-store').button({icons: {primary: 'ui-icon-disk'}})
      .click(this.store.bind(this));
};

this.reload = function() {
  $('#settings-container').empty();

  // Query the backend-specific settings (e.g., adb path).
  devices.getAllBackends().forEach(function(backend) {
    webservice.ajaxRequest(
        '/settings/' + backend,
        this.onGetSettingsAjaxResponse_.bind(
            this, 'backend ' + backend, backend));
  }, this);

  // Also query the device-specific settings (e.g., symbol path).
  devices.getAllDevices().forEach(function(device) {
    var deviceUri = device.backend + '/' + device.id;
    var deviceTitle = 'device ' + device.name + ' [' + device.id + ']';
    webservice.ajaxRequest(
        '/settings/' + deviceUri,
        this.onGetSettingsAjaxResponse_.bind(this, deviceTitle, deviceUri));
  }, this);
};

this.store = function() {
  var container = $('#settings-container');
  var targetsSettings = {};

  $('input[type="text"]', container).each(function(_, field) {
    field = $(field);
    var key = field.data('key');
    var target = field.data('target');
    var value = field.val();
    if (!(target in targetsSettings))
      targetsSettings[target] = {}
    targetsSettings[target][key] = value
  });

  for (var target in targetsSettings) {
    var targetsSetting = targetsSettings[target];
    webservice.ajaxRequest('/settings/' + target,
                           this.onSetSettingsAjaxResponse_.bind(this, target),
                           this.onSetSettingsAjaxError_.bind(this),
                           targetsSetting);
  }
};

this.onGetSettingsAjaxResponse_ = function(title, target, data) {
  var container = $('#settings-container');
  container.append($('<h2/>').text('Settings for ' + title));
  var table = $('<table/>');
  Object.keys(data).forEach(function(key) {
    var setting = data[key];
    var row = $('<tr/>');
    var textfield = $('<input type="text"/>');
    textfield.data('target', target);
    textfield.data('key', key);
    textfield.val(setting.value);
    row.append($('<td/>').text(setting.description));
    row.append($('<td/>').append(textfield));
    table.append(row);
  }, this);
  container.append(table);
};

this.onSetSettingsAjaxResponse_ = function(target, data) {
  var container = $('#settings-container');
  $('input[type="text"]', container).each(function(_, field) {
    field = $(field);
    if (field.data('target') == target)
      field.addClass('saved');
  });
};

this.onSetSettingsAjaxError_ = function(httpStatus, errorMsg) {
  rootUi.showDialog(
      'Error ' + httpStatus + ' while storing settings: ' + errorMsg);
};

$(document).ready(this.onDomReady_.bind(this));

})();