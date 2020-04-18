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

package com.google.android.libraries.feed.host.proto;

import com.google.protobuf.GeneratedMessageLite.GeneratedExtension;
import java.util.List;

/** Allows the host application to register proto extensions in the Feed's global registry. */
public interface ProtoExtensionProvider {
  /**
   * The Feed will call this method on startup. Any proto extensions that will need to be serialized
   * by the Feed should be returned at that time.
   *
   * @return a list of the proto extensions the host application will use in the Feed.
   */
  List<GeneratedExtension<?, ?>> getProtoExtensions();
}
