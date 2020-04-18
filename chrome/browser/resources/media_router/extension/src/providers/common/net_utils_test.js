// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Unit tests for mr.NetUtils.

 */

goog.module('mr.NetUtilsTest');
goog.setTestOnly('mr.NetUtilsTest');

const n = goog.require('mr.NetUtils');

describe('mr.NetUtils', () => {

  it('parses a valid IPv4 address', () => {
    expect(n.parseIPv4Address('128.164.100.104')).toEqual([128, 164, 100, 104]);
    expect(n.parseIPv4Address('0.0.0.0')).toEqual([0, 0, 0, 0]);
    expect(n.parseIPv4Address('255.255.255.255')).toEqual([255, 255, 255, 255]);
  });

  it('does not parse an invalid IPv4 address', () => {
    expect(n.parseIPv4Address('')).toBeNull();
    expect(n.parseIPv4Address('deadbeef')).toBeNull();
    expect(n.parseIPv4Address('128.164.100')).toBeNull();
    expect(n.parseIPv4Address('128.164.100.104.333')).toBeNull();
    expect(n.parseIPv4Address('256.164.100.104')).toBeNull();
    expect(n.parseIPv4Address('-1.164.100.104')).toBeNull();
  });

  it('validates an IPv4 private network address', () => {
    expect(n.isPrivateIPv4Address('10.0.0.0')).toBe(true);
    expect(n.isPrivateIPv4Address('10.255.255.255')).toBe(true);
    expect(n.isPrivateIPv4Address('172.16.0.0')).toBe(true);
    expect(n.isPrivateIPv4Address('172.31.255.255')).toBe(true);
    expect(n.isPrivateIPv4Address('192.168.0.0')).toBe(true);
    expect(n.isPrivateIPv4Address('192.168.255.255')).toBe(true);
  });

  it('does not validate an IPv4 public network address', () => {
    expect(n.isPrivateIPv4Address('9.255.255.255')).toBe(false);
    expect(n.isPrivateIPv4Address('11.0.0.0')).toBe(false);
    expect(n.isPrivateIPv4Address('172.15.255.255')).toBe(false);
    expect(n.isPrivateIPv4Address('172.32.0.0')).toBe(false);
    expect(n.isPrivateIPv4Address('193.167.255.255')).toBe(false);
    expect(n.isPrivateIPv4Address('193.169.0.0')).toBe(false);
  });

  it('parses a URL', () => {
    const url =
        n.parseUrl('https://www.example.com:8080/a/path?a_query#a_fragment');
    expect(url.protocol).toBe('https:');
    expect(url.hostname).toBe('www.example.com');
    expect(url.port).toBe('8080');
    expect(url.pathname).toBe('/a/path');
    expect(url.search).toBe('?a_query');
    expect(url.hash).toBe('#a_fragment');
    expect(url.host).toBe('www.example.com:8080');
  });
});
