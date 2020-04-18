/* Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.vr.keyboard;

/**
 * @hide
 * Provides loading of the GVR keyboard library.
 */
interface IGvrKeyboardLoader {
  /**
   * Attempts to load (dlopen) GVR keyboard library.
   * <p>
   * The library will be loaded only if a matching library can be found
   * that is recent enough. If the library is out-of-date, or cannot be found,
   * 0 will be returned.
   *
   * @param the version of the target library.
   * @return the native handle to the library. 0 if the load fails.
   */
  long loadGvrKeyboard(long version) = 1;

  /**
   * Closes a library (dlclose).
   *
   * @param nativeLibrary the native handle of the library to be closed.
   */
  void closeGvrKeyboard(long nativeLibrary) = 2;
}
