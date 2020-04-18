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

package com.google.android.libraries.feed.basicstream.internal.viewholders;

import android.support.annotation.IntDef;

/**
 * Constants to specify the type of ViewHolder to create in the {@link StreamRecyclerViewAdapter}.
 */
@IntDef({ViewHolderType.TYPE_HEADER, ViewHolderType.TYPE_CARD, ViewHolderType.TYPE_CONTINUATION})
public @interface ViewHolderType {
  int TYPE_HEADER = 0;
  int TYPE_CARD = 1;
  int TYPE_CONTINUATION = 2;
}
