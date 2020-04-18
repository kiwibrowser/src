// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';


var HEALTH_DISPLAY_REFRESH_MS = 1000;


$(document).ready(function() {
  // Setup the health status widget.
  $('#healthStatusDisplay').healthDisplay();
  $('#healthStatusDisplay').healthDisplay('refreshHealthDisplay');

  setInterval(function() {
    $('#healthStatusDisplay').healthDisplay('refreshHealthDisplay');
  }, HEALTH_DISPLAY_REFRESH_MS);

  // Setup the log collection button.
  $(document).on('click', '.collect-logs', function() {
    window.open('/CollectLogs', '_blank');
  });

  // Setup the repair action buttons
  $(document).on('click', '.run-repair-action', function() {
    // Retrieve the service and action for this repair button.
    var action = $(this).attr('action');
    var healthcheck = $(this).closest('.healthcheck-info').attr('hcname');
    var service = $(this).closest('.health-container').attr('id');
    if (service.indexOf(SERVICE_CONTAINER_PREFIX) === 0) {
      service = service.replace(SERVICE_CONTAINER_PREFIX, '');
    }

    // Do not launch dialog if this service does not need repair.
    if (!$('#healthStatusDisplay').healthDisplay('needsRepair', service)) {
      return;
    }

    function repairServiceCallback(response) {
      $('#healthStatusDisplay').healthDisplay('markStale', response.service);
    }

    rpcActionInfo(service, healthcheck, action, function(response) {
      if (isEmpty(response.args) && isEmpty(response.kwargs)) {
        rpcRepairService(service, healthcheck, action,
                         [], {}, repairServiceCallback);
        return;
      }

      var dialog = new ActionRepairDialog(service, response);
      dialog.submitHandler = function(service, action, args, kwargs) {
        rpcRepairService(service, healthcheck, action,
                         args, kwargs, repairServiceCallback);
      };
      dialog.open();
    });
  });
});
