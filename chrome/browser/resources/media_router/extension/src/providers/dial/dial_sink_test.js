// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.SinkTest');
goog.setTestOnly('mr.dial.SinkTest');

const DialSink = goog.require('mr.dial.Sink');
const MockClock = goog.require('mr.MockClock');
const Sink = goog.require('mr.Sink');
const SinkAppStatus = goog.require('mr.dial.SinkAppStatus');

describe('DIAL Sink Tests', function() {
  let mockClock;

  beforeEach(function() {
    mockClock = new MockClock(true);
  });

  afterEach(function() {
    mockClock.uninstall();
  });

  it('Gets and sets fields', function() {
    const sink = new DialSink('name', 'id1');
    expect(sink.getMrSink()).toEqual(new Sink('id1', 'name'));

    expect(sink.getId()).toEqual('id1');
    sink.setId('id2');
    expect(sink.getId()).toEqual('id2');

    expect(sink.getIpAddress()).toBeNull();
    sink.setIpAddress('192.168.111.1');
    expect(sink.getIpAddress()).toEqual('192.168.111.1');

    expect(sink.getDialAppUrl()).toBeNull();
    sink.setDialAppUrl('http://192.168.111.1/apps');
    expect(sink.getDialAppUrl()).toEqual('http://192.168.111.1/apps');

    expect(sink.getDeviceDescriptionUrl()).toBeNull();
    sink.setDeviceDescriptionUrl('http://192.168.111.1/desc');
    expect(sink.getDeviceDescriptionUrl()).toEqual('http://192.168.111.1/desc');

    expect(sink.getModelName()).toBeNull();
    sink.setModelName('chromecast');
    expect(sink.getModelName()).toEqual('chromecast');

    expect(sink.getFriendlyName()).toEqual('name');
    sink.setFriendlyName('newname');
    expect(sink.getFriendlyName()).toEqual('newname');

    expect(sink.getPort()).toEqual(null);
    sink.setPort(8009);
    expect(sink.getPort()).toEqual(8009);
  });

  it('Gets and sets sink app status', function() {
    const sink = new DialSink('name', 'uniqueId');
    expect(sink.getAppStatus('youtube')).toBe(SinkAppStatus.UNKNOWN);
    expect(sink.getAppStatusTimeStamp('youtube')).toBe(null);

    mockClock.tick(10);
    const now1 = Date.now();
    sink.setAppStatus('youtube', SinkAppStatus.AVAILABLE);
    mockClock.tick(10);
    const now2 = Date.now();
    sink.setAppStatus('app2', SinkAppStatus.UNAVAILABLE);
    expect(sink.getAppStatus('youtube')).toBe(SinkAppStatus.AVAILABLE);
    expect(sink.getAppStatusTimeStamp('youtube')).toBe(now1);
    expect(sink.getAppStatus('app2')).toBe(SinkAppStatus.UNAVAILABLE);
    expect(sink.getAppStatusTimeStamp('app2')).toBe(now2);

    sink.clearAppStatus();
    expect(sink.getAppStatus('youtube')).toBe(SinkAppStatus.UNKNOWN);
    expect(sink.getAppStatusTimeStamp('youtube')).toBe(null);
    expect(sink.getAppStatus('app2')).toBe(SinkAppStatus.UNKNOWN);
    expect(sink.getAppStatusTimeStamp('app2')).toBe(null);
  });

  it('Updates sink from another sink', function() {
    const sink = new DialSink('name', 'uniqueId')
                     .setDialAppUrl('http://192.168.111.1/apps')
                     .setPort(8009)
                     .setDeviceDescriptionUrl('http://192.168.111.1/desc');

    let updatedSink = new DialSink('name2', 'uniqueId');
    expect(sink.update(updatedSink)).toBe(true);
    expect(sink.getFriendlyName()).toEqual('name2');

    updatedSink = new DialSink('name', 'uniqueId')
                      .setDialAppUrl('http://192.168.111.1/apps/app2');
    expect(sink.update(updatedSink)).toBe(true);
    expect(sink.getDialAppUrl()).toEqual('http://192.168.111.1/apps/app2');

    updatedSink = new DialSink('name', 'uniqueId').setId('id2');
    expect(sink.update(updatedSink)).toBe(false);
  });

  it('Updates ip address', function() {
    const sink = new DialSink('name', 'uniqueId').setIpAddress('192.168.111.1');
    const updatedSink =
        new DialSink('name', 'uniqueId').setIpAddress('192.168.111.2');
    expect(sink.update(updatedSink)).toBe(true);
    expect(sink.getIpAddress()).toEqual('192.168.111.2');
  });

  it('Updates device description url', function() {
    const sink = new DialSink('name', 'uniqueId')
                     .setDeviceDescriptionUrl('http://192.168.111.1/desc');
    const updatedSink =
        new DialSink('name', 'uniqueId')
            .setDeviceDescriptionUrl('http://192.168.111.2/desc');
    expect(sink.update(updatedSink)).toBe(true);
    expect(sink.getDeviceDescriptionUrl()).toEqual('http://192.168.111.2/desc');
  });

  it('Creates sink from an Object', function() {
    const sink = new DialSink('name', 'uniqueId')
                     .setDialAppUrl('http://192.168.111.1/apps')
                     .setPort(8009)
                     .setDeviceDescriptionUrl('http://192.168.111.1/desc')
                     .setAppStatus('youtube', SinkAppStatus.AVAILABLE)
                     .setIpAddress('192.168.111.1')
                     .setModelName('chromecast');
    expect(DialSink.createFrom(sink)).toEqual(sink);
  });
});
