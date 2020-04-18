// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';


function rpcGetServiceList(callback) {
  $.getJSON('/GetServiceList', callback);
}

function rpcGetStatus(service, callback) {
  $.getJSON('/GetStatus', {service: service}, callback);
}

function rpcActionInfo(service, healthcheck, action, callback) {
  var data = {
    service: service,
    healthcheck: healthcheck,
    action: action
  };

  $.getJSON('/ActionInfo', data, callback);
}

function rpcRepairService(service, healthcheck, action,
                          args, kwargs, callback) {

  if (isEmpty(service))
    throw new InvalidRpcArgumentError(
        'Must specify service in RepairService RPC');

  if (isEmpty(healthcheck))
    throw new InvalidRpcArgumentError(
        'Must specify healthcheck in RepairService RPC');

  if (isEmpty(action))
    throw new InvalidRpcArgumentError(
        'Must specify action in RepairService RPC');

  var data = {
    service: service,
    healthcheck: healthcheck,
    action: action,
    args: JSON.stringify(args),
    kwargs: JSON.stringify(kwargs)
  };

  $.post('/RepairService', data, callback, 'json');
}
