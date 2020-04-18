// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.app;

import org.chromium.base.process_launcher.ChildProcessService;

/**
 * Implementation of ChildProcessService that uses the content specific delegate.
 * The [Sandboxed|Privileged]ProcessService0, 1.. etc classes are the subclasses for sandboxed/non
 * sandboxed child processes.
 * The embedding application must declare these service instances in the application section
 * of its AndroidManifest.xml, first with some meta-data describing the services:
 *     <meta-data android:name="org.chromium.content.browser.NUM_[SANDBOXED|PRIVILEGED]_SERVICES"
 *           android:value="N"/>
 *     <meta-data android:name="org.chromium.content.browser.[SANDBOXED|PRIVILEGED]_SERVICES_NAME"
 *           android:value="org.chromium.content.app.[Sandboxed|Privileged]ProcessService"/>
 * and then N entries of the form:
 *     <service android:name="org.chromium.content.app.[Sandboxed|Privileged]ProcessServiceX"
 *              android:process=":[sandboxed|privileged]_processX" />
 */
public class ContentChildProcessService extends ChildProcessService {
    public ContentChildProcessService() {
        super(new ContentChildProcessServiceDelegate());
    }
}
