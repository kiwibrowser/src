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

package com.google.android.libraries.feed.api.actionmanager;

import com.google.android.libraries.feed.api.common.DismissActionWithSemanticProperties;
import com.google.android.libraries.feed.common.Result;
import java.util.List;

/** Interface for reading various {@link StreamAction}s. */
public interface ActionReader {

  /** Retrieves list of {@link DismissActionWithSemanticProperties} for all valid dismiss actions */
  Result<List<DismissActionWithSemanticProperties>> getDismissActionsWithSemanticProperties();
}
