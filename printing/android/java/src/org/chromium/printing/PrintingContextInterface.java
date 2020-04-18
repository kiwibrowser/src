// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.printing;

/**
 * Defines an interface for PrintingContext.
 */
public interface PrintingContextInterface {
    /**
     * Updates file descriptor to class instance mapping.
     * @param fileDescriptor The file descriptor to which the current PrintingContext will be
     *                       mapped.
     * @param delete If true, delete the entry (if it exists).  If false, add it to the map.
     */
    void updatePrintingContextMap(int fileDescriptor, boolean delete);

    /**
     * Notifies the native side if the printing settings are successfully prepared.
     * @param success True if the settings are successfully prepared to be used by the native side.
     */
    void askUserForSettingsReply(boolean success);

    /**
     * Notifies the native side that the printing process is completed. This method should be
     * called when the process was initiated by the native side (window.print())
     */
    void showSystemDialogDone();
}
