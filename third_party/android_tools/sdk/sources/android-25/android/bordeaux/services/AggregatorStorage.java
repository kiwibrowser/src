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

import android.content.Context;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

// Base Helper class for aggregator storage database
class AggregatorStorage {
    private static final String TAG = "AggregatorStorage";
    private static final String DATABASE_NAME = "aggregator";
    private static final int DATABASE_VERSION = 1;

    protected DBHelper mDbHelper;
    protected SQLiteDatabase mDatabase;

    class DBHelper extends SQLiteOpenHelper {
        private String mTableCmd;
        private String mTableName;
        DBHelper(Context context, String tableName, String tableCmd) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
            mTableName = tableName;
            mTableCmd = tableCmd;
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL(mTableCmd);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.w(TAG, "Upgrading database from version " + oldVersion + " to "
                  + newVersion + ", which will destroy all old data");

            db.execSQL("DROP TABLE IF EXISTS " + mTableName);
            onCreate(db);
        }
    }
}
