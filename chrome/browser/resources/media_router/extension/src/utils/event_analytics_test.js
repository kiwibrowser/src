// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.Analytics');
goog.require('mr.EventAnalytics');

describe('Tests EventAnalytics', () => {

  beforeEach(() => {
    mr.EventAnalytics.firstEvent_ = undefined;
  });

  describe('Test recordEvent', () => {
    it('should record only the first event', () => {
      spyOn(mr.Analytics, 'recordEnum');
      mr.EventAnalytics.recordEvent(mr.EventAnalytics.Event.DIAL_ON_ERROR);
      mr.EventAnalytics.recordEvent(mr.EventAnalytics.Event.TABS_ON_UPDATED);
      expect(mr.Analytics.recordEnum.calls.count()).toEqual(1);
      expect(mr.Analytics.recordEnum)
          .toHaveBeenCalledWith(
              'MediaRouter.Provider.WakeEvent',
              mr.EventAnalytics.Event.DIAL_ON_ERROR, mr.EventAnalytics.Event);
    });
  });
});
