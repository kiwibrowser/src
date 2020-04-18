// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.Route');
goog.require('mr.TabUtils');
goog.require('mr.mirror.Session');

describe('Tests mr.mirror.Session', () => {
  let mirrorRoute;
  let session;
  let onActivityUpdated;

  function expectNormalSession(s) {
    expect(s.tabId).toBe(47);
    expect(s.tab).not.toBe(null);
    expect(s.isRemoting).toBe(false);
    expect(s.activity.getRouteDescription())
        .toBe('Casting tab (news.google.com)');
    expect(s.activity.getRouteMediaStatus()).toBe('Google News');
    expect(s.activity.getCastRemoteTitle()).toBe('Casting tab');
  }

  function expectIncognitoSession(s) {
    expect(s.tabId).toBe(47);
    expect(s.tab).not.toBe(null);
    expect(s.isRemoting).toBe(false);
    expect(s.activity.getRouteDescription())
        .toBe('Casting tab (news.google.com)');
    expect(s.activity.getRouteMediaStatus()).toBe('Google News');
    expect(s.activity.getCastRemoteTitle()).toBe('Casting active');
  }

  beforeEach(() => {
    window['mojo'] = null;  // Workaround to allow tests to run in Jasmine
                            // without mojo bindings
    mirrorRoute = new mr.Route(
        'routeId', 'presentationId', 'sinkId',
        'urn:x-org.chromium.media:source:tab:47', true, null);
    onActivityUpdated = jasmine.createSpy();
    session = new mr.mirror.Session(mirrorRoute, onActivityUpdated);
    session.tabId = 47;
    spyOn(session, 'sendActivityToSink');
  });

  it('has default data for a tab mirroring route', () => {
    expect(session.route).toBe(mirrorRoute);
    expect(session.tabId).toBe(47);
    expect(session.tab).toBe(null);
    expect(session.isRemoting).toBe(false);
    expect(session.activity.getRouteDescription()).toBe('Casting tab');
    expect(session.activity.getRouteMediaStatus()).toBe('');
    expect(session.activity.getCastRemoteTitle()).toBe('Casting tab');
  });

  describe('onTabUpdated', () => {
    it('sets all fields with normal tab', () => {
      session.onTabUpdated(47, {'status': 'complete'}, {
        'title': 'Google News',
        'url': 'https://news.google.com',
        'incognito': false
      });
      expect(session.sendActivityToSink).toHaveBeenCalled();
      expect(onActivityUpdated).toHaveBeenCalled();
      expectNormalSession(session);
    });
    it('sets some fields with incognito tab', () => {
      session.onTabUpdated(47, {'status': 'complete'}, {
        'title': 'Google News',
        'url': 'https://news.google.com',
        'incognito': true
      });
      expect(session.sendActivityToSink).toHaveBeenCalled();
      expect(onActivityUpdated).toHaveBeenCalled();
      expectIncognitoSession(session);
    });
    it('sets some fields with OTR route', () => {
      mirrorRoute.offTheRecord = true;
      otrSession = new mr.mirror.Session(mirrorRoute, onActivityUpdated);
      otrSession.tabId = 47;
      spyOn(otrSession, 'sendActivityToSink');
      otrSession.onTabUpdated(47, {'status': 'complete'}, {
        'title': 'Google News',
        'url': 'https://news.google.com',
        'incognito': true
      });
      expect(otrSession.sendActivityToSink).toHaveBeenCalled();
      expect(onActivityUpdated).toHaveBeenCalled();
      expectIncognitoSession(otrSession);
    });
  });

  describe('setTabId', () => {
    beforeEach((done) => {
      spyOn(mr.TabUtils, 'getTab').and.returnValue(Promise.resolve({
        'title': 'CNN',
        'url': 'https://www.cnn.com',
        'incognito': false
      }));
      session.setTabId(48);
      done();
    });

    it('updates the tab', (done) => {
      expect(session.sendActivityToSink).toHaveBeenCalled();
      expect(onActivityUpdated).toHaveBeenCalled();
      expect(session.tabId).toBe(48);
      expect(session.tab).not.toBe(null);
      expect(session.isRemoting).toBe(false);
      expect(session.activity.getRouteDescription())
          .toBe('Casting tab (www.cnn.com)');
      expect(session.activity.getRouteMediaStatus()).toBe('CNN');
      expect(session.activity.getCastRemoteTitle()).toBe('Casting tab');
      done();
    });
  });
});
