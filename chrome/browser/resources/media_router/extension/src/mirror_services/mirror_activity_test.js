// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.Route');
goog.require('mr.mirror.Activity');

describe('Tests mr.mirror.Activity', () => {
  let tabMirrorRoute;
  let desktopMirrorRoute;
  let presentationRoute;

  beforeEach(() => {
    tabMirrorRoute = new mr.Route(
        'routeId', 'presentationId', 'sinkId',
        'urn:x-org.chromium.media:source:tab:47', true, null);
    desktopMirrorRoute = new mr.Route(
        'routeId2', 'presentationId2', 'sinkId2',
        'urn:x-org.chromium.media:source:desktop', true, null);
    presentationRoute = new mr.Route(
        'routeId3', 'presentationId3', 'sinkId3', 'https://www.example.com',
        true, null);
  });

  it('creates activity from a tab mirroring route', () => {
    let activity = mr.mirror.Activity.createFromRoute(tabMirrorRoute);
    expect(activity.getRouteDescription()).toBe('Casting tab');
    expect(activity.getRouteMediaStatus()).toBe('');
    expect(activity.getCastRemoteTitle()).toBe('Casting tab');
  });

  it('creates activity from a desktop mirroring route', () => {
    let activity = mr.mirror.Activity.createFromRoute(desktopMirrorRoute);
    expect(activity.getRouteDescription()).toBe('Casting desktop');
    expect(activity.getRouteMediaStatus()).toBe('');
    expect(activity.getCastRemoteTitle()).toBe('Casting desktop');
  });

  it('creates activity from a presentation route', () => {
    let activity = mr.mirror.Activity.createFromRoute(presentationRoute);
    expect(activity.getRouteDescription())
        .toBe('Casting https://www.example.com');
    expect(activity.getRouteMediaStatus()).toBe('');
    expect(activity.getCastRemoteTitle()).toBe('Casting site');
  });

  it('uses the tab origin and page title for tab mirroring', () => {
    let activity = mr.mirror.Activity.createFromRoute(tabMirrorRoute);
    activity.setOrigin('news.google.com');
    activity.setContentTitle('Google News');
    expect(activity.getRouteDescription())
        .toBe('Casting tab (news.google.com)');
    expect(activity.getRouteMediaStatus()).toBe('Google News');
    expect(activity.getCastRemoteTitle()).toBe('Casting tab');
  });

  it('uses the tab origin and page title for presentation', () => {
    let activity = mr.mirror.Activity.createFromRoute(presentationRoute);
    activity.setOrigin('www.example.com');
    activity.setContentTitle('Some Presentation');
    expect(activity.getRouteDescription()).toBe('Casting www.example.com');
    expect(activity.getRouteMediaStatus()).toBe('Some Presentation');
    expect(activity.getCastRemoteTitle()).toBe('Casting site');
  });

  it('updates the remote title for incognito', () => {
    let activity = mr.mirror.Activity.createFromRoute(tabMirrorRoute);
    activity.setIncognito(true);
    expect(activity.getCastRemoteTitle()).toBe('Casting active');
  });

  it('updates the activity for remoting', () => {
    let activity = mr.mirror.Activity.createFromRoute(tabMirrorRoute);
    activity.setOrigin('www.vimeo.com');
    activity.setContentTitle('Vimeo');
    activity.setType(mr.mirror.Activity.Type.MEDIA_REMOTING);
    expect(activity.getRouteDescription())
        .toBe('Casting media (www.vimeo.com)');
    expect(activity.getRouteMediaStatus()).toBe('Vimeo');
    expect(activity.getCastRemoteTitle()).toBe('Casting media');
  });

  it('updates the activity for mirroring local media', () => {
    let activity = mr.mirror.Activity.createFromRoute(tabMirrorRoute);
    activity.setOrigin('');
    activity.setContentTitle('some_file.mp4');
    activity.setType(mr.mirror.Activity.Type.MIRROR_FILE);
    expect(activity.getRouteDescription()).toBe('Casting local content');
    expect(activity.getRouteMediaStatus()).toBe('some_file.mp4');
    expect(activity.getCastRemoteTitle()).toBe('Casting local content');
  });
});
