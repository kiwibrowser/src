/*
 * Copyright (C) 2011 The Android Open Source Project
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

package android.system;

import libcore.util.Objects;

/**
 * Information returned by {@link Os#getpwnam} and {@link Os#getpwuid}. Corresponds to C's
 * {@code struct passwd} from {@code <pwd.h>}.
 *
 * @hide
 */
public final class StructPasswd {
  public final String pw_name;
  public final int pw_uid;
  public final int pw_gid;
  public final String pw_dir;
  public final String pw_shell;

  /**
   * Constructs an instance with the given field values.
   */
  public StructPasswd(String pw_name, int pw_uid, int pw_gid, String pw_dir, String pw_shell) {
    this.pw_name = pw_name;
    this.pw_uid = pw_uid;
    this.pw_gid = pw_gid;
    this.pw_dir = pw_dir;
    this.pw_shell = pw_shell;
  }

  @Override public String toString() {
    return Objects.toString(this);
  }
}
