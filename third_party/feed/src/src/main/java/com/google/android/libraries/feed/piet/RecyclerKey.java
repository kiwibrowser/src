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

package com.google.android.libraries.feed.piet;

/**
 * Key for a {@link RecyclerPool}; objects that can be bound to the same model have an equal key.
 * Extend and override hashCode and equals for more granular specification of which objects are
 * compatible for recycling. (ex. {@link TextElementAdapter} is only compatible with other adapters
 * that have the same size, font weight, and italic)
 */
class RecyclerKey {}
