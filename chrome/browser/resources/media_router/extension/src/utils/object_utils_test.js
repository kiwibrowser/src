// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('object_utils_test');

goog.require('mr.ObjectUtils');

describe('mr.ObjectUtils test', () => {
  const thudObj = {thud: 2};
  const obj = {foo: {bar: {xyzzy: null, baz: {qux: 1, corge: thudObj}}}};

  it('getPath returns undefined for nonexistent path', () => {
    expect(mr.ObjectUtils.getPath(obj, 'foo', 'bar', 'nonexistent'))
        .toBeUndefined();
    expect(
        mr.ObjectUtils.getPath(obj, 'foo', 'bar', 'baz', 'qux', 'nonexistent'))
        .toBeUndefined();
    expect(mr.ObjectUtils.getPath(obj, 'foo', 'bar', 'xyzzy', 'nonexistent'))
        .toBeUndefined();
  });

  it('getPath returns value', () => {
    expect(mr.ObjectUtils.getPath(obj, 'foo', 'bar', 'baz', 'qux')).toBe(1);
    expect(mr.ObjectUtils.getPath(obj, 'foo', 'bar', 'baz', 'corge'))
        .toBe(thudObj);
    expect(mr.ObjectUtils.getPath(obj, 'foo', 'bar', 'xyzzy')).toBeNull();
  });

  it('getPath returns itself if no path provided', () => {
    expect(mr.ObjectUtils.getPath(obj)).toBe(obj);
  });
});
