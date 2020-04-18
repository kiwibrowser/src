/*
 * Copyright (C) 2011 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.ex.photo;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.view.Menu;
import android.view.MenuItem;

/**
 * Activity to view the contents of an album.
 */
public class PhotoViewActivity extends FragmentActivity
        implements PhotoViewController.ActivityInterface {

    private PhotoViewController mController;
    private ActionBarWrapper mActionBar;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mController = createController();
        mController.onCreate(savedInstanceState);
    }

    public PhotoViewController createController() {
        return new PhotoViewController(this);
    }

    @Override
    public PhotoViewController getController() {
        return mController;
    }

    @Override
    protected void onStart() {
        super.onStart();
        mController.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mController.onResume();
    }

    @Override
    protected void onPause() {
        mController.onPause();
        super.onPause();
    }

    @Override
    protected void onStop() {
        mController.onStop();
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        mController.onDestroy();
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        if (!mController.onBackPressed()) {
            super.onBackPressed();
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        mController.onSaveInstanceState(outState);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return mController.onCreateOptionsMenu(menu) || super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        return mController.onPrepareOptionsMenu(menu) || super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return mController.onOptionsItemSelected(item) || super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        mController.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public Context getContext() {
        return this;
    }

    @Override
    public ActionBarInterface getActionBarInterface() {
        if (mActionBar == null) {
            mActionBar = new ActionBarWrapper(getActionBar());
        }
        return mActionBar;
    }

}
