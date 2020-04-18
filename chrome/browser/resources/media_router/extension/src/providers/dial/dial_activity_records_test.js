// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.ActivityRecordsTest');
goog.setTestOnly('mr.dial.ActivityRecordsTest');

const Activity = goog.require('mr.dial.Activity');
const ActivityRecords = goog.require('mr.dial.ActivityRecords');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const Route = goog.require('mr.Route');
const UnitTestUtils = goog.require('mr.UnitTestUtils');

describe('DIAL ActivityRecords Tests', function() {
  let records;
  let mockCallbacks;
  let activity1;
  let activity2;

  beforeEach(function() {
    UnitTestUtils.mockMojoApi();
    UnitTestUtils.mockChromeApi();
    let route = new Route(
        'routeId1', 'presentationId1', 'sinkId1', null, false, 'description1',
        'imageUrl1');
    activity1 = new Activity(route, 'app1');
    route = new Route(
        'routeId2', 'presentationId2', 'sinkId2', null, true, 'description2',
        'imageUrl2');
    activity2 = new Activity(route, 'app2');
    mockCallbacks = jasmine.createSpyObj(
        'ActivityCallbacks',
        ['onActivityAdded', 'onActivityRemoved', 'onActivityUpdated']);
    records = new ActivityRecords(mockCallbacks);
    records.init();
  });

  afterEach(function() {
    PersistentDataManager.clear();
    UnitTestUtils.restoreChromeApi();
  });

  it('Add activity', function() {
    records.add(activity1);
    expect(records.getByRouteId(activity1.route.id)).toEqual(activity1);
    expect(mockCallbacks.onActivityAdded.calls.count()).toBe(1);
    expect(mockCallbacks.onActivityAdded).toHaveBeenCalledWith(activity1);
    records.add(activity1);
    expect(mockCallbacks.onActivityAdded.calls.count()).toBe(1);
  });

  it('Get activity and route', function() {
    records.add(activity1);
    expect(records.getByRouteId(activity1.route.id)).toEqual(activity1);
    expect(records.getBySinkId(activity1.route.sinkId)).toEqual(activity1);
    expect(records.getRoutes()).toEqual([activity1.route]);
    records.add(activity2);
    expect(records.getByRouteId(activity2.route.id)).toEqual(activity2);
    expect(records.getBySinkId(activity2.route.sinkId)).toEqual(activity2);
    expect(records.getRoutes()).toEqual([activity1.route, activity2.route]);
  });

  it('Remove activity', function() {
    records.add(activity1);
    records.add(activity2);
    records.removeByRouteId(activity2.route.id);
    expect(mockCallbacks.onActivityRemoved.calls.count()).toBe(1);
    expect(mockCallbacks.onActivityRemoved).toHaveBeenCalledWith(activity2);
    records.removeByRouteId(activity2.route.id);
    expect(mockCallbacks.onActivityRemoved.calls.count()).toBe(1);
    expect(records.getByRouteId(activity2.route.id)).toEqual(null);
    expect(records.getBySinkId(activity2.route.sinkId)).toEqual(null);
    expect(records.getRoutes()).toEqual([activity1.route]);

    records.removeBySinkId(activity1.route.sinkId);
    expect(mockCallbacks.onActivityRemoved.calls.count()).toBe(2);
    expect(mockCallbacks.onActivityRemoved).toHaveBeenCalledWith(activity1);
    expect(records.getRoutes()).toEqual([]);
  });

  it('Save without any data', function() {
    expect(records.getRoutes()).toEqual([]);
    PersistentDataManager.suspendForTest();
    records = new ActivityRecords(mockCallbacks);
    records.loadSavedData();
    expect(records.getRoutes()).toEqual([]);
  });

  it('Save with data', function() {
    records.add(activity1);
    records.add(activity2);
    PersistentDataManager.suspendForTest();
    records = new ActivityRecords(mockCallbacks);
    records.loadSavedData();
    expect(JSON.stringify(records.getRoutes())).toEqual(JSON.stringify([
      activity1.route, activity2.route
    ]));
  });

});
