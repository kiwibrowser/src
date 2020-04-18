/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ipc.invalidation.ticl;

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.ticl.proto.ChannelCommon.NetworkEndpointId;

/**
 * Extension of {@link com.google.ipc.invalidation.external.client.SystemResources.NetworkChannel}
 * that adds a method to get the network endpoint id.
 *
 */
public interface TestableNetworkChannel extends SystemResources.NetworkChannel {
  /**
   * Returns the network id for testing. May throw {@link UnsupportedOperationException}.
   */
  NetworkEndpointId getNetworkIdForTest();

}
