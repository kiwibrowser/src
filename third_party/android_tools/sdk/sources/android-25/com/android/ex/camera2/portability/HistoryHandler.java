/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ex.camera2.portability;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import java.util.LinkedList;

class HistoryHandler extends Handler {
    private static final int MAX_HISTORY_SIZE = 400;

    final LinkedList<Integer> mMsgHistory;

    HistoryHandler(Looper looper) {
        super(looper);
        mMsgHistory = new LinkedList<Integer>();
        // We add a -1 at the beginning to mark the very beginning of the
        // history.
        mMsgHistory.offerLast(-1);
    }

    Integer getCurrentMessage() {
        return mMsgHistory.peekLast();
    }

    String generateHistoryString(int cameraId) {
        String info = new String("HIST");
        info += "_ID" + cameraId;
        for (Integer msg : mMsgHistory) {
            info = info + '_' + msg.toString();
        }
        info += "_HEND";
        return info;
    }

    /**
     * Subclasses' implementations should call this one before doing their work.
     */
    @Override
    public void handleMessage(Message msg) {
        mMsgHistory.offerLast(msg.what);
        while (mMsgHistory.size() > MAX_HISTORY_SIZE) {
            mMsgHistory.pollFirst();
        }
    }
}
