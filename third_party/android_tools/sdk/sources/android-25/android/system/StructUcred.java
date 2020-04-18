/*
 * Copyright (C) 2013 The Android Open Source Project
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
 * Corresponds to C's {@code struct ucred}.
 *
 * @hide
 */
public final class StructUcred {
  /** The peer's process id. */
  public final int pid;

  /** The peer process' uid. */
  public final int uid;

  /** The peer process' gid. */
  public final int gid;

  public StructUcred(int pid, int uid, int gid) {
    this.pid = pid;
    this.uid = uid;
    this.gid = gid;
  }

  @Override public String toString() {
    return Objects.toString(this);
  }
}
