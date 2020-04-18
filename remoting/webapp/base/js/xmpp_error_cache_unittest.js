// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

QUnit.module('XmppErrorCache');

/**
 * @param {string} stanza
 * @param {string} expected
 * @param {QUnit.Assert} assert
 */
function runStanzaTest(stanza, expected, assert) {
  var monitor = new remoting.XmppErrorCache();
  var parser = new DOMParser();
  var xml = parser.parseFromString(stanza, 'text/xml');
  monitor.processStanza(xml.firstElementChild);
  var errorStanza = monitor.getFirstErrorStanza();
  assert.equal(errorStanza, expected);
}

QUnit.test('should strip PII from session-initiate.', function(assert) {
  var input =
    '<iq xmlns="jabber:client" to="foo@google.com/chromotingE5A" ' +
        'type="error" id="12747556118995360108" ' +
        'from="123@chromoting.gserviceaccount.com/chromoting21B21C01">' +
        '<jingle xmlns="urn:xmpp:jingle:1" sid="3953621430175055977" ' +
                'action="session-initiate" ' +
                'initiator="foo@google.com/chromotingE5A">' +
          '<content name="chromoting" creator="initiator">' +
            '<description xmlns="google:remoting">' +
              '<standard-ice/>' +
                '<control transport="mux-stream" version="3"/>' +
                '<event transport="mux-stream" version="2"/>' +
                '<video transport="stream" version="2" codec="vp9"/>' +
                '<video transport="stream" version="2" codec="vp8"/>' +
                '<audio transport="mux-stream" version="2" codec="opus"/>' +
                '<audio transport="none"/>' +
                '<authentication supported-methods="spake2_plain"/>' +
              '</description>' +
              '<unknown-field-that-contains-pii unknown-attribute="pii">' +
                'This is PII' +
              '</unknown-field-that-contains-pii>' +
            '</content>' +
          '</jingle>' +
          '<error code="503" type="cancel">' +
            '<service-unavailable xmlns="urn:ietf:xml:ns:xmpp-stanzas"/>' +
          '</error>' +
        '</iq>';
  var expected =
      '<iq xmlns="jabber:client" to="REDACTED" type="error" ' +
          'id="12747556118995360108" from="REDACTED">' +
        '<jingle xmlns="urn:xmpp:jingle:1" sid="3953621430175055977" ' +
                'action="session-initiate" initiator="REDACTED">' +
          '<content name="chromoting" creator="initiator">' +
            '<description xmlns="google:remoting">' +
              '<standard-ice/>' +
                '<control transport="mux-stream" version="3"/>' +
                '<event transport="mux-stream" version="2"/>' +
                '<video transport="stream" version="2" codec="vp9"/>' +
                '<video transport="stream" version="2" codec="vp8"/>' +
                '<audio transport="mux-stream" version="2" codec="opus"/>' +
                '<audio transport="none"/>' +
                '<authentication supported-methods="spake2_plain"/>' +
              '</description>' +
              '<unknown-field-that-contains-pii unknown-attribute="REDACTED">'+
                'REDACTED' +
              '</unknown-field-that-contains-pii>' +
            '</content>' +
          '</jingle>' +
          '<error code="503" type="cancel">' +
            '<service-unavailable xmlns="urn:ietf:xml:ns:xmpp-stanzas"/>' +
          '</error>' +
        '</iq>';
  runStanzaTest(input, expected, assert);
});

})();
