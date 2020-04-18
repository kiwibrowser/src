/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.testapp;

import android.databinding.testapp.databinding.LayoutWithIncludeBinding;
import android.databinding.testapp.databinding.MergeContainingMergeBinding;
import android.databinding.testapp.databinding.MergeLayoutBinding;
import android.databinding.testapp.vo.NotBindableVo;

import android.test.UiThreadTest;
import android.widget.FrameLayout;
import android.widget.TextView;

public class IncludeTagTest extends BaseDataBinderTest<LayoutWithIncludeBinding> {

    public IncludeTagTest() {
        super(LayoutWithIncludeBinding.class);
    }

    @UiThreadTest
    public void testIncludeTag() {
        initBinder();
        assertNotNull(mBinder.includedPlainLayout);
        assertTrue(mBinder.includedPlainLayout instanceof FrameLayout);
        NotBindableVo vo = new NotBindableVo(3, "a");
        mBinder.setOuterObject(vo);
        mBinder.executePendingBindings();
        final TextView outerText = (TextView) mBinder.getRoot().findViewById(R.id.outerTextView);
        assertEquals("a", outerText.getText());
        final TextView innerText = (TextView) mBinder.getRoot().findViewById(R.id.innerTextView);
        assertEquals("modified 3a", innerText.getText().toString());
        TextView textView1 = (TextView) mBinder.getRoot().findViewById(R.id.innerTextView1);
        assertEquals(mBinder.getRoot(), textView1.getParent().getParent());
        TextView textView2 = (TextView) mBinder.getRoot().findViewById(R.id.innerTextView2);
        assertEquals(mBinder.getRoot(), textView2.getParent().getParent());
        assertEquals("a hello 3a", textView1.getText().toString());
        assertEquals("b hello 3a", textView2.getText().toString());
        MergeLayoutBinding mergeLayoutBinding = mBinder.secondMerge;
        assertNotSame(textView1, mergeLayoutBinding.innerTextView1);
        assertNotSame(textView2, mergeLayoutBinding.innerTextView2);
        assertEquals("a goodbye 3a", mergeLayoutBinding.innerTextView1.getText().toString());
        assertEquals("b goodbye 3a", mergeLayoutBinding.innerTextView2.getText().toString());
        MergeContainingMergeBinding mergeContainingMergeBinding = mBinder.thirdMerge;
        MergeLayoutBinding merge1 = mergeContainingMergeBinding.merge1;
        MergeLayoutBinding merge2 = mergeContainingMergeBinding.merge2;
        assertEquals("a 1 third 3a", merge1.innerTextView1.getText().toString());
        assertEquals("b 1 third 3a", merge1.innerTextView2.getText().toString());
        assertEquals("a 2 third 3a", merge2.innerTextView1.getText().toString());
        assertEquals("b 2 third 3a", merge2.innerTextView2.getText().toString());

        vo.setIntValue(5);
        vo.setStringValue("b");
        mBinder.invalidateAll();
        mBinder.executePendingBindings();
        assertEquals("b", outerText.getText());
        assertEquals("modified 5b", innerText.getText().toString());
        assertEquals("a hello 5b", textView1.getText().toString());
        assertEquals("b hello 5b", textView2.getText().toString());
        assertEquals("a goodbye 5b", mergeLayoutBinding.innerTextView1.getText().toString());
        assertEquals("b goodbye 5b", mergeLayoutBinding.innerTextView2.getText().toString());
        assertEquals("a 1 third 5b", merge1.innerTextView1.getText().toString());
        assertEquals("b 1 third 5b", merge1.innerTextView2.getText().toString());
        assertEquals("a 2 third 5b", merge2.innerTextView1.getText().toString());
        assertEquals("b 2 third 5b", merge2.innerTextView2.getText().toString());
    }
}
