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

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import java.lang.System;
import java.util.concurrent.ConcurrentHashMap;

// This class manages the database for storing the session data.
//
class BordeauxSessionStorage {

    private static final String TAG = "BordeauxSessionStorage";
    // unique key for the session
    public static final String COLUMN_KEY = "key";
    // name of the learning class
    public static final String COLUMN_CLASS = "class";
    // data of the learning model
    public static final String COLUMN_MODEL = "model";
    // last update time
    public static final String COLUMN_TIME = "time";

    private static final String DATABASE_NAME = "bordeaux";
    private static final String SESSION_TABLE = "sessions";
    private static final int DATABASE_VERSION = 1;
    private static final String DATABASE_CREATE =
        "create table " + SESSION_TABLE + "( " + COLUMN_KEY +
        " TEXT primary key, " + COLUMN_CLASS + " TEXT, " +
        COLUMN_MODEL + " BLOB, " + COLUMN_TIME + " INTEGER);";

    private SessionDBHelper mDbHelper;
    private SQLiteDatabase mDbSessions;

    BordeauxSessionStorage(final Context context) {
        try {
            mDbHelper = new SessionDBHelper(context);
            mDbSessions = mDbHelper.getWritableDatabase();
        } catch (SQLException e) {
            throw new RuntimeException("Can't open session database");
        }
    }

    private class SessionDBHelper extends SQLiteOpenHelper {
        SessionDBHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL(DATABASE_CREATE);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.w(TAG, "Upgrading database from version " + oldVersion + " to "
                  + newVersion + ", which will destroy all old data");

            db.execSQL("DROP TABLE IF EXISTS " + SESSION_TABLE);
            onCreate(db);
        }
    }

    private ContentValues createSessionEntry(String key, Class learner, byte[] model) {
        ContentValues entry = new ContentValues();
        entry.put(COLUMN_KEY, key);
        entry.put(COLUMN_TIME, System.currentTimeMillis());
        entry.put(COLUMN_MODEL, model);
        entry.put(COLUMN_CLASS, learner.getName());
        return entry;
    }

    boolean saveSession(String key, Class learner, byte[] model) {
        ContentValues content = createSessionEntry(key, learner, model);
        long rowID =
                mDbSessions.insertWithOnConflict(SESSION_TABLE, null, content,
                                                 SQLiteDatabase.CONFLICT_REPLACE);
        return rowID >= 0;
    }

    private BordeauxSessionManager.Session getSessionFromCursor(Cursor cursor) {
        BordeauxSessionManager.Session session = new BordeauxSessionManager.Session();
        String className = cursor.getString(cursor.getColumnIndex(COLUMN_CLASS));
        try {
            session.learnerClass = Class.forName(className);
            session.learner = (IBordeauxLearner) session.learnerClass.getConstructor().newInstance();
        } catch (Exception e) {
            throw new RuntimeException("Can't instantiate class: " + className);
        }
        byte[] model = cursor.getBlob(cursor.getColumnIndex(COLUMN_MODEL));
        session.learner.setModel(model);
        return session;
    }

    BordeauxSessionManager.Session getSession(String key) {
        Cursor cursor = mDbSessions.query(true, SESSION_TABLE,
                new String[]{COLUMN_KEY, COLUMN_CLASS, COLUMN_MODEL, COLUMN_TIME},
                COLUMN_KEY + "=\"" + key + "\"", null, null, null, null, null);
        if ((cursor == null) | (cursor.getCount() == 0)) {
            cursor.close();
            return null;
        }
        if (cursor.getCount() > 1) {
            cursor.close();
            throw new RuntimeException("Unexpected duplication in session table for key:" + key);
        }
        cursor.moveToFirst();
        BordeauxSessionManager.Session s = getSessionFromCursor(cursor);
        cursor.close();
        return s;
    }

    void getAllSessions(ConcurrentHashMap<String, BordeauxSessionManager.Session> sessions) {
        Cursor cursor = mDbSessions.rawQuery("select * from ?;", new String[]{SESSION_TABLE});
        if (cursor == null) return;
        cursor.moveToFirst();
        do {
            String key = cursor.getString(cursor.getColumnIndex(COLUMN_KEY));
            BordeauxSessionManager.Session session = getSessionFromCursor(cursor);
            sessions.put(key, session);
        } while (cursor.moveToNext());
    }

    // remove all sessions that have the key that matches the given sql regular
    // expression.
    int removeSessions(String reKey) {
        int nDeleteRows = mDbSessions.delete(SESSION_TABLE, "? like \"?\"",
                                             new String[]{COLUMN_KEY, reKey});
        Log.i(TAG, "Number of rows in session table deleted: " + nDeleteRows);
        return nDeleteRows;
    }
}
