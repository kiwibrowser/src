// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.require('mr.RouteId');

describe('Tests RouteId', function() {
  it('Test getRouteId', function() {
    expect(mr.RouteId.getRouteId('123', 'cast', 'sink1', 'some-source'))
        .toEqual(mr.RouteId.PREFIX_ + '123/cast-sink1/some-source');
  });

  it('Test getRouteId in 2UA mode', function() {
    expect(mr.RouteId.getRouteId('123', 'cast', 'sink1', 'http://mysite.com'))
        .toEqual('123');
  });

  it('Test getRouteId with valid input', function() {
    let routeId = mr.RouteId.PREFIX_ + '123/cast-sink1/some-source';
    let obj = mr.RouteId.create(routeId);
    expect(obj.getRouteId()).toEqual(routeId);
    expect(obj.getPresentationId()).toEqual('123');
    expect(obj.getProviderName()).toEqual('cast');
    expect(obj.getSinkId()).toEqual('sink1');
    expect(obj.getSource()).toEqual('some-source');

    routeId = mr.RouteId.PREFIX_ + '/cast-sink1/some-source';
    obj = mr.RouteId.create(routeId);
    expect(obj.getRouteId()).toEqual(routeId);
    expect(obj.getPresentationId()).toEqual('');
    expect(obj.getProviderName()).toEqual('cast');
    expect(obj.getSinkId()).toEqual('sink1');
    expect(obj.getSource()).toEqual('some-source');
  });

  it('Test getRouteId with invalid input 1', function() {
    expect(mr.RouteId.create('123')).toBe(null);
    expect(mr.RouteId.create(mr.RouteId.PREFIX_ + '123/cast-sink1')).toBe(null);
    expect(mr.RouteId.create(mr.RouteId.PREFIX_ + '123/cast-')).toBe(null);
  });
});
