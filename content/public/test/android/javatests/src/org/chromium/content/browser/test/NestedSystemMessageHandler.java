// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Handles processing messages in nested run loops.
 *
 * Android does not support nested run loops by default. While running
 * in nested mode, we use reflection to retreive messages from the MessageQueue
 * and dispatch them.
 */
@JNINamespace("content")
class NestedSystemMessageHandler {
    // See org.chromium.base.SystemMessageHandler for more message ids.
    // The id here should not conflict with the ones in SystemMessageHandler.
    private static final int QUIT_MESSAGE = 10;
    private static final Handler sHandler = new Handler();

    private NestedSystemMessageHandler() {
    }

    /**
     * Processes messages from the current MessageQueue till the queue becomes idle.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private boolean runNestedLoopTillIdle() {
        boolean quitLoop = false;

        MessageQueue queue = Looper.myQueue();
        queue.addIdleHandler(new MessageQueue.IdleHandler() {
            @Override
            public boolean queueIdle() {
                sHandler.sendMessage(sHandler.obtainMessage(QUIT_MESSAGE));
                return false;
            }
        });

        Class<?> messageQueueClazz = queue.getClass();
        Method nextMethod = null;
        try {
            nextMethod = messageQueueClazz.getDeclaredMethod("next");
        } catch (SecurityException e) {
            e.printStackTrace();
            return false;
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
            return false;
        }
        nextMethod.setAccessible(true);

        while (!quitLoop) {
            Message msg = null;
            try {
                msg = (Message) nextMethod.invoke(queue);
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
                return false;
            } catch (IllegalAccessException e) {
                e.printStackTrace();
                return false;
            } catch (InvocationTargetException e) {
                e.printStackTrace();
                return false;
            }

            if (msg != null) {
                if (msg.what == QUIT_MESSAGE) {
                    quitLoop = true;
                }
                Class messageClazz = msg.getClass();
                Field targetFiled = null;
                try {
                    targetFiled = messageClazz.getDeclaredField("target");
                } catch (SecurityException e) {
                    e.printStackTrace();
                    return false;
                } catch (NoSuchFieldException e) {
                    e.printStackTrace();
                    return false;
                }
                targetFiled.setAccessible(true);

                Handler target = null;
                try {
                    target = (Handler) targetFiled.get(msg);
                } catch (IllegalArgumentException e) {
                    e.printStackTrace();
                    return false;
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                    return false;
                }

                if (target == null) {
                    // No target is a magic identifier for the quit message.
                    quitLoop = true;
                } else {
                    target.dispatchMessage(msg);
                }

                // Unset in-use flag.
                Field flagsField = null;
                try {
                    flagsField = messageClazz.getDeclaredField("flags");
                } catch (IllegalArgumentException e) {
                    e.printStackTrace();
                    return false;
                } catch (SecurityException e) {
                    e.printStackTrace();
                    return false;
                } catch (NoSuchFieldException e) {
                    e.printStackTrace();
                    return false;
                }
                flagsField.setAccessible(true);

                try {
                    Integer oldFlags = (Integer) flagsField.get(msg);
                    flagsField.set(msg, oldFlags & ~(1 << 0 /* FLAG_IN_USE */));
                } catch (IllegalArgumentException e) {
                    e.printStackTrace();
                    return false;
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                    return false;
                }

                msg.recycle();
            } else {
                quitLoop = true;
            }
        }
        return true;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private static NestedSystemMessageHandler create() {
        return new NestedSystemMessageHandler();
    }
}
