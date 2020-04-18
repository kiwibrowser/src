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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/* This class implements record like data storage for aggregator.
 * The data is stored in the sqlite database row by row without primary key, all
 * columns are assume having string value.
 * Sample usage:
 *       AggregatorRecordStorage db = new AggregatorRecordStorage(this, "TestTable",
 *               new String[]{"clusterid", "long", "lat"});
 *       db.removeAllData();
 *       HashMap<String, String> row = new HashMap<String, String>();
 *       row.put("clusterid", "home");
 *       row.put("long", "110.203");
 *       row.put("lat", "-13.787");
 *       db.addData(row);
 *       row.put("clusterid", "office");
 *       row.put("long", "1.203");
 *       row.put("lat", "33.787");
 *       db.addData(row);
 *       List<Map<String,String> > allData = db.getAllData();
 *       Log.i(TAG,"Total data in database: " + allData.size());
 */
class AggregatorRecordStorage extends AggregatorStorage {
    private static final String TAG = "AggregatorRecordStorage";
    private String mTableName;
    private List<String> mColumnNames;

    public AggregatorRecordStorage(Context context, String tableName, String [] columnNames) {
        if (columnNames.length < 1) {
            throw new RuntimeException("No column keys");
        }
        mColumnNames = Arrays.asList(columnNames);
        mTableName = tableName;

        String tableCmd = "create table " + tableName + "( " + columnNames[0] +
          " TEXT";
        for (int i = 1; i < columnNames.length; ++i)
            tableCmd = tableCmd + ", " + columnNames[i] + " TEXT";
        tableCmd = tableCmd + ");";
        Log.i(TAG, tableCmd);
        try {
            mDbHelper = new DBHelper(context, tableName, tableCmd);
            mDatabase = mDbHelper.getWritableDatabase();
        } catch (SQLException e) {
            throw new RuntimeException("Can't open table: " + tableName);
        }
    }

    // Adding one more row to the table.
    // the data is a map of <column_name, value> pair.
    public boolean addData(Map<String, String> data) {
        ContentValues content = new ContentValues();
        for (Map.Entry<String, String> item : data.entrySet()) {
            content.put(item.getKey(), item.getValue());
        }
        long rowID =
                mDatabase.insert(mTableName, null, content);
        return rowID >= 0;
    }

    // Return all data as a list of Map.
    // Notice that the column names are repeated for each row.
    public List<Map<String, String>> getAllData() {
        ArrayList<Map<String, String> > allData = new ArrayList<Map<String, String> >();

        Cursor cursor = mDatabase.rawQuery("select * from " + mTableName + ";", null);
        if (cursor.getCount() == 0) {
            return allData;
        }
        cursor.moveToFirst();
        do {
            HashMap<String, String> oneRow = new HashMap<String, String>();
            for (String column : mColumnNames) {
                int columnIndex = cursor.getColumnIndex(column);
                if (!cursor.isNull(columnIndex)) {
                    String value = cursor.getString(columnIndex);
                    oneRow.put(column, value);
                }
            }
            allData.add(oneRow);
        } while (cursor.moveToNext());
        return allData;
    }

    // Empty the storage.
    public int removeAllData() {
        int nDeleteRows = mDatabase.delete(mTableName, "1", null);
        Log.i(TAG, "Number of rows in table deleted: " + nDeleteRows);
        return nDeleteRows;
    }
}
