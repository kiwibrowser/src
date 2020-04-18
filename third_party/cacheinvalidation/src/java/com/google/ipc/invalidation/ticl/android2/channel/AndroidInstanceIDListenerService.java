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
package com.google.ipc.invalidation.ticl.android2.channel;

import com.google.android.gms.iid.InstanceIDListenerService;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;

/**
 * Implementation of {@link InstanceIDListenerService} to receive notifications from GCM to
 * update the registration token.
 */
public class AndroidInstanceIDListenerService extends InstanceIDListenerService {
  private static final Logger logger = AndroidLogger.forTag("InstanceIDListener");
  
  /**
   * Called when the token needs to updated. {@link AndroidGcmController#fetchToken} clears the
   * current token and schedules a task to fetch a new token.
   */
  @Override
  public void onTokenRefresh() {
    logger.info("Received token refresh request");
    AndroidGcmController.get(this).fetchToken();
  }
}
