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

package com.google.android.libraries.feed.host.network;

import com.google.android.libraries.feed.common.functional.Consumer;

/**
 * An object that can send an {@link HttpRequest} and receive an {@link HttpResponse} in response.
 */
public interface NetworkClient extends AutoCloseable {

  /**
   * Sends the HttpRequest. Upon completion, asynchronously calls the consumer with the
   * HttpResponse.
   *
   * <p>Requests and responses should be uncompressed when in the Feed. It is up to the host to gzip
   * / ungzip any requests or responses.
   *
   * @param request The request to send
   * @param responseConsumer The callback to be used when the response comes back.
   */
  void send(HttpRequest request, Consumer<HttpResponse> responseConsumer);
}
