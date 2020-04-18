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
package com.google.ipc.invalidation.external.client.android2;

import com.google.ipc.invalidation.ticl.InvalidationClientCore;
import com.google.ipc.invalidation.ticl.android2.AndroidTiclManifest;
import com.google.ipc.invalidation.ticl.android2.ProtocolIntents;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;
import com.google.ipc.invalidation.util.Bytes;

import android.content.Context;
import android.content.Intent;

/**
 * Factory for creating  Android clients.
 *
 */
public final class AndroidClientFactory {
  /**
   * Creates a new client.
   * <p>
   * REQUIRES: no client exist, or a client exists with the same type and name as provided. In
   * the latter case, this call is a no-op.
   *
   * @param context Android system context
   * @param clientType type of the client to create
   * @param clientName name of the client to create
   */
  public static void createClient(Context context, int clientType, byte[] clientName) {
    ClientConfigP config = InvalidationClientCore.createConfig();
    Intent intent = ProtocolIntents.InternalDowncalls.newCreateClientIntent(
        clientType, Bytes.fromByteArray(clientName), config, false);
    intent.setClassName(context, new AndroidTiclManifest(context).getTiclServiceClass());
    context.startService(intent);
  }

  private AndroidClientFactory() {
    // Disallow instantiation.
  }
}
