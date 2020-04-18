// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('downloadInternalsVisuals', function() {
  'use strict';

  function getOngoingServiceEntryClass(entry) {
    switch (entry.state) {
      case ServiceEntryState.NEW:
        return 'service-entry-new';
      case ServiceEntryState.AVAILABLE:
        return 'service-entry-available';
      case ServiceEntryState.ACTIVE:
        if (entry.driver == undefined || !entry.driver.paused ||
            entry.driver.state == DriverEntryState.INTERRUPTED)
          return 'service-entry-active';
        else
          return 'service-entry-blocked';
      case ServiceEntryState.PAUSED:
        return 'service-entry-paused';
      case ServiceEntryState.COMPLETE:
        return 'service-entry-success';
      default:
        return '';
    }
  }

  function getFinishedServiceEntryClass(entry) {
    switch (entry.result) {
      case ServiceEntryResult.SUCCEED:
        return 'service-entry-success';
      default:
        return 'service-entry-fail';
    }
  }

  function getServiceRequestClass(request) {
    switch (request.result) {
      case ServiceRequestResult.ACCEPTED:
        return 'service-entry-success';
      case ServiceRequestResult.BACKOFF:
      case ServiceRequestResult.CLIENT_CANCELLED:
        return 'service-entry-blocked';
      default:
        return 'service-entry-fail';
    }
  }

  return {
    getOngoingServiceEntryClass: getOngoingServiceEntryClass,
    getFinishedServiceEntryClass: getFinishedServiceEntryClass,
    getServiceRequestClass: getServiceRequestClass
  };
});