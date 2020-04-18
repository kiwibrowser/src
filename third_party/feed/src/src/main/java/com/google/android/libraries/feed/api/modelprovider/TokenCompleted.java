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

package com.google.android.libraries.feed.api.modelprovider;

/** Defines completion of a continuation token initiated request. */
public class TokenCompleted {
  private final ModelCursor modelCursor;

  public TokenCompleted(ModelCursor modelCursor) {
    this.modelCursor = modelCursor;
  }

  /**
   * Returns a cursor representing the continuation from the point in the stream the continuation
   * token was found.
   */
  public ModelCursor getCursor() {
    return modelCursor;
  }
}
