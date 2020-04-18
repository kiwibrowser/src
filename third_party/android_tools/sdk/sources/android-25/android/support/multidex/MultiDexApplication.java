/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.support.multidex;

import android.app.Application;
import android.content.Context;

/**
 * Minimal MultiDex capable application. To use the legacy multidex library there is 3 possibility:
 * <ul>
 * <li>Declare this class as the application in your AndroidManifest.xml.</li>
 * <li>Have your {@link Application} extends this class.</li>
 * <li>Have your {@link Application} override attachBaseContext starting with<br>
 * <code>
  protected void attachBaseContext(Context base) {<br>
    super.attachBaseContext(base);<br>
    MultiDex.install(this);
    </code></li>
 *   <ul>
 */
public class MultiDexApplication extends Application {
  @Override
  protected void attachBaseContext(Context base) {
    super.attachBaseContext(base);
    MultiDex.install(this);
  }
}
