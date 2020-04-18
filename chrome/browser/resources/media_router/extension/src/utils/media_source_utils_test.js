// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.require('mr.MediaSourceUtils');

describe('Tests MediaSourceUtils', function() {
  describe('Tests isTabMirrorSource', function() {
    it('should return true for tab mirror source', function() {
      expect(mr.MediaSourceUtils.isTabMirrorSource(
                 'urn:x-org.chromium.media:source:tab:2'))
          .toBe(true);
      expect(mr.MediaSourceUtils.isTabMirrorSource(
                 'urn:x-org.chromium.media:source:tab:666'))
          .toBe(true);
    });

    it('should return false for non tab mirror source', function() {
      expect(mr.MediaSourceUtils.isTabMirrorSource(
                 'urn:x-org.chromium.media:source:desktop'))
          .toBe(false);
    });
  });

  describe('Tests isPresentationSource', function() {
    it('should return true for presentation source', function() {
      expect(mr.MediaSourceUtils.isPresentationSource('http://www.google.com'))
          .toBe(true);
      expect(mr.MediaSourceUtils.isPresentationSource('https://www.google.com'))
          .toBe(true);
    });

    it('should return false for non tab mirror source', function() {
      expect(
          mr.MediaSourceUtils.isPresentationSource('Invalid media source urn'))
          .toBe(false);
    });

    it('should return false for cast receiver app', function() {
      expect(mr.MediaSourceUtils.isPresentationSource(
                 'http://www.google.com/cast#__castAppId__=deadbeef'))
          .toBe(false);
    });
  });

  describe('Tests getMirrorTabId', function() {
    it('should return null for non tab mirror source or invalid ID',
       function() {
         expect(mr.MediaSourceUtils.getMirrorTabId('http://www.google.com'))
             .toBeNull();
         expect(mr.MediaSourceUtils.getMirrorTabId(
                    'urn:x-org.chromium.media:source:tab:'))
             .toBeNull();
       });

    it('should return correct ID for correct tab mirror source', function() {
      expect(mr.MediaSourceUtils.getMirrorTabId(
                 'urn:x-org.chromium.media:source:tab:2'))
          .toBe(2);
      expect(mr.MediaSourceUtils.getMirrorTabId(
                 'urn:x-org.chromium.media:source:tab:666'))
          .toBe(666);
    });
  });

});
