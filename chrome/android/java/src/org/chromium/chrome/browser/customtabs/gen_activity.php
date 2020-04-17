<?php
for ($i = 10; $i < 40; $i++)
{

$content = '// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

/**
 * Instance class of SeparateTaskManagedCustomTabActivity to simulate the Document-centric behavior
 * in Android L+
 */
public class SeparateTaskCustomTabActivity'.$i.' extends SeparateTaskManagedCustomTabActivity {

}';

file_put_contents('SeparateTaskCustomTabActivity'.$i.'.java', $content);
echo '   "java/src/org/chromium/chrome/browser/customtabs/SeparateTaskCustomTabActivity'.$i.'.java",' . "\n";
}
