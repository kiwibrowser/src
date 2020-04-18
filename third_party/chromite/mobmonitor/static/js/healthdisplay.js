// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var RECORD_TTL_MS = 10000;


$.widget('mobmonitor.healthDisplay', {
  options: {},

  _create: function() {
    this.element.addClass('health-display');

    // Service status information. The variable serviceHealthStatusInfo
    // is a mapping of the following form:
    //
    //  {serviceName: {lastUpdatedTimestampInMs: lastUpdatedTimestampInMs,
    //                 serviceStatus: serviceStatus}}
    //
    //  Where serviceStatus objects are of the following form:
    //
    //  {serviceName: serviceNameString,
    //   health: boolean,
    //   healthchecks: [
    //      {name: healthCheckName, health: boolean,
    //       description: descriptionString,
    //       actions: [actionNameString]}
    //    ]
    //  }
    this.serviceHealthStatusInfo = {};
  },

  _destroy: function() {
    this.element.removeClass('health-display');
    this.element.empty();
  },

  _setOption: function(key, value) {
    this._super(key, value);
  },

  // Private widget methods.
  // TODO (msartori): Implement crbug.com/520746.
  _updateHealthDisplayServices: function(services) {
    var self = this;

    function _removeService(service) {
      // Remove the UI elements.
      var id = 'div[id=' + SERVICE_CONTAINER_PREFIX + service + ']';
      $(id).empty();
      $(id).remove();

      // Remove raw service status info that we are holding.
      delete self.serviceHealthStatusInfo[service];
    }

    function _addService(serviceStatus) {
      // This function is used as a callback to the rpcGetStatus.
      // rpcGetStatus returns a list of service health statuses.
      // In this widget, we add services one at a time, so take
      // the first element.
      serviceStatus = serviceStatus[0];

      // Create the new content for the healthDisplay widget.
      var templateData = jQuery.extend({}, serviceStatus);
      templateData.serviceId = SERVICE_CONTAINER_PREFIX + serviceStatus.service;
      templateData.errors = serviceStatus.healthchecks.filter(function(v) {
        return !v.health;
      });
      templateData.warnings = serviceStatus.healthchecks.filter(function(v) {
        return v.health;
      });

      var healthContainer = renderTemplate('healthstatuscontainer',
                                           templateData);

      // Insert the new container into the display widget.
      $(healthContainer).appendTo(self.element);

      // Maintain alphabetical order in our display.
      self.element.children().sort(function(a, b) {
          return $(a).attr('id') < $(b).attr('id') ? -1 : 1;
      }).appendTo(self.element);

      // Save information to do with this service.
      var curtime = $.now();
      var service = serviceStatus.service;

      self.serviceHealthStatusInfo[service] = {
          lastUpdatedTimestampInMs: curtime,
          serviceStatus: serviceStatus
      };
    }

    // Remove services that are no longer monitored or are stale.
    var now = $.now();

    Object.keys(this.serviceHealthStatusInfo).forEach(
        function(elem, index, array) {
          if ($.inArray(elem, services) < 0 ||
            now > self.serviceHealthStatusInfo[elem].lastUpdatedTimestampInMs +
              RECORD_TTL_MS) {
            _removeService(elem);
          }
    });

    // Get sublist of services to update.
    var updateList =
        $(services).not(Object.keys(this.serviceHealthStatusInfo)).get();

    // Update the services.
    updateList.forEach(function(elem, index, array) {
      rpcGetStatus(elem, _addService);
    });
  },

  // Public widget methods.
  refreshHealthDisplay: function() {
    var self = this;
    rpcGetServiceList(function(services) {
      self._updateHealthDisplayServices(services);
    });
  },

  needsRepair: function(service) {
    var serviceStatus = this.serviceHealthStatusInfo[service].serviceStatus;
    return serviceStatus.health == 'false' ||
        serviceStatus.healthchecks.length > 0;
  },

  markStale: function(service) {
    this.serviceHealthStatusInfo[service].lastUpdatedTimestampInMs = 0;
  },

  getServiceActions: function(service) {
    var actionSet = {};
    var healthchecks =
        this.serviceHealthStatusInfo[service].serviceStatus.healthchecks;

    for (var i = 0; i < healthchecks.length; i++) {
      for (var j = 0; j < healthchecks[i].actions.length; j++) {
        actionSet[healthchecks[i].actions[j]] = true;
      }
    }

    return Object.keys(actionSet);
  }
});
