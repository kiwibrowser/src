// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.base64.test');
goog.setTestOnly();

const {encodeArray, decodeString} = goog.require('mr.base64');

/**
 * Converts a ASCII string to an array of bytes.
 * @param {string} s
 * @return {!Array<number>}
 */
function strBytes(s) {
  return s.split('').map(c => c.codePointAt(0));
}

describe('mr.base64.encodeArray', () => {
  it('encodes well-known values correctly', () => {
    expect(encodeArray(strBytes(''))).toBe('');
    expect(encodeArray(strBytes('f'))).toBe('Zg==');
    expect(encodeArray(strBytes('fo'))).toBe('Zm8=');
    expect(encodeArray(strBytes('foo'))).toBe('Zm9v');
    expect(encodeArray(strBytes('foob'))).toBe('Zm9vYg==');
    expect(encodeArray(strBytes('fooba'))).toBe('Zm9vYmE=');
    expect(encodeArray(strBytes('foobar'))).toBe('Zm9vYmFy');
    expect(
        encodeArray(strBytes(
            '\xe4\xb8\x80\xe4\xba\x8c\xe4\xb8\x89\xe5\x9b\x9b\xe4\xba\x94\xe5' +
            '\x85\xad\xe4\xb8\x83\xe5\x85\xab\xe4\xb9\x9d\xe5\x8d\x81')))
        .toBe('5LiA5LqM5LiJ5Zub5LqU5YWt5LiD5YWr5Lmd5Y2B');
    expect(encodeArray(strBytes('>>>???>>>???=/+')))
        .toBe('Pj4+Pz8/Pj4+Pz8/PS8r');
  });

  it('handles the urlSafe parameter correctly', () => {
    expect(encodeArray(strBytes('f'), true)).toBe('Zg..');
    expect(encodeArray(strBytes('fo'), true)).toBe('Zm8.');
    expect(encodeArray(strBytes('foo'), true)).toBe('Zm9v');
    expect(encodeArray(strBytes('foob'), true)).toBe('Zm9vYg..');
    expect(encodeArray(strBytes('fooba'), true)).toBe('Zm9vYmE.');
    expect(encodeArray(strBytes('foobar'), true)).toBe('Zm9vYmFy');
    expect(encodeArray(strBytes('>>>???>>>???=/+'), true))
        .toBe('Pj4-Pz8_Pj4-Pz8_PS8r');
  });

  it('decodes correctly with the standard alphabet', () => {
    expect(decodeString('')).toBe('');
    expect(decodeString('Zg==')).toBe('f');
    expect(decodeString('Zm8=')).toBe('fo');
    expect(decodeString('Zm9v')).toBe('foo');
    expect(decodeString('Zm9vYg==')).toBe('foob');
    expect(decodeString('Zm9vYmE=')).toBe('fooba');
    expect(decodeString('Zm9vYmFy')).toBe('foobar');
    expect(decodeString('5LiA5LqM5LiJ5Zub5LqU5YWt5LiD5YWr5Lmd5Y2B'))
        .toBe(
            '\xe4\xb8\x80\xe4\xba\x8c\xe4\xb8\x89\xe5\x9b\x9b\xe4\xba\x94\xe5' +
            '\x85\xad\xe4\xb8\x83\xe5\x85\xab\xe4\xb9\x9d\xe5\x8d\x81');
    expect(decodeString('Pj4+Pz8/Pj4+Pz8/PS8r')).toBe('>>>???>>>???=/+');
  });

  it('decodes correctly with the URL-safe alphabet', () => {
    expect(decodeString('Zg..')).toBe('f');
    expect(decodeString('Zm8.')).toBe('fo');
    expect(decodeString('Zm9v')).toBe('foo');
    expect(decodeString('Zm9vYg..')).toBe('foob');
    expect(decodeString('Zm9vYmE.')).toBe('fooba');
    expect(decodeString('Zm9vYmFy')).toBe('foobar');
    expect(decodeString('Pj4-Pz8_Pj4-Pz8_PS8r')).toBe('>>>???>>>???=/+');
  });
});
