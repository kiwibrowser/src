// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.ClientTest');
goog.setTestOnly('mr.dial.ClientTest');

const DialClient = goog.require('mr.dial.Client');
const DialSink = goog.require('mr.dial.Sink');
const XhrUtils = goog.require('mr.XhrUtils');

describe('Dial Client Tests', function() {
  let client;
  let mockXhrManager;
  let sink;
  const appUrl = 'http://198.0.0.100/apps';


  beforeEach(function() {
    sink = new DialSink('sink', 'sinkid');
    sink.setDialAppUrl(appUrl);
    mockXhrManager = jasmine.createSpyObj('XhrManager', ['send']);
    spyOn(XhrUtils, 'logRawXhr');
    client = new DialClient.Client(sink, mockXhrManager);
  });

  const setMockXhrResponse = function(xml) {
    mockXhrManager.send.and.returnValue(
        Promise.resolve({responseText: xml, status: 200}));
  };

  const setMockXhrErrorResponse = function() {
    mockXhrManager.send.and.returnValue(
        Promise.resolve({responseText: null, status: 403 /* Forbidden */}));
  };

  const setMockXhrNotFoundResponse = function() {
    mockXhrManager.send.and.returnValue(
        Promise.resolve({responseText: null, status: 404}));
  };

  const setMockXhrReject = function() {
    mockXhrManager.send.and.returnValue(
        Promise.reject(new Error('send failed')));
  };

  // Suppress Jasmine warning about a spec with no expectations.
  const noExpectations = function() {
    expect(true).toBe(true);
  };

  describe('Tests launchApp', function() {
    const expectLaunchAppFails = function(done) {
      client.launchApp('YouTube', 'v=12345678').then(() => {
        fail('launchApp unexpectedly succeeded.');
      }, done);
    };

    it('Resolves', done => {
      setMockXhrResponse('');
      client.launchApp('YouTube', 'v=12345678').then(() => {
        expect(mockXhrManager.send)
            .toHaveBeenCalledWith(
                appUrl + '/YouTube', 'POST', 'v=12345678', jasmine.any(Object));
        done();
      });
    });

    it('Rejects on error response', done => {
      setMockXhrErrorResponse();
      expectLaunchAppFails(done);
      noExpectations();
    });

    it('Rejects on send rejection', done => {
      setMockXhrReject();
      expectLaunchAppFails(done);
      noExpectations();
    });
  });

  describe('Tests stopApp', function() {
    const expectStopAppFails = function(done) {
      client.stopApp('YouTube').then(() => {
        fail('stopApp unexpectedly succeeded.');
      }, done);
    };

    it('Resolves', done => {
      setMockXhrResponse('');
      client.stopApp('YouTube').then(() => {
        expect(mockXhrManager.send)
            .toHaveBeenCalledWith(appUrl + '/YouTube', 'DELETE');
        done();
      });
    });

    it('Rejects on error response', done => {
      setMockXhrErrorResponse();
      expectStopAppFails(done);
      noExpectations();
    });

    it('Rejects on send rejection', done => {
      setMockXhrReject();
      expectStopAppFails(done);
      noExpectations();
    });
  });

  const expectMockSendGet = function() {
    expect(mockXhrManager.send)
        .toHaveBeenCalledWith(
            'http://198.0.0.100/apps/YouTube', 'GET', undefined,
            jasmine.any(Object));
  };

  const VALID_GET_RESPONSE_ = '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<name>YouTube</name>' +
      '<options allowStop="false"/>' +
      '<state>running</state>' +
      '<link rel="run" href="run"/>' +
      '</service>';

  const VALID_GET_RESPONSE_EXTRA_DATA_ =
      '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<name>YouTube</name>' +
      '<state>running</state>' +
      '<link rel="run" href="run"/>' +
      '<port>8080</port>' +
      '<additionalData>' +
      '<screenId>e5n3112oskr42pg0td55b38nh4</screenId>' +
      '<otherField>2</otherField>' +
      '</additionalData>' +
      '</service>';

  const INVALID_GET_RESPONSE_NO_SERVICE_ =
      '<?xml version="1.0" encoding="UTF-8"?>';

  const INVALID_GET_RESPONSE_NO_STATE_ =
      '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<name>YouTube</name>' +
      '<options allowStop="true"/>' +
      '<link rel="run" href="run"/>' +
      '</service>';

  const INVALID_GET_RESPONSE_INVALID_STATE_ =
      '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<name>YouTube</name>' +
      '<options allowStop="true"/>' +
      '<state>xyzzy</state>' +
      '<link rel="run" href="run"/>' +
      '</service>';

  const INVALID_GET_RESPONSE_INSTALLABLE_ =
      '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<name>YouTube</name>' +
      '<options allowStop="true"/>' +
      '<state>installable=http://play.google.com/youtube</state>' +
      '<link rel="run" href="run"/>' +
      '</service>';

  const INVALID_GET_RESPONSE_NO_NAME_ =
      '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<options allowStop="true"/>' +
      '<state>running</state>' +
      '<link rel="run" href="run"/>' +
      '</service>';

  const INVALID_GET_RESPONSE_WRONG_APP_NAME_ =
      '<?xml version="1.0" encoding="UTF-8"?>' +
      '<service xmlns="urn:dial-multiscreen-org:schemas:dial">' +
      '<name>WrongAppName</name>' +
      '<options allowStop="true"/>' +
      '<state>running</state>' +
      '<link rel="run" href="run"/>' +
      '</service>';

  describe('Tests getAppInfo', function() {
    it('Returns info from valid response', done => {
      setMockXhrResponse(VALID_GET_RESPONSE_);
      client.getAppInfo('YouTube').then(appInfo => {
        expect(appInfo.name).toEqual('YouTube');
        expect(appInfo.state).toEqual('running');
        expect(appInfo.allowStop).toBe(false);
        expect(appInfo.resource).toEqual('run');
        expectMockSendGet();
        done();
      });
    });

    it('Returns info with extraData', done => {
      setMockXhrResponse(VALID_GET_RESPONSE_EXTRA_DATA_);
      client.getAppInfo('YouTube').then(appInfo => {
        expect(appInfo.name).toEqual('YouTube');
        expect(appInfo.state).toEqual('running');
        expect(appInfo.allowStop).toBe(true);
        expect(appInfo.resource).toEqual('run');
        expect(appInfo.extraData.port).toEqual('8080');
        expect(appInfo.extraData.additionalData)
            .toEqual(
                '<screenId xmlns="urn:dial-multiscreen-org:schemas:dial">' +
                'e5n3112oskr42pg0td55b38nh4</screenId>' +
                '<otherField xmlns="urn:dial-multiscreen-org:schemas:dial">2' +
                '</otherField>');
        expectMockSendGet();
        done();
      });
    });

    const expectGetAppInfoFails = function(done) {
      client.getAppInfo('YouTube').then(
          _ => {
            fail('getAppInfo unexpectedly succeeded.');
          },
          e => {
            expectMockSendGet();
            done();
          });
    };

    const testInvalidResponse = function(response, done) {
      setMockXhrResponse(response);
      expectGetAppInfoFails(done);
    };

    it('Rejects on invalid response 1', done => {
      testInvalidResponse('blarg', done);
    });

    it('Rejects on invalid response 2', done => {
      testInvalidResponse(INVALID_GET_RESPONSE_NO_SERVICE_, done);
    });

    it('Rejects on invalid response 3', done => {
      testInvalidResponse(INVALID_GET_RESPONSE_NO_STATE_, done);
    });

    it('Rejects on invalid response 4', done => {
      testInvalidResponse(INVALID_GET_RESPONSE_NO_NAME_, done);
    });

    it('Rejects on invalid response 5', done => {
      testInvalidResponse(INVALID_GET_RESPONSE_INVALID_STATE_, done);
    });

    it('Rejects on invalid response 6', done => {
      testInvalidResponse(INVALID_GET_RESPONSE_INSTALLABLE_, done);
    });

    it('Rejects on mismatched app name', done => {
      testInvalidResponse(INVALID_GET_RESPONSE_WRONG_APP_NAME_, done);
    });

    it('Rejects on error response', done => {
      setMockXhrErrorResponse();
      expectGetAppInfoFails(done);
    });

    it('Rejects on send rejection', done => {
      setMockXhrReject();
      expectGetAppInfoFails(done);
    });

    it('Rejects with AppInfoNotFoundError', done => {
      setMockXhrNotFoundResponse();
      client.getAppInfo('YouTube').then(
          _ => {
            fail('getAppInfo unexpectedly succeeded.');
          },
          e => {
            expect(e instanceof DialClient.AppInfoNotFoundError).toBe(true);
            expectMockSendGet();
            done();
          });
    });
  });
});
