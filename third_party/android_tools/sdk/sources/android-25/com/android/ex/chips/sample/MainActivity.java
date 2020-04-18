/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.ex.chips.sample;

import android.os.Bundle;
import android.text.util.Rfc822Tokenizer;
import android.widget.MultiAutoCompleteTextView;
import android.app.Activity;
import android.util.Log;

import com.android.ex.chips.BaseRecipientAdapter;
import com.android.ex.chips.RecipientEditTextView;
import com.android.ex.chips.RecipientEditTextView.PermissionsRequestItemClickedListener;
import com.android.ex.chips.RecipientEditTextView.RecipientChipAddedListener;
import com.android.ex.chips.RecipientEditTextView.RecipientChipDeletedListener;
import com.android.ex.chips.RecipientEntry;

public class MainActivity extends Activity
    implements PermissionsRequestItemClickedListener, RecipientChipDeletedListener,
        RecipientChipAddedListener {

    private RecipientEditTextView mEmailRetv;
    private RecipientEditTextView mPhoneRetv;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mEmailRetv = (RecipientEditTextView) findViewById(R.id.email_retv);
        mEmailRetv.setTokenizer(new Rfc822Tokenizer());
        final BaseRecipientAdapter emailAdapter = new BaseRecipientAdapter(this);
        emailAdapter.setShowRequestPermissionsItem(true);
        mEmailRetv.setAdapter(emailAdapter);
        mEmailRetv.setPermissionsRequestItemClickedListener(this);


        mPhoneRetv = (RecipientEditTextView) findViewById(R.id.phone_retv);
        mPhoneRetv.setTokenizer(new MultiAutoCompleteTextView.CommaTokenizer());
        final BaseRecipientAdapter phoneAdapter =
                new BaseRecipientAdapter(BaseRecipientAdapter.QUERY_TYPE_PHONE, this);
        phoneAdapter.setShowRequestPermissionsItem(true);
        mPhoneRetv.setAdapter(phoneAdapter);
        mPhoneRetv.setPermissionsRequestItemClickedListener(this);
        mEmailRetv.setRecipientChipAddedListener(this);
        mEmailRetv.setRecipientChipDeletedListener(this);
    }

    @Override
    public void onPermissionsRequestItemClicked(
            RecipientEditTextView view, String[] permissions) {
        requestPermissions(permissions, 0 /* requestCode */);
    }

    @Override
    public void onPermissionRequestDismissed() {
        mEmailRetv.getAdapter().setShowRequestPermissionsItem(false);
        mPhoneRetv.getAdapter().setShowRequestPermissionsItem(false);
    }

    @Override
    public void onRecipientChipAdded(RecipientEntry entry) {
        Log.i("ChipsSample", entry.getDisplayName() + " recipient chip added");
    }

    @Override
    public void onRecipientChipDeleted(RecipientEntry entry) {
        Log.i("ChipsSample", entry.getDisplayName() + " recipient chip removed");
    }
}
