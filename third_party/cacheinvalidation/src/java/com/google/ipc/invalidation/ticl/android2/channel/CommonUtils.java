/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ipc.invalidation.ticl.android2.channel;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

/** Common utility functions for the channel. */
public class CommonUtils {
  /** Returns the package version for the given package name or -1 if the package is not found. */
  static int getPackageVersion(Context context, String packageName) {
    int versionCode = -1;
    PackageManager pm = context.getPackageManager();
    try {
      PackageInfo packageInfo = pm.getPackageInfo(packageName, 0);
      if (packageInfo != null) {
        versionCode = packageInfo.versionCode;
      }
    } catch (PackageManager.NameNotFoundException e) {
      // Do nothing, versionCode stays -1
    }
    return versionCode;
  }
}

