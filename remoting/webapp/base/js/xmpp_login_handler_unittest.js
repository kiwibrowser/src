// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

var testUsername = 'testUsername@gmail.com';
var testToken = 'testToken';

/** @type {(sinon.Spy|function(string):void)} */
var sendMessage_spy = function(msg) {};
/** @type {function(string):void} */
var sendMessage = function(msg) {};

/** @type {(sinon.Spy|function():void)} */
var startTls_spy = function() {};
/** @type {function():void} */
var startTls = function() {};

/** @type {(sinon.Spy|function(string, remoting.XmppStreamParser):void)} */
var onHandshakeDone_spy = function(name, parser) {};
/** @type {function(string, remoting.XmppStreamParser):void} */
var onHandshakeDone = function(name, parser) {};

/** @type {(sinon.Spy|function(remoting.Error, string):void)} */
var onError_spy = function(error, message) {};
/** @type {function(remoting.Error, string):void} */
var onError = function(error, message) {};

/** @type {remoting.XmppLoginHandler} */
var loginHandler = null;

QUnit.module('XmppLoginHandler', {
  beforeEach: function() {
    sendMessage_spy = sinon.spy();
    sendMessage = /** @type {function(string):void} */ (sendMessage_spy);
    startTls_spy = sinon.spy();
    startTls = /** @type {function():void} */ (startTls_spy);
    onHandshakeDone_spy = sinon.spy();
    onHandshakeDone =
          /** @type {function(string, remoting.XmppStreamParser):void} */
          (onHandshakeDone_spy);
    onError_spy = sinon.spy();
    onError = /** @type {function(remoting.Error, string):void} */(onError_spy);

    loginHandler = new remoting.XmppLoginHandler(
        'google.com', testUsername, testToken,
        remoting.TlsMode.WITHOUT_HANDSHAKE, sendMessage, startTls,
        onHandshakeDone, onError);
  }
});

// Executes handshake base.
function handshakeBase() {
  loginHandler.start();

  sinon.assert.calledWith(startTls);
  startTls_spy.reset();

  loginHandler.onTlsStarted();
  var cookie = window.btoa("\0" + testUsername + "\0" + testToken);
  sinon.assert.calledWith(
      sendMessage,
      '<stream:stream to="google.com" version="1.0" xmlns="jabber:client" ' +
          'xmlns:stream="http://etherx.jabber.org/streams">' +
      '<auth xmlns="urn:ietf:params:xml:ns:xmpp-sasl" mechanism="X-OAUTH2" ' +
          'auth:service="oauth2" auth:allow-generated-jid="true" ' +
          'auth:client-uses-full-bind-result="true" ' +
          'auth:allow-non-google-login="true" ' +
          'xmlns:auth="http://www.google.com/talk/protocol/auth">' + cookie +
      '</auth>');
  sendMessage_spy.reset();

  loginHandler.onDataReceived(base.encodeUtf8(
      '<stream:stream from="google.com" id="DCDDE5171CB2154A" version="1.0" ' +
          'xmlns:stream="http://etherx.jabber.org/streams" ' +
          'xmlns="jabber:client">' +
        '<stream:features>' +
          '<mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">' +
            '<mechanism>X-OAUTH2</mechanism>' +
            '<mechanism>X-GOOGLE-TOKEN</mechanism>' +
            '<mechanism>PLAIN</mechanism>' +
          '</mechanisms>' +
        '</stream:features>'));
}

QUnit.test('should authenticate', function() {
  handshakeBase();

  loginHandler.onDataReceived(
      base.encodeUtf8('<success xmlns="urn:ietf:params:xml:ns:xmpp-sasl"/>'));
  sinon.assert.calledWith(
      sendMessage,
      '<stream:stream to="google.com" version="1.0" xmlns="jabber:client" ' +
          'xmlns:stream="http://etherx.jabber.org/streams">' +
        '<iq type="set" id="0">' +
          '<bind xmlns="urn:ietf:params:xml:ns:xmpp-bind">' +
            '<resource>chromoting</resource>' +
          '</bind>' +
        '</iq>' +
        '<iq type="set" id="1">' +
          '<session xmlns="urn:ietf:params:xml:ns:xmpp-session"/>' +
        '</iq>');
  sendMessage_spy.reset();

  loginHandler.onDataReceived(base.encodeUtf8(
      '<stream:stream from="google.com" id="104FA10576E2AA80" version="1.0" ' +
          'xmlns:stream="http://etherx.jabber.org/streams" ' +
          'xmlns="jabber:client">' +
        '<stream:features>' +
          '<bind xmlns="urn:ietf:params:xml:ns:xmpp-bind"/>' +
          '<session xmlns="urn:ietf:params:xml:ns:xmpp-session"/>' +
        '</stream:features>' +
        '<iq id="0" type="result">' +
          '<bind xmlns="urn:ietf:params:xml:ns:xmpp-bind">' +
            '<jid>' + testUsername + '/chromoting52B4920E</jid>' +
          '</bind>' +
        '</iq>' +
        '<iq type="result" id="1"/>'));

  sinon.assert.calledWith(onHandshakeDone);
});

QUnit.test('use <starttls> handshake', function() {
  loginHandler = new remoting.XmppLoginHandler(
      'google.com', testUsername, testToken, remoting.TlsMode.WITH_HANDSHAKE,
      sendMessage, startTls, onHandshakeDone, onError);
  loginHandler.start();

  sinon.assert.calledWith(
      sendMessage,
      '<stream:stream to="google.com" version="1.0" xmlns="jabber:client" ' +
          'xmlns:stream="http://etherx.jabber.org/streams">' +
          '<starttls xmlns="urn:ietf:params:xml:ns:xmpp-tls"/>');
  sendMessage_spy.reset();

  loginHandler.onDataReceived(base.encodeUtf8(
      '<stream:stream from="google.com" id="78A87C70559EF28A" version="1.0" ' +
          'xmlns:stream="http://etherx.jabber.org/streams" ' +
          'xmlns="jabber:client">' +
        '<stream:features>' +
          '<starttls xmlns="urn:ietf:params:xml:ns:xmpp-tls">' +
            '<required/>' +
          '</starttls>' +
          '<mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">' +
            '<mechanism>X-OAUTH2</mechanism>' +
            '<mechanism>X-GOOGLE-TOKEN</mechanism>' +
          '</mechanisms>' +
        '</stream:features>'));

  loginHandler.onDataReceived(
      base.encodeUtf8('<proceed xmlns="urn:ietf:params:xml:ns:xmpp-tls"/>'));

  sinon.assert.calledWith(startTls);
});

QUnit.test(
    'should return AUTHENTICATION_FAILED error when failed to authenticate',
    function() {
  handshakeBase();

  loginHandler.onDataReceived(
      base.encodeUtf8('<failure xmlns="urn:ietf:params:xml:ns:xmpp-sasl">' +
                      '<not-authorized/></failure>'));
  sinon.assert.calledWith(onError, new remoting.Error(
      remoting.Error.Tag.AUTHENTICATION_FAILED));
});

QUnit.test('should return UNEXPECTED error when failed to parse stream',
     function() {
  handshakeBase();
  loginHandler.onDataReceived(
      base.encodeUtf8('BAD DATA'));
  sinon.assert.calledWith(onError, remoting.Error.unexpected());
});

})();
