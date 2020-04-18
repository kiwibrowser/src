// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.require('mr.mirror.Settings');

describe('Tests Mirror Settings', function() {
  it('produces JSON strings containing public fields', function() {
    // Emit Settings as a JSON-format string.
    const settings = new mr.mirror.Settings();
    const jsonString = settings.toJsonString();

    // The string should be parseable as JSON and contain only public fields.
    const parsed = JSON.parse(jsonString);
    expect(typeof parsed['maxWidth']).toBe('number');
    expect(typeof parsed['enableLogging']).toBe('boolean');
    expect(Object.keys(parsed).sort())
        .toEqual(Object.keys(settings).filter(x => !x.endsWith('_')).sort());
  });

  it('only updates public fields from localStorage overrides', function() {
    const settings = new mr.mirror.Settings();

    // Normal case: A public field is updated with a new value of the same type.
    const originalMaxWidth = settings.maxWidth;
    settings.update_({'maxWidth': originalMaxWidth / 2});
    expect(settings.maxWidth).toBe(originalMaxWidth / 2);

    // Bad case: A public field being set to a value of a different type.
    const jsonBefore = settings.toJsonString();
    settings.update_({'maxWidth': true});
    expect(settings.toJsonString()).toEqual(jsonBefore);

    // Bad case: A non-existant field.
    settings.update_({'fooey': true});
    expect(settings.toJsonString()).toEqual(jsonBefore);

    // Bad case: Attempt to set private field.
    const loggerBefore = settings.logger_;
    const injectionAttackFunc = function() {
      return 'MUAHAHAHAHA!';
    };
    settings.update_({'logger_': injectionAttackFunc});
    expect(settings.logger_).not.toBe(injectionAttackFunc);
    expect(settings.logger_).toBe(loggerBefore);
    expect(settings.toJsonString()).toEqual(jsonBefore);
  });

  it('clamps max dimensions to 1920x1080 screen size', function() {
    spyOn(mr.mirror.Settings, 'getScreenWidth').and.returnValue(1920);
    spyOn(mr.mirror.Settings, 'getScreenHeight').and.returnValue(1080);
    const settings = new mr.mirror.Settings();
    settings.makeFinalAdjustmentsAndFreeze();
    expect(settings.maxWidth).toBe(1920);
    expect(settings.maxHeight).toBe(1080);
  });

  it('clamps max dimensions to 1366x768 screen size', function() {
    spyOn(mr.mirror.Settings, 'getScreenWidth').and.returnValue(1366);
    spyOn(mr.mirror.Settings, 'getScreenHeight').and.returnValue(768);
    const settings = new mr.mirror.Settings();
    settings.makeFinalAdjustmentsAndFreeze();
    expect(settings.maxWidth).toBe(1280);
    expect(settings.maxHeight).toBe(720);
  });

  it('returns min size matching aspect ratio of max size', function() {
    const settings = new mr.mirror.Settings();
    settings.maxWidth = 1920;
    settings.maxHeight = 1080;
    settings.minWidth = 320;
    settings.minHeight = 180;
    expect(settings.minAndMaxAspectRatiosAreSimilar()).toBe(true);
    expect(settings.getMinDimensionsToMatchAspectRatio())
        .toEqual({width: 320, height: 180});

    settings.minWidth = 0;
    settings.minHeight = 0;
    expect(settings.minAndMaxAspectRatiosAreSimilar()).toBe(false);
    expect(settings.getMinDimensionsToMatchAspectRatio())
        .toEqual({width: 16, height: 9});
    settings.minWidth = 1;
    settings.minHeight = 1;
    expect(settings.minAndMaxAspectRatiosAreSimilar()).toBe(false);
    expect(settings.getMinDimensionsToMatchAspectRatio())
        .toEqual({width: 16, height: 9});
    settings.minWidth = 16;
    settings.minHeight = 9;
    expect(settings.minAndMaxAspectRatiosAreSimilar()).toBe(true);

    settings.minWidth = 320;
    settings.minHeight = 240;
    expect(settings.minAndMaxAspectRatiosAreSimilar()).toBe(false);
    expect(settings.getMinDimensionsToMatchAspectRatio())
        .toEqual({width: 427, height: 240});

    settings.maxWidth = 1000;
    settings.maxHeight = 1000;
    settings.minWidth = 48;
    settings.minHeight = 27;
    expect(settings.minAndMaxAspectRatiosAreSimilar()).toBe(false);
    expect(settings.getMinDimensionsToMatchAspectRatio())
        .toEqual({width: 48, height: 48});

    settings.maxWidth = 1001;
    settings.maxHeight = 999;
    settings.minWidth = 0;
    settings.minHeight = 0;
    expect(settings.getMinDimensionsToMatchAspectRatio())
        .toEqual({width: 1001, height: 999});
  });

  it('is frozen after final adjustments are made', function() {
    'use strict';
    const settings = new mr.mirror.Settings();
    expect(Object.isFrozen(settings)).toBe(false);
    settings.makeFinalAdjustmentsAndFreeze();
    expect(Object.isFrozen(settings)).toBe(true);
    expect(() => {
      settings.maxWidth = 999;
    }).toThrow();
  });
});
