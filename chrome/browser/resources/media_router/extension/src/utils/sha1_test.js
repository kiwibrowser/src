// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.Sha1.test');
goog.setTestOnly();

const Sha1 = goog.require('mr.Sha1');

/**
 * Converts an array of byte values to a hexadecimal string.
 * @param {!Array<number>} bytes
 * @return {string}
 */
function toHex(bytes) {
  return bytes.map(byte => byte.toString(16).padStart(2, '0')).join('');
}

describe('mr.Sha1', () => {
  // Test vectors from:
  // csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf

  let sha1;

  beforeEach(() => {
    sha1 = new Sha1();
  });

  it('hashes an empty stream correctly', () => {
    expect(toHex(sha1.digest()))
        .toBe('da39a3ee5e6b4b0d3255bfef95601890afd80709');
  });

  it('hashes a one-block message correctly', () => {
    sha1.update('abc');
    expect(toHex(sha1.digest()))
        .toBe('a9993e364706816aba3e25717850c26c9cd0d89d');
  });

  it('hashes a multi-block message correctly', () => {
    sha1.update('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq');
    expect(toHex(sha1.digest()))
        .toBe('84983e441c3bd26ebaae4aa1f95129e5e54670f1');
  });

  it('hashes a long message correctly', () => {
    const thousandAs = 'a'.repeat(1000);
    for (let i = 0; i < 1000; ++i) {
      sha1.update(thousandAs);
    }
    expect(toHex(sha1.digest()))
        .toBe('34aa973cd4c4daa4f61eeb2bdbad27316534016f');
  });

  it('hashes a standard message correctly', () => {
    sha1.update('The quick brown fox jumps over the lazy dog');
    expect(toHex(sha1.digest()))
        .toBe('2fd4e1c67a2d28fced849ee1bb76e7391b93eb12');
  });
});
