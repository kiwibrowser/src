/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.bordeaux.services;

import android.bordeaux.services.IBordeauxLearner.ModelChangeCallback;
import android.content.Context;
import android.os.IBinder;
import android.util.Log;

import java.lang.NoSuchMethodException;
import java.lang.InstantiationException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

// This class manages the learning sessions from multiple applications.
// The learning sessions are automatically backed up to the storage.
//
class BordeauxSessionManager {

    static private final String TAG = "BordeauxSessionManager";
    private BordeauxSessionStorage mSessionStorage;

    static class Session {
        Class learnerClass;
        IBordeauxLearner learner;
        boolean modified = false;
    };

    static class SessionKey {
        String value;
    };

    // Thread to periodically save the sessions to storage
    class PeriodicSave extends Thread implements Runnable {
        long mSavingInterval = 60000; // 60 seconds
        boolean mQuit = false;
        PeriodicSave() {}
        public void run() {
            while (!mQuit) {
                try {
                    sleep(mSavingInterval);
                } catch (InterruptedException e) {
                    // thread waked up.
                    // ignore
                }
                saveSessions();
            }
        }
    }

    PeriodicSave mSavingThread = new PeriodicSave();

    private ConcurrentHashMap<String, Session> mSessions =
            new ConcurrentHashMap<String, Session>();

    public BordeauxSessionManager(final Context context) {
        mSessionStorage = new BordeauxSessionStorage(context);
        mSavingThread.start();
    }

    class LearningUpdateCallback implements ModelChangeCallback {
        private String mKey;

        public LearningUpdateCallback(String key) {
            mKey = key;
        }

        public void modelChanged(IBordeauxLearner learner) {
            // Save the session
            Session session = mSessions.get(mKey);
            if (session != null) {
                synchronized(session) {
                    if (session.learner != learner) {
                        throw new RuntimeException("Session data corrupted!");
                    }
                    session.modified = true;
                }
            }
        }
    }

    // internal unique key that identifies the learning instance.
    // Composed by the package id of the calling process, learning class name
    // and user specified name.
    public SessionKey getSessionKey(String callingUid, Class learnerClass, String name) {
        SessionKey key = new SessionKey();
        key.value = callingUid + "#" + "_" + name + "_" + learnerClass.getName();
        return key;
    }

    public IBinder getSessionBinder(Class learnerClass, SessionKey key) {
        if (mSessions.containsKey(key.value)) {
            return mSessions.get(key.value).learner.getBinder();
        }
        // not in memory cache
        try {
            // try to find it in the database
            Session stored = mSessionStorage.getSession(key.value);
            if (stored != null) {
                // set the callback, so that we can save the state
                stored.learner.setModelChangeCallback(new LearningUpdateCallback(key.value));
                // found session in the storage, put in the cache
                mSessions.put(key.value, stored);
                return stored.learner.getBinder();
            }

            // if session is not already stored, create a new one.
            Log.i(TAG, "create a new learning session: " + key.value);
            IBordeauxLearner learner =
                    (IBordeauxLearner) learnerClass.getConstructor().newInstance();
            // set the callback, so that we can save the state
            learner.setModelChangeCallback(new LearningUpdateCallback(key.value));
            Session session = new Session();
            session.learnerClass = learnerClass;
            session.learner = learner;
            mSessions.put(key.value, session);
            return learner.getBinder();
        } catch (Exception e) {
            throw new RuntimeException("Can't instantiate class: " +
                                       learnerClass.getName());
        }
    }

    public void saveSessions() {
        for (Map.Entry<String, Session> session : mSessions.entrySet()) {
            synchronized(session) {
                // Save the session if it's modified.
                if (session.getValue().modified) {
                    SessionKey skey = new SessionKey();
                    skey.value = session.getKey();
                    saveSession(skey);
                }
            }
        }
    }

    public boolean saveSession(SessionKey key) {
        Session session = mSessions.get(key.value);
        if (session != null) {
            synchronized(session) {
                byte[] model = session.learner.getModel();

                // write to database
                boolean res = mSessionStorage.saveSession(key.value, session.learnerClass, model);
                if (res)
                    session.modified = false;
                else {
                    Log.e(TAG, "Can't save session: " + key.value);
                }
                return res;
            }
        }
        Log.e(TAG, "Session not found: " + key.value);
        return false;
    }

    // Load all session data into memory.
    // The session data will be loaded into the memory from the database, even
    // if this method is not called.
    public void loadSessions() {
        synchronized(mSessions) {
            mSessionStorage.getAllSessions(mSessions);
            for (Map.Entry<String, Session> session : mSessions.entrySet()) {
                // set the callback, so that we can save the state
                session.getValue().learner.setModelChangeCallback(
                        new LearningUpdateCallback(session.getKey()));
            }
        }
    }

    public void removeAllSessionsFromCaller(String callingUid) {
        // remove in the hash table
        ArrayList<String> remove_keys = new ArrayList<String>();
        for (Map.Entry<String, Session> session : mSessions.entrySet()) {
            if (session.getKey().startsWith(callingUid + "#")) {
                remove_keys.add(session.getKey());
            }
        }
        for (String key : remove_keys) {
            mSessions.remove(key);
        }
        // remove all session data from the callingUid in database
        // % is used as wild match for the rest of the string in sql
        int nDeleted = mSessionStorage.removeSessions(callingUid + "#%");
        if (nDeleted > 0)
            Log.i(TAG, "Successfully deleted " + nDeleted + "sessions");
    }
}
