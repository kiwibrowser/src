// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('PresentationUrlTest');
goog.setTestOnly('PresentationUrlTest');

const PresentationUrl = goog.require('mr.dial.PresentationUrl');

describe('Tests PresentationUrl', function() {
  it('Does not create from empty input', function() {
    expect(PresentationUrl.create('')).toBeNull();
  });

  it('Creates from a valid URL', function() {
    expect(PresentationUrl.create(
               'https://www.youtube.com/tv#__dialAppName__=YouTube'))
        .toEqual(new PresentationUrl('YouTube'));
  });

  it('Creates from a valid URL with launch parameters', function() {
    expect(PresentationUrl.create(
               'https://www.youtube.com/tv#' +
               '__dialAppName__=YouTube/__dialPostData__=dj0xMjM='))
        .toEqual(new PresentationUrl('YouTube', 'v=123'));
    expect(PresentationUrl.create(
               'https://www.youtube.com/tv#' +
               '__dialAppName__=YouTube/__dialPostData__=dj1NSnlKS3d6eEZwWQ=='))
        .toEqual(new PresentationUrl('YouTube', 'v=MJyJKwzxFpY'));
  });

  it('Does not create from an invalid URL', function() {
    expect(PresentationUrl.create(
               'https://www.youtube.com/tv#___emanPpaLiad__=YouTube'))
        .toBeNull();
  });

  it('Does not create from an invalid postData', function() {
    expect(PresentationUrl.create(
               'https://www.youtube.com/tv#___emanPpaLiad__=YouTube' +
               '/__dialPostData__=dj1=N'))
        .toBeNull();
  });

  it('Creates from DIAL URL', () => {
    expect(PresentationUrl.create('dial:YouTube'))
        .toEqual(new PresentationUrl('YouTube'));
    expect(PresentationUrl.create('dial:YouTube?foo=bar'))
        .toEqual(new PresentationUrl('YouTube'));
    expect(PresentationUrl.create('dial:YouTube?foo=bar&postData=dj0xMjM='))
        .toEqual(new PresentationUrl('YouTube', 'v=123'));
    expect(PresentationUrl.create('dial:YouTube?postData=dj0xMjM%3D'))
        .toEqual(new PresentationUrl('YouTube', 'v=123'));
  });

  it('Does not create from invalid DIAL URL', () => {
    expect(PresentationUrl.create('dial:')).toBeNull();
    expect(PresentationUrl.create('dial://')).toBeNull();
    expect(PresentationUrl.create('dial://YouTube')).toBeNull();
    expect(
        PresentationUrl.create('dial:YouTube?postData=notEncodedProperly111'))
        .toBeNull();
  });

  it('Does not create from URL of unknown protocol', () => {
    expect(PresentationUrl.create('unknown:YouTube')).toBeNull();
  });

  it('getPresentationUrl returns DIAL presentation URLs', () => {
    expect(PresentationUrl.getPresentationUrlAsString('YouTube', null))
        .toEqual('dial:YouTube');
    expect(PresentationUrl.getPresentationUrlAsString('YouTube', 'dj0xMjM='))
        .toEqual('dial:YouTube?postData=dj0xMjM%3D');
  });
});
