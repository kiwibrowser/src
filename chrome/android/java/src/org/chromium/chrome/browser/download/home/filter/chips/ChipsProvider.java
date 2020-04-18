// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter.chips;

import java.util.List;

/** A source of Chips meant to be visually represented by a {@link ChipCoordinator}. */
public interface ChipsProvider {
    /** Interface to be called a Chip's state changes. */
    interface Observer {
        /** Called whenever the list of Chips changes. */
        void onChipChanged(int position, Chip chip);
    }

    /** Adds an {@link Observer} to be notified of Chip state changes. */
    void addObserver(Observer observer);

    /** Removes an {@link Observer} to be notified of Chip state changes. */
    void removeObserver(Observer observer);

    /** @return A list of {@link Chip} objects. */
    List<Chip> getChips();
}