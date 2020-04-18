/*
 * Copyright (C) 2007 The Android Open Source Project
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

package org.apache.harmony.dalvik.ddmc;

import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;


/**
 * This represents our connection to the DDM Server.
 */
public class DdmServer {

    public static final int CLIENT_PROTOCOL_VERSION = 1;

    private static HashMap<Integer,ChunkHandler> mHandlerMap =
        new HashMap<Integer,ChunkHandler>();

    private static final int CONNECTED = 1;
    private static final int DISCONNECTED = 2;

    private static volatile boolean mRegistrationComplete = false;
    private static boolean mRegistrationTimedOut = false;


    /**
     * Don't instantiate; all members and methods are static.
     */
    private DdmServer() {}

    /**
     * Register an instance of the ChunkHandler class to handle a specific
     * chunk type.
     *
     * Throws an exception if the type already has a handler registered.
     */
    public static void registerHandler(int type, ChunkHandler handler) {
        if (handler == null) {
            throw new NullPointerException("handler == null");
        }
        synchronized (mHandlerMap) {
            if (mHandlerMap.get(type) != null)
                throw new RuntimeException("type " + Integer.toHexString(type)
                    + " already registered");

            mHandlerMap.put(type, handler);
        }
    }

    /**
     * Unregister the existing handler for the specified type.
     *
     * Returns the old handler.
     */
    public static ChunkHandler unregisterHandler(int type) {
        synchronized (mHandlerMap) {
            return mHandlerMap.remove(type);
        }
    }

    /**
     * The application must call here after it finishes registering
     * handlers.
     */
    public static void registrationComplete() {
        // sync on mHandlerMap because it's convenient and makes a kind of
        // sense
        synchronized (mHandlerMap) {
            mRegistrationComplete = true;
            mHandlerMap.notifyAll();
        }
    }

    /**
     * Send a chunk of data to the DDM server.  This takes the form of a
     * JDWP "event", which does not elicit a response from the server.
     *
     * Use this for "unsolicited" chunks.
     */
    public static void sendChunk(Chunk chunk) {
        nativeSendChunk(chunk.type, chunk.data, chunk.offset, chunk.length);
    }

    /* send a chunk to the DDM server */
    native private static void nativeSendChunk(int type, byte[] data,
        int offset, int length);

    /*
     * Called by the VM when the DDM server connects or disconnects.
     */
    private static void broadcast(int event)
    {
        synchronized (mHandlerMap) {
            Collection values = mHandlerMap.values();
            Iterator iter = values.iterator();

            while (iter.hasNext()) {
                ChunkHandler handler = (ChunkHandler) iter.next();
                switch (event) {
                    case CONNECTED:
                        handler.connected();
                        break;
                    case DISCONNECTED:
                        handler.disconnected();
                        break;
                    default:
                        throw new UnsupportedOperationException();
                }
            }
        }
    }

    /*
     * This is called by the VM when a chunk arrives.
     *
     * For a DDM-aware application, we want to wait until the app has had
     * a chance to register all of its chunk handlers.  Otherwise, we'll
     * end up dropping early-arriving packets on the floor.
     *
     * For a non-DDM-aware application, we'll end up waiting here forever
     * if DDMS happens to connect.  It's hard to know for sure that
     * registration isn't going to happen, so we settle for a timeout.
     */
    private static Chunk dispatch(int type, byte[] data, int offset, int length)
    {
        ChunkHandler handler;

        synchronized (mHandlerMap) {
            /*
             * If registration hasn't completed, and we haven't timed out
             * waiting for it, wait a bit.
             */
            while (!mRegistrationComplete && !mRegistrationTimedOut) {
                //System.out.println("dispatch() waiting for reg");
                try {
                    mHandlerMap.wait(1000);     // 1.0 sec
                } catch (InterruptedException ie) {
                    continue;
                }

                if (!mRegistrationComplete) {
                    /* timed out, don't wait again */
                    mRegistrationTimedOut = true;
                }
            }

            handler = mHandlerMap.get(type);
        }
        //System.out.println(" dispatch cont");

        if (handler == null) {
            return null;
        }

        Chunk chunk = new Chunk(type, data, offset, length);
        return handler.handleChunk(chunk);
    }
}
