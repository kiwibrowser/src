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

package com.google.android.libraries.feed.host.stream;

/** Interface which is able to provide host configuration for default stream look and feel. */
public interface StreamConfiguration {

  /**
   * Returns the padding (in px) that appears to the start (left in LtR)) of each view in the
   * Stream.
   */
  int getPaddingStart();

  /**
   * Returns the padding (in px) that appears to the end (right in LtR)) of each view in the Stream.
   */
  int getPaddingEnd();

  /** Returns the padding (in px) that appears before the first view in the Stream. */
  int getPaddingTop();

  /** Returns the padding (in px) that appears after the last view in the Stream. */
  int getPaddingBottom();
}
