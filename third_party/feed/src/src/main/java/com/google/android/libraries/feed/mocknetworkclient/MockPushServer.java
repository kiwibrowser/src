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

package com.google.android.libraries.feed.mocknetworkclient;

import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.mockserver.MockServerProto.MockServer;
import com.google.search.now.wire.feed.mockserver.MockServerProto.MockUpdate;
import java.util.List;

/** This is a mock server client which will push responses through the session manager. */
public class MockPushServer {
  private static final String TAG = "MockPushServer";

  private final SessionManager sessionManager;
  private final ProtocolAdapter protocolAdapter;
  @VisibleForTesting final MockServerLooper looper;
  @VisibleForTesting boolean hasQuit = false;

  @SuppressWarnings("nullness")
  public MockPushServer(
      MockServer mockServer, SessionManager sessionManager, ProtocolAdapter protocolAdapter) {
    this.sessionManager = sessionManager;
    this.protocolAdapter = protocolAdapter;
    this.looper = new MockServerLooper();
    Handler handler = looper.getHandler();
    // This triggers a nullness warning, suppressing above.  This should be ok since this is a mock
    // class.
    for (MockUpdate update : mockServer.getMockUpdatesList()) {
      handler.postDelayed(() -> handleUpdate(update.getResponse()), update.getUpdateTriggerTime());
    }
  }

  /** Called to shutdown the Looper providing support for Mock Updates */
  public void quitSafely() {
    looper.getHandler().removeCallbacksAndMessages(null);
    looper.quitSafely();
    hasQuit = true;
  }

  @VisibleForTesting
  void handleUpdate(Response response) {
    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    if (!results.isSuccessful()) {
      Logger.e(TAG, "Unable to parse the response for push update, ignoring");
      return;
    }
    sessionManager
        .getUpdateConsumer(MutationContext.EMPTY_CONTEXT)
        .accept(Result.success(results.getValue()));
  }

  @VisibleForTesting
  static class MockServerLooper {
    private final HandlerThread updateThread;
    private final Handler handler;

    private MockServerLooper() {
      Logger.i(TAG, "creating MockServerLooperImpl");
      updateThread = new HandlerThread("UpdateThread");
      updateThread.start();
      handler = new Handler(updateThread.getLooper());
    }

    Handler getHandler() {
      return handler;
    }

    void quitSafely() {
      Logger.i(TAG, "Looper quitSafely");
      if (VERSION.SDK_INT >= VERSION_CODES.JELLY_BEAN_MR2) {
        updateThread.quitSafely();
      } else {
        updateThread.quit();
      }
    }
  }
}
