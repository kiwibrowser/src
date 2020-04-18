/*
 * Copyright (C) 2015 The Android Open Source Project
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
package android.databinding.testapp.vo;

import android.content.Context;
import android.databinding.BaseObservable;
import android.databinding.ObservableBoolean;
import android.graphics.Outline;
import android.media.MediaPlayer;
import android.text.Editable;
import android.view.ContextMenu;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewStub;
import android.view.WindowInsets;
import android.view.animation.Animation;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.CalendarView;
import android.widget.Chronometer;
import android.widget.CompoundButton;
import android.widget.ExpandableListView;
import android.widget.NumberPicker;
import android.widget.RadioGroup;
import android.widget.RatingBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.TimePicker;

public class ListenerBindingObject {
    public static int lastClick = 0;
    public boolean inflateCalled;
    private final Context mContext;
    public boolean wasRunnableRun;

    public final ObservableBoolean clickable = new ObservableBoolean();
    public final ObservableBoolean useOne = new ObservableBoolean();

    public ListenerBindingObject(Context context) {
        clickable.set(true);
        this.mContext = context;
    }

    public void onMovedToScrapHeap(View view) { }

    public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
            int totalItemCount) {}

    public void onScrollStateChanged(AbsListView view, int scrollState) { }

    public boolean onMenuItemClick(MenuItem item) {
        return false;
    }

    public void onItemClick(AdapterView<?> parent, View view, int position, long id) { }

    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        return true;
    }

    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) { }

    public void onNothingSelected(AdapterView<?> parent) { }

    public void onDismiss() { }

    public CharSequence fixText(CharSequence invalidText) {
        return invalidText;
    }

    public boolean isValid(CharSequence text) {
        return true;
    }

    public void onSelectedDayChange(CalendarView view, int year, int month, int dayOfMonth) { }

    public void onChronometerTick(Chronometer chronometer) { }

    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) { }

    public boolean onChildClick(ExpandableListView parent, View v, int groupPosition,
            int childPosition, long id) {
        return false;
    }

    public boolean onGroupClick(ExpandableListView parent, View v, int groupPosition, long id) {
        return false;
    }

    public void onGroupCollapse(int groupPosition) { }

    public void onGroupExpand(int groupPosition) { }

    public String format(int value) {
        return null;
    }

    public void onValueChange(NumberPicker picker, int oldVal, int newVal) { }

    public void onScrollStateChange(NumberPicker view, int scrollState) { }

    public void onCheckedChanged(RadioGroup group, int checkedId) { }

    public void onRatingChanged(RatingBar ratingBar, float rating, boolean fromUser) { }

    public boolean onClose() {
        return false;
    }

    public boolean onQueryTextChange(String newText) {
        return false;
    }

    public boolean onQueryTextSubmit(String query) {
        return false;
    }

    public boolean onSuggestionClick(int position) {
        return false;
    }

    public boolean onSuggestionSelect(int position) {
        return false;
    }

    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { }

    public void onStartTrackingTouch(SeekBar seekBar) { }

    public void onStopTrackingTouch(SeekBar seekBar) { }

    public void onTabChanged(String tabId) { }

    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        return false;
    }

    public void afterTextChanged(Editable s) { }

    public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

    public void onTextChanged(CharSequence s, int start, int before, int count) { }

    public void onTimeChanged(TimePicker view, int hourOfDay, int minute) { }

    public void onClick(View view) { }

    public void onCompletion(MediaPlayer mp) { }

    public boolean onError(MediaPlayer mp, int what, int extra) {
        return true;
    }

    public boolean onInfo(MediaPlayer mp, int what, int extra) {
        return true;
    }

    public void onPrepared(MediaPlayer mp) { }

    public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
        return null;
    }

    public void onCreateContextMenu(ContextMenu menu, View v,
            ContextMenu.ContextMenuInfo menuInfo) { }

    public boolean onDrag(View v, DragEvent event) {
        return true;
    }

    public void onFocusChange(View v, boolean hasFocus) { }

    public boolean onGenericMotion(View v, MotionEvent event) {
        return true;
    }

    public boolean onHover(View v, MotionEvent event) {
        return true;
    }

    public boolean onKey(View v, int keyCode, KeyEvent event) {
        return true;
    }

    public boolean onLongClick(View v) {
        return true;
    }

    public void onSystemUiVisibilityChange(int visibility) { }

    public boolean onTouch(View v, MotionEvent event) {
        return true;
    }

    public void getOutline(View view, Outline outline) { }

    public void onViewAttachedToWindow(View v) { }

    public void onViewDetachedFromWindow(View v) { }

    public void onChildViewAdded(View parent, View child) { }

    public void onChildViewRemoved(View parent, View child) { }

    public void onAnimationEnd(Animation animation) { }

    public void onAnimationRepeat(Animation animation) { }

    public void onAnimationStart(Animation animation) { }

    public void onInflate(ViewStub stub, View inflated) {
        inflateCalled = true;
    }

    public View makeView() {
        return new View(mContext);
    }

    public void onClick1(View view) {
        lastClick = 1;
    }

    public static void onClick2(View view) {
        lastClick = 2;
    }

    public void onClick3(View view) {
        lastClick = 3;
    }

    public static void onClick4(View view) {
        lastClick = 4;
    }

    public void runnableRun() {
        this.wasRunnableRun = true;
    }

    public void onFoo() {
    }

    public void onBar() {}

    public boolean onBar(View view) {
        return true;
    }

    public static class Inner extends BaseObservable {
        public boolean clicked;
        public void onClick(View view) {
            clicked = true;
        }
    }
}
