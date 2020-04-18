// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common.functional;

/**
 * Interface which externalizes the action of committing a mutation from the class implementing the
 * mutation. This is a common design pattern used to mutate the library components. The mutation
 * builder will call a {@code Committer} to commit the accumulated changes.
 *
 * @param <R> The return value
 * @param <C> A class containing the changes which will be committed.
 */
public interface Committer<R, C> {
  /** Commit the change T, and optionally return a value R */
  R commit(C change);
}
