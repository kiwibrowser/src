// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.tabmodel.document;

import org.chromium.chrome.browser.tabmodel.TabCreatorManager;

/** Mocks out calls to the TabCreatorManager and TabCreators. */
public class MockDocumentTabCreatorManager implements TabCreatorManager {
    MockTabDelegate mRegularTabCreator = new MockTabDelegate(false);
    MockTabDelegate mIncognitoTabCreator = new MockTabDelegate(true);

    @Override
    public MockTabDelegate getTabCreator(boolean incognito) {
        return incognito ? mIncognitoTabCreator : mRegularTabCreator;
    }
}