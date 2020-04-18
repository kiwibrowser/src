/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.ex.editstyledtext;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;

import com.android.ex.editstyledtext.EditStyledText.EditModeActions.EditModeActionBase;
import com.android.ex.editstyledtext.EditStyledText.EditStyledTextSpans.HorizontalLineSpan;
import com.android.ex.editstyledtext.EditStyledText.EditStyledTextSpans.MarqueeSpan;
import com.android.ex.editstyledtext.EditStyledText.EditStyledTextSpans.RescalableImageSpan;

import android.R;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RectShape;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.ResultReceiver;
import android.text.ClipboardManager;
import android.text.Editable;
import android.text.Html;
import android.text.Layout;
import android.text.NoCopySpan;
import android.text.NoCopySpan.Concrete;
import android.text.Selection;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.Html.ImageGetter;
import android.text.Html.TagHandler;
import android.text.method.ArrowKeyMovementMethod;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.AlignmentSpan;
import android.text.style.BackgroundColorSpan;
import android.text.style.CharacterStyle;
import android.text.style.DynamicDrawableSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.ImageSpan;
import android.text.style.ParagraphStyle;
import android.text.style.QuoteSpan;
import android.text.style.UnderlineSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * EditStyledText extends EditText for managing the flow and status to edit the styled text. This
 * manages the states and flows of editing, supports inserting image, import/export HTML.
 */
public class EditStyledText extends EditText {

    private static final String TAG = "EditStyledText";
    /**
     * DBG should be false at checking in.
     */
    private static final boolean DBG = true;

    /**
     * Modes of editing actions.
     */
    /** The mode that no editing action is done. */
    public static final int MODE_NOTHING = 0;
    /** The mode of copy. */
    public static final int MODE_COPY = 1;
    /** The mode of paste. */
    public static final int MODE_PASTE = 2;
    /** The mode of changing size. */
    public static final int MODE_SIZE = 3;
    /** The mode of changing color. */
    public static final int MODE_COLOR = 4;
    /** The mode of selection. */
    public static final int MODE_SELECT = 5;
    /** The mode of changing alignment. */
    public static final int MODE_ALIGN = 6;
    /** The mode of changing cut. */
    public static final int MODE_CUT = 7;
    public static final int MODE_TELOP = 8;
    public static final int MODE_SWING = 9;
    public static final int MODE_MARQUEE = 10;
    public static final int MODE_SELECTALL = 11;
    public static final int MODE_HORIZONTALLINE = 12;
    public static final int MODE_STOP_SELECT = 13;
    public static final int MODE_CLEARSTYLES = 14;
    public static final int MODE_IMAGE = 15;
    public static final int MODE_BGCOLOR = 16;
    public static final int MODE_PREVIEW = 17;
    public static final int MODE_CANCEL = 18;
    public static final int MODE_TEXTVIEWFUNCTION = 19;
    public static final int MODE_START_EDIT = 20;
    public static final int MODE_END_EDIT = 21;
    public static final int MODE_RESET = 22;
    public static final int MODE_SHOW_MENU = 23;

    /**
     * States of selection.
     */
    /** The state that selection isn't started. */
    public static final int STATE_SELECT_OFF = 0;
    /** The state that selection is started. */
    public static final int STATE_SELECT_ON = 1;
    /** The state that selection is done, but not fixed. */
    public static final int STATE_SELECTED = 2;
    /** The state that selection is done and not fixed. */
    public static final int STATE_SELECT_FIX = 3;

    /**
     * Help message strings.
     */
    public static final int HINT_MSG_NULL = 0;
    public static final int HINT_MSG_COPY_BUF_BLANK = 1;
    public static final int HINT_MSG_SELECT_START = 2;
    public static final int HINT_MSG_SELECT_END = 3;
    public static final int HINT_MSG_PUSH_COMPETE = 4;
    public static final int HINT_MSG_BIG_SIZE_ERROR = 5;
    public static final int HINT_MSG_END_PREVIEW = 6;
    public static final int HINT_MSG_END_COMPOSE = 7;

    /**
     * Fixed Values.
     */
    public static final int DEFAULT_TRANSPARENT_COLOR = 0x00FFFFFF;
    public static final int DEFAULT_FOREGROUND_COLOR = 0xFF000000;
    public static final char ZEROWIDTHCHAR = '\u2060';
    public static final char IMAGECHAR = '\uFFFC';
    private static final int ID_SELECT_ALL = android.R.id.selectAll;
    private static final int ID_START_SELECTING_TEXT = android.R.id.startSelectingText;
    private static final int ID_STOP_SELECTING_TEXT = android.R.id.stopSelectingText;
    private static final int ID_PASTE = android.R.id.paste;
    private static final int ID_COPY = android.R.id.copy;
    private static final int ID_CUT = android.R.id.cut;
    private static final int ID_HORIZONTALLINE = 0x00FFFF01;
    private static final int ID_CLEARSTYLES = 0x00FFFF02;
    private static final int ID_SHOWEDIT = 0x00FFFF03;
    private static final int ID_HIDEEDIT = 0x00FFFF04;
    private static final int MAXIMAGEWIDTHDIP = 300;

    /**
     * Strings for context menu. TODO: Extract the strings to strings.xml.
     */
    private static CharSequence STR_HORIZONTALLINE;
    private static CharSequence STR_CLEARSTYLES;
    private static CharSequence STR_PASTE;

    private float mPaddingScale = 0;
    private ArrayList<EditStyledTextNotifier> mESTNotifiers;
    private Drawable mDefaultBackground;
    // EditStyledTextEditorManager manages the flow and status of each function of StyledText.
    private EditorManager mManager;
    private InputConnection mInputConnection;
    private StyledTextConverter mConverter;
    private StyledTextDialog mDialog;

    private static final Concrete SELECTING = new NoCopySpan.Concrete();
    private static final int PRESSED = Spannable.SPAN_MARK_MARK | (1 << Spannable.SPAN_USER_SHIFT);

    /**
     * EditStyledText extends EditText for managing flow of each editing action.
     */
    public EditStyledText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    public EditStyledText(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public EditStyledText(Context context) {
        super(context);
        init();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean superResult;
        if (event.getAction() == MotionEvent.ACTION_UP) {
            cancelLongPress();
            boolean editting = isEditting();
            // If View is touched but not in Edit Mode, starts Edit Mode.
            if (!editting) {
                onStartEdit();
            }
            int oldSelStart = Selection.getSelectionStart(getText());
            int oldSelEnd = Selection.getSelectionEnd(getText());
            superResult = super.onTouchEvent(event);
            if (isFocused()) {
                // If selection is started, don't open soft key by
                // touching.
                if (getSelectState() == STATE_SELECT_OFF) {
                    if (editting) {
                        mManager.showSoftKey(Selection.getSelectionStart(getText()),
                                Selection.getSelectionEnd(getText()));
                    } else {
                        mManager.showSoftKey(oldSelStart, oldSelEnd);
                    }
                }
            }
            mManager.onCursorMoved();
            mManager.unsetTextComposingMask();
        } else {
            superResult = super.onTouchEvent(event);
        }
        sendOnTouchEvent(event);
        return superResult;
    }

    @Override
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedStyledTextState ss = new SavedStyledTextState(superState);
        ss.mBackgroundColor = mManager.getBackgroundColor();
        return ss;
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        if (!(state instanceof SavedStyledTextState)) {
            super.onRestoreInstanceState(state);
            return;
        }
        SavedStyledTextState ss = (SavedStyledTextState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        setBackgroundColor(ss.mBackgroundColor);
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        if (mManager != null) {
            mManager.onRefreshStyles();
        }
    }

    @Override
    public boolean onTextContextMenuItem(int id) {
        boolean selection = getSelectionStart() != getSelectionEnd();
        switch (id) {
            case ID_SELECT_ALL:
                onStartSelectAll();
                return true;
            case ID_START_SELECTING_TEXT:
                onStartSelect();
                mManager.blockSoftKey();
                break;
            case ID_STOP_SELECTING_TEXT:
                onFixSelectedItem();
                break;
            case ID_PASTE:
                onStartPaste();
                return true;
            case ID_COPY:
                if (selection) {
                    onStartCopy();
                } else {
                    mManager.onStartSelectAll(false);
                    onStartCopy();
                }
                return true;
            case ID_CUT:
                if (selection) {
                    onStartCut();
                } else {
                    mManager.onStartSelectAll(false);
                    onStartCut();
                }
                return true;
            case ID_HORIZONTALLINE:
                onInsertHorizontalLine();
                return true;
            case ID_CLEARSTYLES:
                onClearStyles();
                return true;
            case ID_SHOWEDIT:
                onStartEdit();
                return true;
            case ID_HIDEEDIT:
                onEndEdit();
                return true;
        }
        return super.onTextContextMenuItem(id);
    }

    @Override
    protected void onCreateContextMenu(ContextMenu menu) {
        super.onCreateContextMenu(menu);
        MenuHandler handler = new MenuHandler();
        if (STR_HORIZONTALLINE != null) {
            menu.add(0, ID_HORIZONTALLINE, 0, STR_HORIZONTALLINE).setOnMenuItemClickListener(
                    handler);
        }
        if (isStyledText() && STR_CLEARSTYLES != null) {
            menu.add(0, ID_CLEARSTYLES, 0, STR_CLEARSTYLES)
                    .setOnMenuItemClickListener(handler);
        }
        if (mManager.canPaste()) {
            menu.add(0, ID_PASTE, 0, STR_PASTE)
                    .setOnMenuItemClickListener(handler).setAlphabeticShortcut('v');
        }
    }

    @Override
    protected void onTextChanged(CharSequence text, int start, int before, int after) {
        // onTextChanged will be called super's constructor.
        if (mManager != null) {
            mManager.updateSpanNextToCursor(getText(), start, before, after);
            mManager.updateSpanPreviousFromCursor(getText(), start, before, after);
            if (after > before) {
                mManager.setTextComposingMask(start, start + after);
            } else if (before < after) {
                mManager.unsetTextComposingMask();
            }
            if (mManager.isWaitInput()) {
                if (after > before) {
                    mManager.onCursorMoved();
                    onFixSelectedItem();
                } else if (after < before) {
                    mManager.onAction(MODE_RESET);
                }
            }
        }
        super.onTextChanged(text, start, before, after);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        mInputConnection =
                new StyledTextInputConnection(super.onCreateInputConnection(outAttrs), this);
        return mInputConnection;
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
        if (focused) {
            onStartEdit();
        } else if (!isButtonsFocused()) {
            onEndEdit();
        }
    }

    /**
     * Initialize members.
     */
    private void init() {
        mConverter = new StyledTextConverter(this, new StyledTextHtmlStandard());
        mDialog = new StyledTextDialog(this);
        mManager = new EditorManager(this, mDialog);
        setMovementMethod(new StyledTextArrowKeyMethod(mManager));
        mDefaultBackground = getBackground();
        requestFocus();
    }

    public interface StyledTextHtmlConverter {
        public String toHtml(Spanned text);

        public String toHtml(Spanned text, boolean escapeNonAsciiChar);

        public String toHtml(Spanned text, boolean escapeNonAsciiChar, int width, float scale);

        public Spanned fromHtml(String string);

        public Spanned fromHtml(String source, ImageGetter imageGetter, TagHandler tagHandler);
    }

    public void setStyledTextHtmlConverter(StyledTextHtmlConverter html) {
        mConverter.setStyledTextHtmlConverter(html);
    }

    /**
     * EditStyledTextInterface provides functions for notifying messages to calling class.
     */
    public interface EditStyledTextNotifier {
        public void sendHintMsg(int msgId);

        public void onStateChanged(int mode, int state);

        public boolean sendOnTouchEvent(MotionEvent event);

        public boolean isButtonsFocused();

        public boolean showPreview();

        public void cancelViewManager();

        public boolean showInsertImageSelectAlertDialog();

        public boolean showMenuAlertDialog();
    }

    /**
     * Add Notifier.
     */
    public void addEditStyledTextListener(EditStyledTextNotifier estInterface) {
        if (mESTNotifiers == null) {
            mESTNotifiers = new ArrayList<EditStyledTextNotifier>();
        }
        mESTNotifiers.add(estInterface);
    }

    /**
     * Remove Notifier.
     */
    public void removeEditStyledTextListener(EditStyledTextNotifier estInterface) {
        if (mESTNotifiers != null) {
            int i = mESTNotifiers.indexOf(estInterface);

            if (i > 0) {
                mESTNotifiers.remove(i);
            }
        }
    }

    private void sendOnTouchEvent(MotionEvent event) {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                notifier.sendOnTouchEvent(event);
            }
        }
    }

    public boolean isButtonsFocused() {
        boolean retval = false;
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                retval |= notifier.isButtonsFocused();
            }
        }
        return retval;
    }

    private void showPreview() {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                if (notifier.showPreview()) {
                    break;
                }
            }
        }
    }

    private void cancelViewManagers() {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                notifier.cancelViewManager();
            }
        }
    }

    private void showInsertImageSelectAlertDialog() {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                if (notifier.showInsertImageSelectAlertDialog()) {
                    break;
                }
            }
        }
    }

    private void showMenuAlertDialog() {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                if (notifier.showMenuAlertDialog()) {
                    break;
                }
            }
        }
    }

    /**
     * Notify hint messages what action is expected to calling class.
     *
     * @param msgId Id of the hint message.
     */
    private void sendHintMessage(int msgId) {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                notifier.sendHintMsg(msgId);
            }
        }
    }

    /**
     * Notify the event that the mode and state are changed.
     *
     * @param mode Mode of the editing action.
     * @param state Mode of the selection state.
     */
    private void notifyStateChanged(int mode, int state) {
        if (mESTNotifiers != null) {
            for (EditStyledTextNotifier notifier : mESTNotifiers) {
                notifier.onStateChanged(mode, state);
            }
        }
    }

    /** Start to edit styled text */
    public void onStartEdit() {
        mManager.onAction(MODE_START_EDIT);
    }

    /** End of editing styled text */
    public void onEndEdit() {
        mManager.onAction(MODE_END_EDIT);
    }

    public void onResetEdit() {
        mManager.onAction(MODE_RESET);
    }

    /** Start to copy styled text */
    public void onStartCopy() {
        mManager.onAction(MODE_COPY);
    }

    /** Start to cut styled text */
    public void onStartCut() {
        mManager.onAction(MODE_CUT);
    }

    /** Start to paste styled text */
    public void onStartPaste() {
        mManager.onAction(MODE_PASTE);
    }

    /** Start to change size */
    public void onStartSize() {
        mManager.onAction(MODE_SIZE);
    }

    /** Start to change color */
    public void onStartColor() {
        mManager.onAction(MODE_COLOR);
    }

    /** Start to change background color */
    public void onStartBackgroundColor() {
        mManager.onAction(MODE_BGCOLOR);
    }

    /** Start to change Alignment */
    public void onStartAlign() {
        mManager.onAction(MODE_ALIGN);
    }

    public void onStartTelop() {
        mManager.onAction(MODE_TELOP);
    }

    public void onStartSwing() {
        mManager.onAction(MODE_SWING);
    }

    public void onStartMarquee() {
        mManager.onAction(MODE_MARQUEE);
    }

    /** Start to select a text */
    public void onStartSelect() {
        mManager.onStartSelect(true);
    }

    /** Start to select all characters */
    public void onStartSelectAll() {
        mManager.onStartSelectAll(true);
    }

    public void onStartShowPreview() {
        mManager.onAction(MODE_PREVIEW);
    }

    public void onStartShowMenuAlertDialog() {
        mManager.onStartShowMenuAlertDialog();
    }

    public void onStartAction(int mode, boolean notifyStateChanged) {
        mManager.onAction(mode, notifyStateChanged);
    }

    /** Fix selection */
    public void onFixSelectedItem() {
        mManager.onFixSelectedItem();
    }

    public void onInsertImage() {
        mManager.onAction(MODE_IMAGE);
    }

    /**
     * InsertImage to TextView by using URI
     *
     * @param uri URI of the iamge inserted to TextView.
     */
    public void onInsertImage(Uri uri) {
        mManager.onInsertImage(uri);
    }

    /**
     * InsertImage to TextView by using resource ID
     *
     * @param resId Resource ID of the iamge inserted to TextView.
     */
    public void onInsertImage(int resId) {
        mManager.onInsertImage(resId);
    }

    public void onInsertHorizontalLine() {
        mManager.onAction(MODE_HORIZONTALLINE);
    }

    public void onClearStyles() {
        mManager.onClearStyles();
    }

    public void onBlockSoftKey() {
        mManager.blockSoftKey();
    }

    public void onUnblockSoftKey() {
        mManager.unblockSoftKey();
    }

    public void onCancelViewManagers() {
        mManager.onCancelViewManagers();
    }

    private void onRefreshStyles() {
        mManager.onRefreshStyles();
    }

    private void onRefreshZeoWidthChar() {
        mManager.onRefreshZeoWidthChar();
    }

    /**
     * Set Size of the Item.
     *
     * @param size The size of the Item.
     */
    public void setItemSize(int size) {
        mManager.setItemSize(size, true);
    }

    /**
     * Set Color of the Item.
     *
     * @param color The color of the Item.
     */
    public void setItemColor(int color) {
        mManager.setItemColor(color, true);
    }

    /**
     * Set Alignment of the Item.
     *
     * @param align The color of the Item.
     */
    public void setAlignment(Layout.Alignment align) {
        mManager.setAlignment(align);
    }

    /**
     * Set Background color of View.
     *
     * @param color The background color of view.
     */
    @Override
    public void setBackgroundColor(int color) {
        if (color != DEFAULT_TRANSPARENT_COLOR) {
            super.setBackgroundColor(color);
        } else {
            setBackgroundDrawable(mDefaultBackground);
        }
        mManager.setBackgroundColor(color);
        onRefreshStyles();
    }

    public void setMarquee(int marquee) {
        mManager.setMarquee(marquee);
    }

    /**
     * Set html to EditStyledText.
     *
     * @param html The html to be set.
     */
    public void setHtml(String html) {
        mConverter.SetHtml(html);
    }

    /**
     * Set Builder for AlertDialog.
     *
     * @param builder Builder for opening Alert Dialog.
     */
    public void setBuilder(Builder builder) {
        mDialog.setBuilder(builder);
    }

    /**
     * Set Parameters for ColorAlertDialog.
     *
     * @param colortitle Title for Alert Dialog.
     * @param colornames List of name of selecting color.
     * @param colorints List of int of color.
     */
    public void setColorAlertParams(CharSequence colortitle, CharSequence[] colornames,
            CharSequence[] colorints, CharSequence transparent) {
        mDialog.setColorAlertParams(colortitle, colornames, colorints, transparent);
    }

    /**
     * Set Parameters for SizeAlertDialog.
     *
     * @param sizetitle Title for Alert Dialog.
     * @param sizenames List of name of selecting size.
     * @param sizedisplayints List of int of size displayed in TextView.
     * @param sizesendints List of int of size exported to HTML.
     */
    public void setSizeAlertParams(CharSequence sizetitle, CharSequence[] sizenames,
            CharSequence[] sizedisplayints, CharSequence[] sizesendints) {
        mDialog.setSizeAlertParams(sizetitle, sizenames, sizedisplayints, sizesendints);
    }

    public void setAlignAlertParams(CharSequence aligntitle, CharSequence[] alignnames) {
        mDialog.setAlignAlertParams(aligntitle, alignnames);
    }

    public void setMarqueeAlertParams(CharSequence marqueetitle, CharSequence[] marqueenames) {
        mDialog.setMarqueeAlertParams(marqueetitle, marqueenames);
    }

    public void setContextMenuStrings(CharSequence horizontalline, CharSequence clearstyles,
            CharSequence paste) {
        STR_HORIZONTALLINE = horizontalline;
        STR_CLEARSTYLES = clearstyles;
        STR_PASTE = paste;
    }

    /**
     * Check whether editing is started or not.
     *
     * @return Whether editing is started or not.
     */
    public boolean isEditting() {
        return mManager.isEditting();
    }

    /**
     * Check whether styled text or not.
     *
     * @return Whether styled text or not.
     */
    public boolean isStyledText() {
        return mManager.isStyledText();
    }

    /**
     * Check whether SoftKey is Blocked or not.
     *
     * @return whether SoftKey is Blocked or not.
     */
    public boolean isSoftKeyBlocked() {
        return mManager.isSoftKeyBlocked();
    }

    /**
     * Get the mode of the action.
     *
     * @return The mode of the action.
     */
    public int getEditMode() {
        return mManager.getEditMode();
    }

    /**
     * Get the state of the selection.
     *
     * @return The state of the selection.
     */
    public int getSelectState() {
        return mManager.getSelectState();
    }

    /**
     * Get the state of the selection.
     *
     * @return The state of the selection.
     */
    public String getHtml() {
        return mConverter.getHtml(true);
    }

    public String getHtml(boolean escapeFlag) {
        return mConverter.getHtml(escapeFlag);
    }

    /**
     * Get the state of the selection.
     *
     * @param uris The array of used uris.
     * @return The state of the selection.
     */
    public String getHtml(ArrayList<Uri> uris, boolean escapeFlag) {
        mConverter.getUriArray(uris, getText());
        return mConverter.getHtml(escapeFlag);
    }

    public String getPreviewHtml() {
        return mConverter.getPreviewHtml();
    }

    /**
     * Get Background color of View.
     *
     * @return The background color of View.
     */
    public int getBackgroundColor() {
        return mManager.getBackgroundColor();
    }

    public EditorManager getEditStyledTextManager() {
        return mManager;
    }

    /**
     * Get Foreground color of View.
     *
     * @return The background color of View.
     */
    public int getForegroundColor(int pos) {
        if (pos < 0 || pos > getText().length()) {
            return DEFAULT_FOREGROUND_COLOR;
        } else {
            ForegroundColorSpan[] spans =
                    getText().getSpans(pos, pos, ForegroundColorSpan.class);
            if (spans.length > 0) {
                return spans[0].getForegroundColor();
            } else {
                return DEFAULT_FOREGROUND_COLOR;
            }
        }
    }

    private void finishComposingText() {
        if (mInputConnection != null && !mManager.mTextIsFinishedFlag) {
            mInputConnection.finishComposingText();
            mManager.mTextIsFinishedFlag = true;
        }
    }

    private float getPaddingScale() {
        if (mPaddingScale <= 0) {
            mPaddingScale = getContext().getResources().getDisplayMetrics().density;
        }
        return mPaddingScale;
    }

    /** Convert pixcel to DIP */
    private int dipToPx(int dip) {
        if (mPaddingScale <= 0) {
            mPaddingScale = getContext().getResources().getDisplayMetrics().density;
        }
        return (int) ((float) dip * getPaddingScale() + 0.5);
    }

    private int getMaxImageWidthDip() {
        return MAXIMAGEWIDTHDIP;
    }

    private int getMaxImageWidthPx() {
        return dipToPx(MAXIMAGEWIDTHDIP);
    }

    public void addAction(int mode, EditModeActionBase action) {
        mManager.addAction(mode, action);
    }

    public void addInputExtra(boolean create, String extra) {
        Bundle bundle = super.getInputExtras(create);
        if (bundle != null) {
            bundle.putBoolean(extra, true);
        }
    }

    private static void startSelecting(View view, Spannable content) {
        content.setSpan(SELECTING, 0, 0, PRESSED);
    }

    private static void stopSelecting(View view, Spannable content) {
        content.removeSpan(SELECTING);
    }

    /**
     * EditorManager manages the flow and status of editing actions.
     */
    private class EditorManager {

        static final private String LOG_TAG = "EditStyledText.EditorManager";

        private boolean mEditFlag = false;
        private boolean mSoftKeyBlockFlag = false;
        private boolean mKeepNonLineSpan = false;
        private boolean mWaitInputFlag = false;
        private boolean mTextIsFinishedFlag = false;
        private int mMode = MODE_NOTHING;
        private int mState = STATE_SELECT_OFF;
        private int mCurStart = 0;
        private int mCurEnd = 0;
        private int mColorWaitInput = DEFAULT_TRANSPARENT_COLOR;
        private int mSizeWaitInput = 0;
        private int mBackgroundColor = DEFAULT_TRANSPARENT_COLOR;

        private BackgroundColorSpan mComposingTextMask;
        private EditStyledText mEST;
        private EditModeActions mActions;
        private SoftKeyReceiver mSkr;
        private SpannableStringBuilder mCopyBuffer;

        EditorManager(EditStyledText est, StyledTextDialog dialog) {
            mEST = est;
            mActions = new EditModeActions(mEST, this, dialog);
            mSkr = new SoftKeyReceiver(mEST);
        }

        public void addAction(int mode, EditModeActionBase action) {
            mActions.addAction(mode, action);
        }

        public void onAction(int mode) {
            onAction(mode, true);
        }

        public void onAction(int mode, boolean notifyStateChanged) {
            mActions.onAction(mode);
            if (notifyStateChanged) {
                mEST.notifyStateChanged(mMode, mState);
            }
        }

        private void startEdit() {
            resetEdit();
            showSoftKey();
        }

        public void onStartSelect(boolean notifyStateChanged) {
            if (DBG) {
                Log.d(LOG_TAG, "--- onClickSelect");
            }
            mMode = MODE_SELECT;
            if (mState == STATE_SELECT_OFF) {
                mActions.onSelectAction();
            } else {
                unsetSelect();
                mActions.onSelectAction();
            }
            if (notifyStateChanged) {
                mEST.notifyStateChanged(mMode, mState);
            }
        }

        public void onCursorMoved() {
            if (DBG) {
                Log.d(LOG_TAG, "--- onClickView");
            }
            if (mState == STATE_SELECT_ON || mState == STATE_SELECTED) {
                mActions.onSelectAction();
                mEST.notifyStateChanged(mMode, mState);
            }
        }

        public void onStartSelectAll(boolean notifyStateChanged) {
            if (DBG) {
                Log.d(LOG_TAG, "--- onClickSelectAll");
            }
            handleSelectAll();
            if (notifyStateChanged) {
                mEST.notifyStateChanged(mMode, mState);
            }
        }

        public void onStartShowMenuAlertDialog() {
            mActions.onAction(MODE_SHOW_MENU);
            // don't call notify state changed because it have to continue
            // to the next action.
            // mEST.notifyStateChanged(mMode, mState);
        }

        public void onFixSelectedItem() {
            if (DBG) {
                Log.d(LOG_TAG, "--- onFixSelectedItem");
            }
            fixSelectionAndDoNextAction();
            mEST.notifyStateChanged(mMode, mState);
        }

        public void onInsertImage(Uri uri) {
            mActions.onAction(MODE_IMAGE, uri);
            mEST.notifyStateChanged(mMode, mState);
        }

        public void onInsertImage(int resId) {
            mActions.onAction(MODE_IMAGE, resId);
            mEST.notifyStateChanged(mMode, mState);
        }

        private void insertImageFromUri(Uri uri) {
            insertImageSpan(new EditStyledTextSpans.RescalableImageSpan(mEST.getContext(),
                    uri, mEST.getMaxImageWidthPx()), mEST.getSelectionStart());
        }

        private void insertImageFromResId(int resId) {
            insertImageSpan(new EditStyledTextSpans.RescalableImageSpan(mEST.getContext(),
                    resId, mEST.getMaxImageWidthDip()), mEST.getSelectionStart());
        }

        private void insertHorizontalLine() {
            if (DBG) {
                Log.d(LOG_TAG, "--- onInsertHorizontalLine:");
            }
            int curpos = mEST.getSelectionStart();
            if (curpos > 0 && mEST.getText().charAt(curpos - 1) != '\n') {
                mEST.getText().insert(curpos++, "\n");
            }
            insertImageSpan(
                    new HorizontalLineSpan(0xFF000000, mEST.getWidth(), mEST.getText()),
                    curpos++);
            mEST.getText().insert(curpos++, "\n");
            mEST.setSelection(curpos);
            mEST.notifyStateChanged(mMode, mState);
        }

        private void clearStyles(CharSequence txt) {
            if (DBG) {
                Log.d("EditStyledText", "--- onClearStyles");
            }
            int len = txt.length();
            if (txt instanceof Editable) {
                Editable editable = (Editable) txt;
                Object[] styles = editable.getSpans(0, len, Object.class);
                for (Object style : styles) {
                    if (style instanceof ParagraphStyle || style instanceof QuoteSpan
                            || style instanceof CharacterStyle
                            && !(style instanceof UnderlineSpan)) {
                        if (style instanceof ImageSpan || style instanceof HorizontalLineSpan) {
                            int start = editable.getSpanStart(style);
                            int end = editable.getSpanEnd(style);
                            editable.replace(start, end, "");
                        }
                        editable.removeSpan(style);
                    }
                }
            }
        }

        public void onClearStyles() {
            mActions.onAction(MODE_CLEARSTYLES);
        }

        public void onCancelViewManagers() {
            mActions.onAction(MODE_CANCEL);
        }

        private void clearStyles() {
            if (DBG) {
                Log.d(LOG_TAG, "--- onClearStyles");
            }
            clearStyles(mEST.getText());
            mEST.setBackgroundDrawable(mEST.mDefaultBackground);
            mBackgroundColor = DEFAULT_TRANSPARENT_COLOR;
            onRefreshZeoWidthChar();
        }

        public void onRefreshZeoWidthChar() {
            Editable txt = mEST.getText();
            for (int i = 0; i < txt.length(); i++) {
                if (txt.charAt(i) == ZEROWIDTHCHAR) {
                    txt.replace(i, i + 1, "");
                    i--;
                }
            }
        }

        public void onRefreshStyles() {
            if (DBG) {
                Log.d(LOG_TAG, "--- onRefreshStyles");
            }
            Editable txt = mEST.getText();
            int len = txt.length();
            int width = mEST.getWidth();
            HorizontalLineSpan[] lines = txt.getSpans(0, len, HorizontalLineSpan.class);
            for (HorizontalLineSpan line : lines) {
                line.resetWidth(width);
            }
            MarqueeSpan[] marquees = txt.getSpans(0, len, MarqueeSpan.class);
            for (MarqueeSpan marquee : marquees) {
                marquee.resetColor(mEST.getBackgroundColor());
            }

            if (lines.length > 0) {
                // This is hack, bad needed for renewing View
                // by inserting new line.
                txt.replace(0, 1, "" + txt.charAt(0));
            }
        }

        public void setBackgroundColor(int color) {
            mBackgroundColor = color;
        }

        public void setItemSize(int size, boolean reset) {
            if (DBG) {
                Log.d(LOG_TAG, "--- setItemSize");
            }
            if (isWaitingNextAction()) {
                mSizeWaitInput = size;
            } else if (mState == STATE_SELECTED || mState == STATE_SELECT_FIX) {
                if (size > 0) {
                    changeSizeSelectedText(size);
                }
                if (reset) {
                    resetEdit();
                }
            }
        }

        public void setItemColor(int color, boolean reset) {
            if (DBG) {
                Log.d(LOG_TAG, "--- setItemColor");
            }
            if (isWaitingNextAction()) {
                mColorWaitInput = color;
            } else if (mState == STATE_SELECTED || mState == STATE_SELECT_FIX) {
                if (color != DEFAULT_TRANSPARENT_COLOR) {
                    changeColorSelectedText(color);
                }
                if (reset) {
                    resetEdit();
                }
            }
        }

        public void setAlignment(Layout.Alignment align) {
            if (mState == STATE_SELECTED || mState == STATE_SELECT_FIX) {
                changeAlign(align);
                resetEdit();
            }
        }

        public void setTelop() {
            if (mState == STATE_SELECTED || mState == STATE_SELECT_FIX) {
                addTelop();
                resetEdit();
            }
        }

        public void setSwing() {
            if (mState == STATE_SELECTED || mState == STATE_SELECT_FIX) {
                addSwing();
                resetEdit();
            }
        }

        public void setMarquee(int marquee) {
            if (mState == STATE_SELECTED || mState == STATE_SELECT_FIX) {
                addMarquee(marquee);
                resetEdit();
            }
        }

        public void setTextComposingMask(int start, int end) {
            if (DBG) {
                Log.d(TAG, "--- setTextComposingMask:" + start + "," + end);
            }
            int min = Math.min(start, end);
            int max = Math.max(start, end);
            int foregroundColor;
            if (isWaitInput() && mColorWaitInput != DEFAULT_TRANSPARENT_COLOR) {
                foregroundColor = mColorWaitInput;
            } else {
                foregroundColor = mEST.getForegroundColor(min);
            }
            int backgroundColor = mEST.getBackgroundColor();
            if (DBG) {
                Log.d(TAG,
                        "--- fg:" + Integer.toHexString(foregroundColor) + ",bg:"
                                + Integer.toHexString(backgroundColor) + "," + isWaitInput()
                                + "," + "," + mMode);
            }
            if (foregroundColor == backgroundColor) {
                int maskColor = 0x80000000 | ~(backgroundColor | 0xFF000000);
                if (mComposingTextMask == null
                        || mComposingTextMask.getBackgroundColor() != maskColor) {
                    mComposingTextMask = new BackgroundColorSpan(maskColor);
                }
                mEST.getText().setSpan(mComposingTextMask, min, max,
                        Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
        }

        private void setEditMode(int mode) {
            mMode = mode;
        }

        private void setSelectState(int state) {
            mState = state;
        }

        public void unsetTextComposingMask() {
            if (DBG) {
                Log.d(TAG, "--- unsetTextComposingMask");
            }
            if (mComposingTextMask != null) {
                mEST.getText().removeSpan(mComposingTextMask);
                mComposingTextMask = null;
            }
        }

        public boolean isEditting() {
            return mEditFlag;
        }

        /* If the style of the span is added, add check case for that style */
        public boolean isStyledText() {
            Editable txt = mEST.getText();
            int len = txt.length();
            if (txt.getSpans(0, len, ParagraphStyle.class).length > 0
                    || txt.getSpans(0, len, QuoteSpan.class).length > 0
                    || txt.getSpans(0, len, CharacterStyle.class).length > 0
                    || mBackgroundColor != DEFAULT_TRANSPARENT_COLOR) {
                return true;
            }
            return false;
        }

        public boolean isSoftKeyBlocked() {
            return mSoftKeyBlockFlag;
        }

        public boolean isWaitInput() {
            return mWaitInputFlag;
        }

        public int getBackgroundColor() {
            return mBackgroundColor;
        }

        public int getEditMode() {
            return mMode;
        }

        public int getSelectState() {
            return mState;
        }

        public int getSelectionStart() {
            return mCurStart;
        }

        public int getSelectionEnd() {
            return mCurEnd;
        }

        public int getSizeWaitInput() {
            return mSizeWaitInput;
        }

        public int getColorWaitInput() {
            return mColorWaitInput;
        }

        private void setInternalSelection(int curStart, int curEnd) {
            mCurStart = curStart;
            mCurEnd = curEnd;
        }

        public void
                updateSpanPreviousFromCursor(Editable txt, int start, int before, int after) {
            if (DBG) {
                Log.d(LOG_TAG, "updateSpanPrevious:" + start + "," + before + "," + after);
            }
            int end = start + after;
            int min = Math.min(start, end);
            int max = Math.max(start, end);
            Object[] spansBefore = txt.getSpans(min, min, Object.class);
            for (Object span : spansBefore) {
                if (span instanceof ForegroundColorSpan || span instanceof AbsoluteSizeSpan
                        || span instanceof MarqueeSpan || span instanceof AlignmentSpan) {
                    int spanstart = txt.getSpanStart(span);
                    int spanend = txt.getSpanEnd(span);
                    if (DBG) {
                        Log.d(LOG_TAG, "spantype:" + span.getClass() + "," + spanstart);
                    }
                    int tempmax = max;
                    if (span instanceof MarqueeSpan || span instanceof AlignmentSpan) {
                        // Line Span
                        tempmax = findLineEnd(mEST.getText(), max);
                    } else {
                        if (mKeepNonLineSpan) {
                            tempmax = spanend;
                        }
                    }
                    if (spanend < tempmax) {
                        if (DBG) {
                            Log.d(LOG_TAG, "updateSpanPrevious: extend span");
                        }
                        txt.setSpan(span, spanstart, tempmax,
                                Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                    }
                } else if (span instanceof HorizontalLineSpan) {
                    int spanstart = txt.getSpanStart(span);
                    int spanend = txt.getSpanEnd(span);
                    if (before > after) {
                        // When text is deleted just after horizontalLineSpan, horizontalLineSpan
                        // has to be deleted, because the charactor just after horizontalLineSpan
                        // is '\n'.
                        txt.replace(spanstart, spanend, "");
                        txt.removeSpan(span);
                    } else {
                        // When text is added just after horizontalLineSpan add '\n' just after
                        // horizontalLineSpan.
                        if (spanend == end && end < txt.length()
                                && mEST.getText().charAt(end) != '\n') {
                            mEST.getText().insert(end, "\n");
                        }
                    }
                }
            }
        }

        public void updateSpanNextToCursor(Editable txt, int start, int before, int after) {
            if (DBG) {
                Log.d(LOG_TAG, "updateSpanNext:" + start + "," + before + "," + after);
            }
            int end = start + after;
            int min = Math.min(start, end);
            int max = Math.max(start, end);
            Object[] spansAfter = txt.getSpans(max, max, Object.class);
            for (Object span : spansAfter) {
                if (span instanceof MarqueeSpan || span instanceof AlignmentSpan) {
                    int spanstart = txt.getSpanStart(span);
                    int spanend = txt.getSpanEnd(span);
                    if (DBG) {
                        Log.d(LOG_TAG, "spantype:" + span.getClass() + "," + spanend);
                    }
                    int tempmin = min;
                    if (span instanceof MarqueeSpan || span instanceof AlignmentSpan) {
                        tempmin = findLineStart(mEST.getText(), min);
                    }
                    if (tempmin < spanstart && before > after) {
                        txt.removeSpan(span);
                    } else if (spanstart > min) {
                        txt.setSpan(span, min, spanend, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                    }
                } else if (span instanceof HorizontalLineSpan) {
                    int spanstart = txt.getSpanStart(span);
                    // Whene text is changed just before horizontalLineSpan and there is no '\n'
                    // just before horizontalLineSpan add '\n'
                    if (spanstart == end && end > 0 && mEST.getText().charAt(end - 1) != '\n') {
                        mEST.getText().insert(end, "\n");
                        mEST.setSelection(end);
                    }
                }
            }
        }

        /** canPaste returns true only if ClipboardManager doen't contain text. */
        public boolean canPaste() {
            return (mCopyBuffer != null && mCopyBuffer.length() > 0 && removeImageChar(
                    mCopyBuffer).length() == 0);
        }

        private void endEdit() {
            if (DBG) {
                Log.d(LOG_TAG, "--- handleCancel");
            }
            mMode = MODE_NOTHING;
            mState = STATE_SELECT_OFF;
            mEditFlag = false;
            mColorWaitInput = DEFAULT_TRANSPARENT_COLOR;
            mSizeWaitInput = 0;
            mWaitInputFlag = false;
            mSoftKeyBlockFlag = false;
            mKeepNonLineSpan = false;
            mTextIsFinishedFlag = false;
            unsetSelect();
            mEST.setOnClickListener(null);
            unblockSoftKey();
        }

        private void fixSelectionAndDoNextAction() {
            if (DBG) {
                Log.d(LOG_TAG, "--- handleComplete:" + mCurStart + "," + mCurEnd);
            }
            if (!mEditFlag) {
                return;
            }
            if (mCurStart == mCurEnd) {
                if (DBG) {
                    Log.d(LOG_TAG, "--- cancel handle complete:" + mCurStart);
                }
                resetEdit();
                return;
            }
            if (mState == STATE_SELECTED) {
                mState = STATE_SELECT_FIX;
            }
            // When the complete button is clicked, this should do action.
            mActions.doNext(mMode);
            //MetaKeyKeyListener.stopSelecting(mEST, mEST.getText());
            stopSelecting(mEST, mEST.getText());
        }

        // Remove obj character for DynamicDrawableSpan from clipboard.
        private SpannableStringBuilder removeImageChar(SpannableStringBuilder text) {
            SpannableStringBuilder buf = new SpannableStringBuilder(text);
            DynamicDrawableSpan[] styles =
                    buf.getSpans(0, buf.length(), DynamicDrawableSpan.class);
            for (DynamicDrawableSpan style : styles) {
                if (style instanceof HorizontalLineSpan
                        || style instanceof RescalableImageSpan) {
                    int start = buf.getSpanStart(style);
                    int end = buf.getSpanEnd(style);
                    buf.replace(start, end, "");
                }
            }
            return buf;
        }

        private void copyToClipBoard() {
            int min = Math.min(getSelectionStart(), getSelectionEnd());
            int max = Math.max(getSelectionStart(), getSelectionEnd());
            mCopyBuffer = (SpannableStringBuilder) mEST.getText().subSequence(min, max);
            SpannableStringBuilder clipboardtxt = removeImageChar(mCopyBuffer);
            ClipboardManager clip =
                    (ClipboardManager) getContext()
                            .getSystemService(Context.CLIPBOARD_SERVICE);
            clip.setText(clipboardtxt);
            if (DBG) {
                dumpSpannableString(clipboardtxt);
                dumpSpannableString(mCopyBuffer);
            }
        }

        private void cutToClipBoard() {
            copyToClipBoard();
            int min = Math.min(getSelectionStart(), getSelectionEnd());
            int max = Math.max(getSelectionStart(), getSelectionEnd());
            mEST.getText().delete(min, max);
        }

        private boolean isClipBoardChanged(CharSequence clipboardText) {
            if (DBG) {
                Log.d(TAG, "--- isClipBoardChanged:" + clipboardText);
            }
            if (mCopyBuffer == null) {
                return true;
            }
            int len = clipboardText.length();
            CharSequence removedClipBoard = removeImageChar(mCopyBuffer);
            if (DBG) {
                Log.d(TAG, "--- clipBoard:" + len + "," + removedClipBoard + clipboardText);
            }
            if (len != removedClipBoard.length()) {
                return true;
            }
            for (int i = 0; i < len; ++i) {
                if (clipboardText.charAt(i) != removedClipBoard.charAt(i)) {
                    return true;
                }
            }
            return false;
        }

        private void pasteFromClipboard() {
            int min = Math.min(mEST.getSelectionStart(), mEST.getSelectionEnd());
            int max = Math.max(mEST.getSelectionStart(), mEST.getSelectionEnd());
            // TODO: Find more smart way to set Span to Clipboard.
            Selection.setSelection(mEST.getText(), max);
            ClipboardManager clip =
                    (ClipboardManager) getContext()
                            .getSystemService(Context.CLIPBOARD_SERVICE);
            mKeepNonLineSpan = true;
            mEST.getText().replace(min, max, clip.getText());
            if (!isClipBoardChanged(clip.getText())) {
                if (DBG) {
                    Log.d(TAG, "--- handlePaste: startPasteImage");
                }
                DynamicDrawableSpan[] styles =
                        mCopyBuffer.getSpans(0, mCopyBuffer.length(),
                                DynamicDrawableSpan.class);
                for (DynamicDrawableSpan style : styles) {
                    int start = mCopyBuffer.getSpanStart(style);
                    if (style instanceof HorizontalLineSpan) {
                        insertImageSpan(new HorizontalLineSpan(0xFF000000, mEST.getWidth(),
                                mEST.getText()), min + start);
                    } else if (style instanceof RescalableImageSpan) {
                        insertImageSpan(
                                new RescalableImageSpan(mEST.getContext(),
                                        ((RescalableImageSpan) style).getContentUri(),
                                        mEST.getMaxImageWidthPx()), min + start);
                    }
                }
            }
        }

        private void handleSelectAll() {
            if (!mEditFlag) {
                return;
            }
            mActions.onAction(MODE_SELECTALL);
        }

        private void selectAll() {
            Selection.selectAll(mEST.getText());
            mCurStart = mEST.getSelectionStart();
            mCurEnd = mEST.getSelectionEnd();
            mMode = MODE_SELECT;
            mState = STATE_SELECT_FIX;
        }

        private void resetEdit() {
            endEdit();
            mEditFlag = true;
            mEST.notifyStateChanged(mMode, mState);
        }

        private void setSelection() {
            if (DBG) {
                Log.d(LOG_TAG, "--- onSelect:" + mCurStart + "," + mCurEnd);
            }
            if (mCurStart >= 0 && mCurStart <= mEST.getText().length() && mCurEnd >= 0
                    && mCurEnd <= mEST.getText().length()) {
                if (mCurStart < mCurEnd) {
                    mEST.setSelection(mCurStart, mCurEnd);
                    mState = STATE_SELECTED;
                } else if (mCurStart > mCurEnd) {
                    mEST.setSelection(mCurEnd, mCurStart);
                    mState = STATE_SELECTED;
                } else {
                    mState = STATE_SELECT_ON;
                }
            } else {
                Log.e(LOG_TAG, "Select is on, but cursor positions are illigal.:"
                        + mEST.getText().length() + "," + mCurStart + "," + mCurEnd);
            }
        }

        private void unsetSelect() {
            if (DBG) {
                Log.d(LOG_TAG, "--- offSelect");
            }
            //MetaKeyKeyListener.stopSelecting(mEST, mEST.getText());
            stopSelecting(mEST, mEST.getText());
            int currpos = mEST.getSelectionStart();
            mEST.setSelection(currpos, currpos);
            mState = STATE_SELECT_OFF;
        }

        private void setSelectStartPos() {
            if (DBG) {
                Log.d(LOG_TAG, "--- setSelectStartPos");
            }
            mCurStart = mEST.getSelectionStart();
            mState = STATE_SELECT_ON;
        }

        private void setSelectEndPos() {
            if (mEST.getSelectionEnd() == mCurStart) {
                setEndPos(mEST.getSelectionStart());
            } else {
                setEndPos(mEST.getSelectionEnd());
            }
        }

        public void setEndPos(int pos) {
            if (DBG) {
                Log.d(LOG_TAG, "--- setSelectedEndPos:" + pos);
            }
            mCurEnd = pos;
            setSelection();
        }

        private boolean isWaitingNextAction() {
            if (DBG) {
                Log.d(LOG_TAG, "--- waitingNext:" + mCurStart + "," + mCurEnd + "," + mState);
            }
            if (mCurStart == mCurEnd && mState == STATE_SELECT_FIX) {
                waitSelection();
                return true;
            } else {
                resumeSelection();
                return false;
            }
        }

        private void waitSelection() {
            if (DBG) {
                Log.d(LOG_TAG, "--- waitSelection");
            }
            mWaitInputFlag = true;
            if (mCurStart == mCurEnd) {
                mState = STATE_SELECT_ON;
            } else {
                mState = STATE_SELECTED;
            }
            //MetaKeyKeyListener.startSelecting(mEST, mEST.getText());
            startSelecting(mEST, mEST.getText());
        }

        private void resumeSelection() {
            if (DBG) {
                Log.d(LOG_TAG, "--- resumeSelection");
            }
            mWaitInputFlag = false;
            mState = STATE_SELECT_FIX;
            //MetaKeyKeyListener.stopSelecting(mEST, mEST.getText());
            stopSelecting(mEST, mEST.getText());
        }

        private boolean isTextSelected() {
            return (mState == STATE_SELECTED || mState == STATE_SELECT_FIX);
        }

        private void setStyledTextSpan(Object span, int start, int end) {
            if (DBG) {
                Log.d(LOG_TAG, "--- setStyledTextSpan:" + mMode + "," + start + "," + end);
            }
            int min = Math.min(start, end);
            int max = Math.max(start, end);
            mEST.getText().setSpan(span, min, max, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            Selection.setSelection(mEST.getText(), max);
        }

        private void setLineStyledTextSpan(Object span) {
            int min = Math.min(mCurStart, mCurEnd);
            int max = Math.max(mCurStart, mCurEnd);
            int current = mEST.getSelectionStart();
            int start = findLineStart(mEST.getText(), min);
            int end = findLineEnd(mEST.getText(), max);
            if (start == end) {
                mEST.getText().insert(end, "\n");
                setStyledTextSpan(span, start, end + 1);
            } else {
                setStyledTextSpan(span, start, end);
            }
            Selection.setSelection(mEST.getText(), current);
        }

        private void changeSizeSelectedText(int size) {
            if (mCurStart != mCurEnd) {
                setStyledTextSpan(new AbsoluteSizeSpan(size), mCurStart, mCurEnd);
            } else {
                Log.e(LOG_TAG, "---changeSize: Size of the span is zero");
            }
        }

        private void changeColorSelectedText(int color) {
            if (mCurStart != mCurEnd) {
                setStyledTextSpan(new ForegroundColorSpan(color), mCurStart, mCurEnd);
            } else {
                Log.e(LOG_TAG, "---changeColor: Size of the span is zero");
            }
        }

        private void changeAlign(Layout.Alignment align) {
            setLineStyledTextSpan(new AlignmentSpan.Standard(align));
        }

        private void addTelop() {
            addMarquee(MarqueeSpan.ALTERNATE);
        }

        private void addSwing() {
            addMarquee(MarqueeSpan.SCROLL);
        }

        private void addMarquee(int marquee) {
            if (DBG) {
                Log.d(LOG_TAG, "--- addMarquee:" + marquee);
            }
            setLineStyledTextSpan(new MarqueeSpan(marquee, mEST.getBackgroundColor()));
        }

        private void insertImageSpan(DynamicDrawableSpan span, int curpos) {
            if (DBG) {
                Log.d(LOG_TAG, "--- insertImageSpan:");
            }
            if (span != null && span.getDrawable() != null) {
                mEST.getText().insert(curpos, "" + IMAGECHAR);
                mEST.getText().setSpan(span, curpos, curpos + 1,
                        Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                mEST.notifyStateChanged(mMode, mState);
            } else {
                Log.e(LOG_TAG, "--- insertImageSpan: null span was inserted");
                mEST.sendHintMessage(HINT_MSG_BIG_SIZE_ERROR);
            }
        }

        private int findLineStart(Editable text, int current) {
            int pos = current;
            for (; pos > 0; pos--) {
                if (text.charAt(pos - 1) == '\n') {
                    break;
                }
            }
            if (DBG) {
                Log.d(LOG_TAG, "--- findLineStart:" + current + "," + text.length() + ","
                        + pos);
            }
            return pos;
        }

        private int findLineEnd(Editable text, int current) {
            int pos = current;
            for (; pos < text.length(); pos++) {
                if (text.charAt(pos) == '\n') {
                    pos++;
                    break;
                }
            }
            if (DBG) {
                Log.d(LOG_TAG, "--- findLineEnd:" + current + "," + text.length() + "," + pos);
            }
            return pos;
        }

        // Only for debug.
        private void dumpSpannableString(CharSequence txt) {
            if (txt instanceof Spannable) {
                Spannable spannable = (Spannable) txt;
                int len = spannable.length();
                if (DBG) {
                    Log.d(TAG, "--- dumpSpannableString, txt:" + spannable + ", len:" + len);
                }
                Object[] styles = spannable.getSpans(0, len, Object.class);
                for (Object style : styles) {
                    if (DBG) {
                        Log.d(TAG,
                                "--- dumpSpannableString, class:" + style + ","
                                        + spannable.getSpanStart(style) + ","
                                        + spannable.getSpanEnd(style) + ","
                                        + spannable.getSpanFlags(style));
                    }
                }
            }
        }

        public void showSoftKey() {
            showSoftKey(mEST.getSelectionStart(), mEST.getSelectionEnd());
        }

        public void showSoftKey(int oldSelStart, int oldSelEnd) {
            if (DBG) {
                Log.d(LOG_TAG, "--- showsoftkey");
            }
            if (!mEST.isFocused() || isSoftKeyBlocked()) {
                return;
            }
            mSkr.mNewStart = Selection.getSelectionStart(mEST.getText());
            mSkr.mNewEnd = Selection.getSelectionEnd(mEST.getText());
            InputMethodManager imm =
                    (InputMethodManager) getContext().getSystemService(
                            Context.INPUT_METHOD_SERVICE);
            if (imm.showSoftInput(mEST, 0, mSkr) && mSkr != null) {
                Selection.setSelection(getText(), oldSelStart, oldSelEnd);
            }
        }

        public void hideSoftKey() {
            if (DBG) {
                Log.d(LOG_TAG, "--- hidesoftkey");
            }
            if (!mEST.isFocused()) {
                return;
            }
            mSkr.mNewStart = Selection.getSelectionStart(mEST.getText());
            mSkr.mNewEnd = Selection.getSelectionEnd(mEST.getText());
            InputMethodManager imm =
                    (InputMethodManager) mEST.getContext().getSystemService(
                            Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(mEST.getWindowToken(), 0, mSkr);
        }

        public void blockSoftKey() {
            if (DBG) {
                Log.d(LOG_TAG, "--- blockSoftKey:");
            }
            hideSoftKey();
            mSoftKeyBlockFlag = true;
        }

        public void unblockSoftKey() {
            if (DBG) {
                Log.d(LOG_TAG, "--- unblockSoftKey:");
            }
            mSoftKeyBlockFlag = false;
        }
    }

    private class StyledTextHtmlStandard implements StyledTextHtmlConverter {
        public String toHtml(Spanned text) {
            return Html.toHtml(text);
        }

        public String toHtml(Spanned text, boolean escapeNonAsciiChar) {
            return Html.toHtml(text);
        }

        public String toHtml(Spanned text, boolean escapeNonAsciiChar, int width, float scale) {
            return Html.toHtml(text);
        }

        public Spanned fromHtml(String source) {
            return Html.fromHtml(source);
        }

        public Spanned fromHtml(String source, ImageGetter imageGetter, TagHandler tagHandler) {
            return Html.fromHtml(source, imageGetter, tagHandler);
        }
    }

    private class StyledTextConverter {
        private EditStyledText mEST;
        private StyledTextHtmlConverter mHtml;

        public StyledTextConverter(EditStyledText est, StyledTextHtmlConverter html) {
            mEST = est;
            mHtml = html;
        }

        public void setStyledTextHtmlConverter(StyledTextHtmlConverter html) {
            mHtml = html;
        }

        public String getHtml(boolean escapeFlag) {
            mEST.clearComposingText();
            mEST.onRefreshZeoWidthChar();
            String htmlBody = mHtml.toHtml(mEST.getText(), escapeFlag);
            if (DBG) {
                Log.d(TAG, "--- getHtml:" + htmlBody);
            }
            return htmlBody;
        }

        public String getPreviewHtml() {
            mEST.clearComposingText();
            mEST.onRefreshZeoWidthChar();
            String html =
                    mHtml.toHtml(mEST.getText(), true, getMaxImageWidthDip(),
                            getPaddingScale());
            int bgColor = mEST.getBackgroundColor();
            html =
                    String.format("<body bgcolor=\"#%02X%02X%02X\">%s</body>",
                            Color.red(bgColor), Color.green(bgColor), Color.blue(bgColor),
                            html);
            if (DBG) {
                Log.d(TAG, "--- getPreviewHtml:" + html + "," + mEST.getWidth());
            }
            return html;
        }

        public void getUriArray(ArrayList<Uri> uris, Editable text) {
            uris.clear();
            if (DBG) {
                Log.d(TAG, "--- getUriArray:");
            }
            int len = text.length();
            int next;
            for (int i = 0; i < text.length(); i = next) {
                next = text.nextSpanTransition(i, len, ImageSpan.class);
                ImageSpan[] images = text.getSpans(i, next, ImageSpan.class);
                for (int j = 0; j < images.length; j++) {
                    if (DBG) {
                        Log.d(TAG, "--- getUriArray: foundArray" + images[j].getSource());
                    }
                    uris.add(Uri.parse(images[j].getSource()));
                }
            }
        }

        public void SetHtml(String html) {
            final Spanned spanned = mHtml.fromHtml(html, new Html.ImageGetter() {
                public Drawable getDrawable(String src) {
                    Log.d(TAG, "--- sethtml: src=" + src);
                    if (src.startsWith("content://")) {
                        Uri uri = Uri.parse(src);
                        try {
                            Drawable drawable = null;
                            Bitmap bitmap = null;
                            System.gc();
                            InputStream is =
                                    mEST.getContext().getContentResolver().openInputStream(uri);
                            BitmapFactory.Options opt = new BitmapFactory.Options();
                            opt.inJustDecodeBounds = true;
                            BitmapFactory.decodeStream(is, null, opt);
                            is.close();
                            is =  mEST.getContext().getContentResolver().openInputStream(uri);
                            int width, height;
                            width = opt.outWidth;
                            height = opt.outHeight;
                            if (opt.outWidth > getMaxImageWidthPx()) {
                                width = getMaxImageWidthPx();
                                height = height * getMaxImageWidthPx() / opt.outWidth;
                                Rect padding = new Rect(0, 0, width, height);
                                bitmap = BitmapFactory.decodeStream(is, padding, null);
                            } else {
                                bitmap = BitmapFactory.decodeStream(is);
                            }
                            drawable = new BitmapDrawable(
                                    mEST.getContext().getResources(), bitmap);
                            drawable.setBounds(0, 0, width, height);
                            is.close();
                            return drawable;
                        } catch (Exception e) {
                            Log.e(TAG, "--- set html: Failed to loaded content " + uri, e);
                            return null;
                        } catch (OutOfMemoryError e) {
                            Log.e(TAG, "OutOfMemoryError");
                            mEST.setHint(HINT_MSG_BIG_SIZE_ERROR);

                            return null;
                        }
                    }
                    return null;
                }
            }, null);
            mEST.setText(spanned);
        }
    }

    private static class SoftKeyReceiver extends ResultReceiver {
        int mNewStart;
        int mNewEnd;
        EditStyledText mEST;

        SoftKeyReceiver(EditStyledText est) {
            super(est.getHandler());
            mEST = est;
        }

        @Override
        protected void onReceiveResult(int resultCode, Bundle resultData) {
            if (resultCode != InputMethodManager.RESULT_SHOWN) {
                Selection.setSelection(mEST.getText(), mNewStart, mNewEnd);
            }
        }
    }

    public static class SavedStyledTextState extends BaseSavedState {
        public int mBackgroundColor;

        SavedStyledTextState(Parcelable superState) {
            super(superState);
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeInt(mBackgroundColor);
        }

        @Override
        public String toString() {
            return "EditStyledText.SavedState{"
                    + Integer.toHexString(System.identityHashCode(this)) + " bgcolor="
                    + mBackgroundColor + "}";
        }
    }

    private static class StyledTextDialog {
        private static final int TYPE_FOREGROUND = 0;
        private static final int TYPE_BACKGROUND = 1;
        private Builder mBuilder;
        private AlertDialog mAlertDialog;
        private CharSequence mColorTitle;
        private CharSequence mSizeTitle;
        private CharSequence mAlignTitle;
        private CharSequence mMarqueeTitle;
        private CharSequence[] mColorNames;
        private CharSequence[] mColorInts;
        private CharSequence[] mSizeNames;
        private CharSequence[] mSizeDisplayInts;
        private CharSequence[] mSizeSendInts;
        private CharSequence[] mAlignNames;
        private CharSequence[] mMarqueeNames;
        private CharSequence mColorDefaultMessage;
        private EditStyledText mEST;

        public StyledTextDialog(EditStyledText est) {
            mEST = est;
        }

        public void setBuilder(Builder builder) {
            mBuilder = builder;
        }

        public void setColorAlertParams(CharSequence colortitle, CharSequence[] colornames,
                CharSequence[] colorInts, CharSequence defaultColorMessage) {
            mColorTitle = colortitle;
            mColorNames = colornames;
            mColorInts = colorInts;
            mColorDefaultMessage = defaultColorMessage;
        }

        public void setSizeAlertParams(CharSequence sizetitle, CharSequence[] sizenames,
                CharSequence[] sizedisplayints, CharSequence[] sizesendints) {
            mSizeTitle = sizetitle;
            mSizeNames = sizenames;
            mSizeDisplayInts = sizedisplayints;
            mSizeSendInts = sizesendints;
        }

        public void setAlignAlertParams(CharSequence aligntitle, CharSequence[] alignnames) {
            mAlignTitle = aligntitle;
            mAlignNames = alignnames;
        }

        public void setMarqueeAlertParams(CharSequence marqueetitle,
                CharSequence[] marqueenames) {
            mMarqueeTitle = marqueetitle;
            mMarqueeNames = marqueenames;
        }

        private boolean checkColorAlertParams() {
            if (DBG) {
                Log.d(TAG, "--- checkParams");
            }
            if (mBuilder == null) {
                Log.e(TAG, "--- builder is null.");
                return false;
            } else if (mColorTitle == null || mColorNames == null || mColorInts == null) {
                Log.e(TAG, "--- color alert params are null.");
                return false;
            } else if (mColorNames.length != mColorInts.length) {
                Log.e(TAG, "--- the length of color alert params are " + "different.");
                return false;
            }
            return true;
        }

        private boolean checkSizeAlertParams() {
            if (DBG) {
                Log.d(TAG, "--- checkParams");
            }
            if (mBuilder == null) {
                Log.e(TAG, "--- builder is null.");
                return false;
            } else if (mSizeTitle == null || mSizeNames == null || mSizeDisplayInts == null
                    || mSizeSendInts == null) {
                Log.e(TAG, "--- size alert params are null.");
                return false;
            } else if (mSizeNames.length != mSizeDisplayInts.length
                    && mSizeSendInts.length != mSizeDisplayInts.length) {
                Log.e(TAG, "--- the length of size alert params are " + "different.");
                return false;
            }
            return true;
        }

        private boolean checkAlignAlertParams() {
            if (DBG) {
                Log.d(TAG, "--- checkAlignAlertParams");
            }
            if (mBuilder == null) {
                Log.e(TAG, "--- builder is null.");
                return false;
            } else if (mAlignTitle == null) {
                Log.e(TAG, "--- align alert params are null.");
                return false;
            }
            return true;
        }

        private boolean checkMarqueeAlertParams() {
            if (DBG) {
                Log.d(TAG, "--- checkMarqueeAlertParams");
            }
            if (mBuilder == null) {
                Log.e(TAG, "--- builder is null.");
                return false;
            } else if (mMarqueeTitle == null) {
                Log.e(TAG, "--- Marquee alert params are null.");
                return false;
            }
            return true;
        }

        private void buildDialogue(CharSequence title, CharSequence[] names,
                DialogInterface.OnClickListener l) {
            mBuilder.setTitle(title);
            mBuilder.setIcon(0);
            mBuilder.setPositiveButton(null, null);
            mBuilder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mEST.onStartEdit();
                }
            });
            mBuilder.setItems(names, l);
            mBuilder.setView(null);
            mBuilder.setCancelable(true);
            mBuilder.setOnCancelListener(new OnCancelListener() {
                public void onCancel(DialogInterface arg0) {
                    if (DBG) {
                        Log.d(TAG, "--- oncancel");
                    }
                    mEST.onStartEdit();
                }
            });
            mBuilder.show();
        }

        private void buildAndShowColorDialogue(int type, CharSequence title, int[] colors) {
            final int HORIZONTAL_ELEMENT_NUM = 5;
            final int BUTTON_SIZE = mEST.dipToPx(50);
            final int BUTTON_MERGIN = mEST.dipToPx(2);
            final int BUTTON_PADDING = mEST.dipToPx(15);
            mBuilder.setTitle(title);
            mBuilder.setIcon(0);
            mBuilder.setPositiveButton(null, null);
            mBuilder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mEST.onStartEdit();
                }
            });
            mBuilder.setItems(null, null);
            LinearLayout verticalLayout = new LinearLayout(mEST.getContext());
            verticalLayout.setOrientation(LinearLayout.VERTICAL);
            verticalLayout.setGravity(Gravity.CENTER_HORIZONTAL);
            verticalLayout.setPadding(BUTTON_PADDING, BUTTON_PADDING, BUTTON_PADDING,
                    BUTTON_PADDING);
            LinearLayout horizontalLayout = null;
            for (int i = 0; i < colors.length; i++) {
                if (i % HORIZONTAL_ELEMENT_NUM == 0) {
                    horizontalLayout = new LinearLayout(mEST.getContext());
                    verticalLayout.addView(horizontalLayout);
                }
                Button button = new Button(mEST.getContext());
                button.setHeight(BUTTON_SIZE);
                button.setWidth(BUTTON_SIZE);
                ColorPaletteDrawable cp =
                        new ColorPaletteDrawable(colors[i], BUTTON_SIZE, BUTTON_SIZE,
                                BUTTON_MERGIN);
                button.setBackgroundDrawable(cp);
                button.setDrawingCacheBackgroundColor(colors[i]);
                if (type == TYPE_FOREGROUND) {
                    button.setOnClickListener(new View.OnClickListener() {
                        public void onClick(View view) {
                            mEST.setItemColor(view.getDrawingCacheBackgroundColor());
                            if (mAlertDialog != null) {
                                mAlertDialog.setView(null);
                                mAlertDialog.dismiss();
                                mAlertDialog = null;
                            } else {
                                Log.e(TAG,
                                        "--- buildAndShowColorDialogue: can't find alertDialog");
                            }
                        }
                    });
                } else if (type == TYPE_BACKGROUND) {
                    button.setOnClickListener(new View.OnClickListener() {
                        public void onClick(View view) {
                            mEST.setBackgroundColor(view.getDrawingCacheBackgroundColor());
                            if (mAlertDialog != null) {
                                mAlertDialog.setView(null);
                                mAlertDialog.dismiss();
                                mAlertDialog = null;
                            } else {
                                Log.e(TAG,
                                        "--- buildAndShowColorDialogue: can't find alertDialog");
                            }
                        }
                    });
                }
                horizontalLayout.addView(button);
            }

            if (type == TYPE_BACKGROUND) {
                mBuilder.setPositiveButton(mColorDefaultMessage,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                mEST.setBackgroundColor(DEFAULT_TRANSPARENT_COLOR);
                            }
                        });
            } else if (type == TYPE_FOREGROUND) {
                mBuilder.setPositiveButton(mColorDefaultMessage,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                mEST.setItemColor(DEFAULT_FOREGROUND_COLOR);
                            }
                        });
            }

            mBuilder.setView(verticalLayout);
            mBuilder.setCancelable(true);
            mBuilder.setOnCancelListener(new OnCancelListener() {
                public void onCancel(DialogInterface arg0) {
                    mEST.onStartEdit();
                }
            });
            mAlertDialog = mBuilder.show();
        }

        private void onShowForegroundColorAlertDialog() {
            if (DBG) {
                Log.d(TAG, "--- onShowForegroundColorAlertDialog");
            }
            if (!checkColorAlertParams()) {
                return;
            }
            int[] colorints = new int[mColorInts.length];
            for (int i = 0; i < colorints.length; i++) {
                colorints[i] = Integer.parseInt((String) mColorInts[i], 16) - 0x01000000;
            }
            buildAndShowColorDialogue(TYPE_FOREGROUND, mColorTitle, colorints);
        }

        private void onShowBackgroundColorAlertDialog() {
            if (DBG) {
                Log.d(TAG, "--- onShowBackgroundColorAlertDialog");
            }
            if (!checkColorAlertParams()) {
                return;
            }
            int[] colorInts = new int[mColorInts.length];
            for (int i = 0; i < colorInts.length; i++) {
                colorInts[i] = Integer.parseInt((String) mColorInts[i], 16) - 0x01000000;
            }
            buildAndShowColorDialogue(TYPE_BACKGROUND, mColorTitle, colorInts);
        }

        private void onShowSizeAlertDialog() {
            if (DBG) {
                Log.d(TAG, "--- onShowSizeAlertDialog");
            }
            if (!checkSizeAlertParams()) {
                return;
            }
            buildDialogue(mSizeTitle, mSizeNames, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    Log.d(TAG, "mBuilder.onclick:" + which);
                    int size =
                            mEST.dipToPx(Integer.parseInt((String) mSizeDisplayInts[which]));
                    mEST.setItemSize(size);
                }
            });
        }

        private void onShowAlignAlertDialog() {
            if (DBG) {
                Log.d(TAG, "--- onShowAlignAlertDialog");
            }
            if (!checkAlignAlertParams()) {
                return;
            }
            buildDialogue(mAlignTitle, mAlignNames, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    Layout.Alignment align = Layout.Alignment.ALIGN_NORMAL;
                    switch (which) {
                        case 0:
                            align = Layout.Alignment.ALIGN_NORMAL;
                            break;
                        case 1:
                            align = Layout.Alignment.ALIGN_CENTER;
                            break;
                        case 2:
                            align = Layout.Alignment.ALIGN_OPPOSITE;
                            break;
                        default:
                            Log.e(TAG, "--- onShowAlignAlertDialog: got illigal align.");
                            break;
                    }
                    mEST.setAlignment(align);
                }
            });
        }

        private void onShowMarqueeAlertDialog() {
            if (DBG) {
                Log.d(TAG, "--- onShowMarqueeAlertDialog");
            }
            if (!checkMarqueeAlertParams()) {
                return;
            }
            buildDialogue(mMarqueeTitle, mMarqueeNames, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    if (DBG) {
                        Log.d(TAG, "mBuilder.onclick:" + which);
                    }
                    mEST.setMarquee(which);
                }
            });
        }
    }

    private class MenuHandler implements MenuItem.OnMenuItemClickListener {
        public boolean onMenuItemClick(MenuItem item) {
            return onTextContextMenuItem(item.getItemId());
        }
    }

    private static class StyledTextArrowKeyMethod extends ArrowKeyMovementMethod {
        EditorManager mManager;
        String LOG_TAG = "StyledTextArrowKeyMethod";

        StyledTextArrowKeyMethod(EditorManager manager) {
            super();
            mManager = manager;
        }

        @Override
        public boolean
                onKeyDown(TextView widget, Spannable buffer, int keyCode, KeyEvent event) {
            if (DBG) {
                Log.d(LOG_TAG, "---onkeydown:" + keyCode);
            }
            mManager.unsetTextComposingMask();
            if (mManager.getSelectState() == STATE_SELECT_ON
                    || mManager.getSelectState() == STATE_SELECTED) {
                return executeDown(widget, buffer, keyCode);
            } else {
                return super.onKeyDown(widget, buffer, keyCode, event);
            }
        }

        private int getEndPos(TextView widget) {
            int end;
            if (widget.getSelectionStart() == mManager.getSelectionStart()) {
                end = widget.getSelectionEnd();
            } else {
                end = widget.getSelectionStart();
            }
            return end;
        }

        protected boolean up(TextView widget, Spannable buffer) {
            if (DBG) {
                Log.d(LOG_TAG, "--- up:");
            }
            Layout layout = widget.getLayout();
            int end = getEndPos(widget);
            int line = layout.getLineForOffset(end);
            if (line > 0) {
                int to;
                if (layout.getParagraphDirection(line) == layout
                        .getParagraphDirection(line - 1)) {
                    float h = layout.getPrimaryHorizontal(end);
                    to = layout.getOffsetForHorizontal(line - 1, h);
                } else {
                    to = layout.getLineStart(line - 1);
                }
                mManager.setEndPos(to);
                mManager.onCursorMoved();
            }
            return true;
        }

        protected boolean down(TextView widget, Spannable buffer) {
            if (DBG) {
                Log.d(LOG_TAG, "--- down:");
            }
            Layout layout = widget.getLayout();
            int end = getEndPos(widget);
            int line = layout.getLineForOffset(end);
            if (line < layout.getLineCount() - 1) {
                int to;
                if (layout.getParagraphDirection(line) == layout
                        .getParagraphDirection(line + 1)) {
                    float h = layout.getPrimaryHorizontal(end);
                    to = layout.getOffsetForHorizontal(line + 1, h);
                } else {
                    to = layout.getLineStart(line + 1);
                }
                mManager.setEndPos(to);
                mManager.onCursorMoved();
            }
            return true;
        }

        protected boolean left(TextView widget, Spannable buffer) {
            if (DBG) {
                Log.d(LOG_TAG, "--- left:");
            }
            Layout layout = widget.getLayout();
            int to = layout.getOffsetToLeftOf(getEndPos(widget));
            mManager.setEndPos(to);
            mManager.onCursorMoved();
            return true;
        }

        protected boolean right(TextView widget, Spannable buffer) {
            if (DBG) {
                Log.d(LOG_TAG, "--- right:");
            }
            Layout layout = widget.getLayout();
            int to = layout.getOffsetToRightOf(getEndPos(widget));
            mManager.setEndPos(to);
            mManager.onCursorMoved();
            return true;
        }

        private boolean executeDown(TextView widget, Spannable buffer, int keyCode) {
            if (DBG) {
                Log.d(LOG_TAG, "--- executeDown: " + keyCode);
            }
            boolean handled = false;

            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_UP:
                    handled |= up(widget, buffer);
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    handled |= down(widget, buffer);
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    handled |= left(widget, buffer);
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    handled |= right(widget, buffer);
                    break;
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    mManager.onFixSelectedItem();
                    handled = true;
                    break;
            }
            return handled;
        }
    }

    public static class StyledTextInputConnection extends InputConnectionWrapper {
        EditStyledText mEST;

        public StyledTextInputConnection(InputConnection target, EditStyledText est) {
            super(target, true);
            mEST = est;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            if (DBG) {
                Log.d(TAG, "--- commitText:");
            }
            mEST.mManager.unsetTextComposingMask();
            return super.commitText(text, newCursorPosition);
        }

        @Override
        public boolean finishComposingText() {
            if (DBG) {
                Log.d(TAG, "--- finishcomposing:");
            }
            if (!mEST.isSoftKeyBlocked() && !mEST.isButtonsFocused() && !mEST.isEditting()) {
                // TODO onEndEdit isn't called temporally .
                mEST.onEndEdit();
            }
            return super.finishComposingText();
        }
    }

    public static class EditStyledTextSpans {
        private static final String LOG_TAG = "EditStyledTextSpan";

        public static class HorizontalLineSpan extends DynamicDrawableSpan {
            HorizontalLineDrawable mDrawable;

            public HorizontalLineSpan(int color, int width, Spannable spannable) {
                super(ALIGN_BOTTOM);
                mDrawable = new HorizontalLineDrawable(color, width, spannable);
            }

            @Override
            public Drawable getDrawable() {
                return mDrawable;
            }

            public void resetWidth(int width) {
                mDrawable.renewBounds(width);
            }

            public int getColor() {
                return mDrawable.getPaint().getColor();
            }
        }

        public static class MarqueeSpan extends CharacterStyle {
            public static final int SCROLL = 0;
            public static final int ALTERNATE = 1;
            public static final int NOTHING = 2;
            private int mType;
            private int mMarqueeColor;

            public MarqueeSpan(int type, int bgc) {
                mType = type;
                checkType(type);
                mMarqueeColor = getMarqueeColor(type, bgc);
            }

            public MarqueeSpan(int type) {
                this(type, EditStyledText.DEFAULT_TRANSPARENT_COLOR);
            }

            public int getType() {
                return mType;
            }

            public void resetColor(int bgc) {
                mMarqueeColor = getMarqueeColor(mType, bgc);
            }

            private int getMarqueeColor(int type, int bgc) {
                int THRESHOLD = 128;
                int a = Color.alpha(bgc);
                int r = Color.red(bgc);
                int g = Color.green(bgc);
                int b = Color.blue(bgc);
                if (a == 0) {
                    a = 0x80;
                }
                switch (type) {
                    case SCROLL:
                        if (r > THRESHOLD) {
                            r = r / 2;
                        } else {
                            r = (0XFF - r) / 2;
                        }
                        break;
                    case ALTERNATE:
                        if (g > THRESHOLD) {
                            g = g / 2;
                        } else {
                            g = (0XFF - g) / 2;
                        }
                        break;
                    case NOTHING:
                        return DEFAULT_TRANSPARENT_COLOR;
                    default:
                        Log.e(TAG, "--- getMarqueeColor: got illigal marquee ID.");
                        return DEFAULT_TRANSPARENT_COLOR;
                }
                return Color.argb(a, r, g, b);
            }

            private boolean checkType(int type) {
                if (type == SCROLL || type == ALTERNATE) {
                    return true;
                } else {
                    Log.e(LOG_TAG, "--- Invalid type of MarqueeSpan");
                    return false;
                }
            }

            @Override
            public void updateDrawState(TextPaint tp) {
                tp.bgColor = mMarqueeColor;
            }
        }

        public static class RescalableImageSpan extends ImageSpan {
            Uri mContentUri;
            private Drawable mDrawable;
            private Context mContext;
            public int mIntrinsicWidth = -1;
            public int mIntrinsicHeight = -1;
            private final int MAXWIDTH;

            public RescalableImageSpan(Context context, Uri uri, int maxwidth) {
                super(context, uri);
                mContext = context;
                mContentUri = uri;
                MAXWIDTH = maxwidth;
            }

            public RescalableImageSpan(Context context, int resourceId, int maxwidth) {
                super(context, resourceId);
                mContext = context;
                MAXWIDTH = maxwidth;
            }

            @Override
            public Drawable getDrawable() {
                if (mDrawable != null) {
                    return mDrawable;
                } else if (mContentUri != null) {
                    Bitmap bitmap = null;
                    System.gc();
                    try {
                        InputStream is =
                                mContext.getContentResolver().openInputStream(mContentUri);
                        BitmapFactory.Options opt = new BitmapFactory.Options();
                        opt.inJustDecodeBounds = true;
                        BitmapFactory.decodeStream(is, null, opt);
                        is.close();
                        is = mContext.getContentResolver().openInputStream(mContentUri);
                        int width, height;
                        width = opt.outWidth;
                        height = opt.outHeight;
                        mIntrinsicWidth = width;
                        mIntrinsicHeight = height;
                        if (opt.outWidth > MAXWIDTH) {
                            width = MAXWIDTH;
                            height = height * MAXWIDTH / opt.outWidth;
                            Rect padding = new Rect(0, 0, width, height);
                            bitmap = BitmapFactory.decodeStream(is, padding, null);
                        } else {
                            bitmap = BitmapFactory.decodeStream(is);
                        }
                        mDrawable = new BitmapDrawable(mContext.getResources(), bitmap);
                        mDrawable.setBounds(0, 0, width, height);
                        is.close();
                    } catch (Exception e) {
                        Log.e(LOG_TAG, "Failed to loaded content " + mContentUri, e);
                        return null;
                    } catch (OutOfMemoryError e) {
                        Log.e(LOG_TAG, "OutOfMemoryError");
                        return null;
                    }
                } else {
                    mDrawable = super.getDrawable();
                    rescaleBigImage(mDrawable);
                    mIntrinsicWidth = mDrawable.getIntrinsicWidth();
                    mIntrinsicHeight = mDrawable.getIntrinsicHeight();
                }
                return mDrawable;
            }

            public boolean isOverSize() {
                return (getDrawable().getIntrinsicWidth() > MAXWIDTH);
            }

            public Uri getContentUri() {
                return mContentUri;
            }

            private void rescaleBigImage(Drawable image) {
                if (DBG) {
                    Log.d(LOG_TAG, "--- rescaleBigImage:");
                }
                if (MAXWIDTH < 0) {
                    return;
                }
                int image_width = image.getIntrinsicWidth();
                int image_height = image.getIntrinsicHeight();
                if (DBG) {
                    Log.d(LOG_TAG, "--- rescaleBigImage:" + image_width + "," + image_height
                            + "," + MAXWIDTH);
                }
                if (image_width > MAXWIDTH) {
                    image_width = MAXWIDTH;
                    image_height = image_height * MAXWIDTH / image_width;
                }
                image.setBounds(0, 0, image_width, image_height);
            }
        }

        public static class HorizontalLineDrawable extends ShapeDrawable {
            private Spannable mSpannable;
            private int mWidth;
            private static boolean DBG_HL = false;

            public HorizontalLineDrawable(int color, int width, Spannable spannable) {
                super(new RectShape());
                mSpannable = spannable;
                mWidth = width;
                renewColor(color);
                renewBounds(width);
            }

            @Override
            public void draw(Canvas canvas) {
                renewColor();
                Rect rect = new Rect(0, 9, mWidth, 11);
                canvas.drawRect(rect, getPaint());
            }

            public void renewBounds(int width) {
                int MARGIN = 20;
                int HEIGHT = 20;
                if (DBG_HL) {
                    Log.d(LOG_TAG, "--- renewBounds:" + width);
                }
                if (width > MARGIN) {
                    width -= MARGIN;
                }
                mWidth = width;
                setBounds(0, 0, width, HEIGHT);
            }

            private void renewColor(int color) {
                if (DBG_HL) {
                    Log.d(LOG_TAG, "--- renewColor:" + color);
                }
                getPaint().setColor(color);
            }

            private void renewColor() {
                HorizontalLineSpan parent = getParentSpan();
                Spannable text = mSpannable;
                int start = text.getSpanStart(parent);
                int end = text.getSpanEnd(parent);
                ForegroundColorSpan[] spans =
                        text.getSpans(start, end, ForegroundColorSpan.class);
                if (DBG_HL) {
                    Log.d(LOG_TAG, "--- renewColor:" + spans.length);
                }
                if (spans.length > 0) {
                    renewColor(spans[spans.length - 1].getForegroundColor());
                }
            }

            private HorizontalLineSpan getParentSpan() {
                Spannable text = mSpannable;
                HorizontalLineSpan[] images =
                        text.getSpans(0, text.length(), HorizontalLineSpan.class);
                if (images.length > 0) {
                    for (HorizontalLineSpan image : images) {
                        if (image.getDrawable() == this) {
                            return image;
                        }
                    }
                }
                Log.e(LOG_TAG, "---renewBounds: Couldn't find");
                return null;
            }
        }
    }

    public static class ColorPaletteDrawable extends ShapeDrawable {
        private Rect mRect;

        public ColorPaletteDrawable(int color, int width, int height, int mergin) {
            super(new RectShape());
            mRect = new Rect(mergin, mergin, width - mergin, height - mergin);
            getPaint().setColor(color);
        }

        @Override
        public void draw(Canvas canvas) {
            canvas.drawRect(mRect, getPaint());
        }
    }

    public class EditModeActions {

        private static final String TAG = "EditModeActions";
        private static final boolean DBG = true;
        private EditStyledText mEST;
        private EditorManager mManager;
        private StyledTextDialog mDialog;

        private int mMode = EditStyledText.MODE_NOTHING;

        private HashMap<Integer, EditModeActionBase> mActionMap =
                new HashMap<Integer, EditModeActionBase>();

        private NothingAction mNothingAction = new NothingAction();
        private CopyAction mCopyAction = new CopyAction();
        private PasteAction mPasteAction = new PasteAction();
        private SelectAction mSelectAction = new SelectAction();
        private CutAction mCutAction = new CutAction();
        private SelectAllAction mSelectAllAction = new SelectAllAction();
        private HorizontalLineAction mHorizontalLineAction = new HorizontalLineAction();
        private StopSelectionAction mStopSelectionAction = new StopSelectionAction();
        private ClearStylesAction mClearStylesAction = new ClearStylesAction();
        private ImageAction mImageAction = new ImageAction();
        private BackgroundColorAction mBackgroundColorAction = new BackgroundColorAction();
        private PreviewAction mPreviewAction = new PreviewAction();
        private CancelAction mCancelEditAction = new CancelAction();
        private TextViewAction mTextViewAction = new TextViewAction();
        private StartEditAction mStartEditAction = new StartEditAction();
        private EndEditAction mEndEditAction = new EndEditAction();
        private ResetAction mResetAction = new ResetAction();
        private ShowMenuAction mShowMenuAction = new ShowMenuAction();
        private AlignAction mAlignAction = new AlignAction();
        private TelopAction mTelopAction = new TelopAction();
        private SwingAction mSwingAction = new SwingAction();
        private MarqueeDialogAction mMarqueeDialogAction = new MarqueeDialogAction();
        private ColorAction mColorAction = new ColorAction();
        private SizeAction mSizeAction = new SizeAction();

        EditModeActions(EditStyledText est, EditorManager manager, StyledTextDialog dialog) {
            mEST = est;
            mManager = manager;
            mDialog = dialog;
            mActionMap.put(EditStyledText.MODE_NOTHING, mNothingAction);
            mActionMap.put(EditStyledText.MODE_COPY, mCopyAction);
            mActionMap.put(EditStyledText.MODE_PASTE, mPasteAction);
            mActionMap.put(EditStyledText.MODE_SELECT, mSelectAction);
            mActionMap.put(EditStyledText.MODE_CUT, mCutAction);
            mActionMap.put(EditStyledText.MODE_SELECTALL, mSelectAllAction);
            mActionMap.put(EditStyledText.MODE_HORIZONTALLINE, mHorizontalLineAction);
            mActionMap.put(EditStyledText.MODE_STOP_SELECT, mStopSelectionAction);
            mActionMap.put(EditStyledText.MODE_CLEARSTYLES, mClearStylesAction);
            mActionMap.put(EditStyledText.MODE_IMAGE, mImageAction);
            mActionMap.put(EditStyledText.MODE_BGCOLOR, mBackgroundColorAction);
            mActionMap.put(EditStyledText.MODE_PREVIEW, mPreviewAction);
            mActionMap.put(EditStyledText.MODE_CANCEL, mCancelEditAction);
            mActionMap.put(EditStyledText.MODE_TEXTVIEWFUNCTION, mTextViewAction);
            mActionMap.put(EditStyledText.MODE_START_EDIT, mStartEditAction);
            mActionMap.put(EditStyledText.MODE_END_EDIT, mEndEditAction);
            mActionMap.put(EditStyledText.MODE_RESET, mResetAction);
            mActionMap.put(EditStyledText.MODE_SHOW_MENU, mShowMenuAction);
            mActionMap.put(EditStyledText.MODE_ALIGN, mAlignAction);
            mActionMap.put(EditStyledText.MODE_TELOP, mTelopAction);
            mActionMap.put(EditStyledText.MODE_SWING, mSwingAction);
            mActionMap.put(EditStyledText.MODE_MARQUEE, mMarqueeDialogAction);
            mActionMap.put(EditStyledText.MODE_COLOR, mColorAction);
            mActionMap.put(EditStyledText.MODE_SIZE, mSizeAction);
        }

        public void addAction(int modeId, EditModeActionBase action) {
            mActionMap.put(modeId, action);
        }

        public void onAction(int newMode, Object[] params) {
            getAction(newMode).addParams(params);
            mMode = newMode;
            doNext(newMode);
        }

        public void onAction(int newMode, Object param) {
            onAction(newMode, new Object[] { param });
        }

        public void onAction(int newMode) {
            onAction(newMode, null);
        }

        public void onSelectAction() {
            doNext(EditStyledText.MODE_SELECT);
        }

        private EditModeActionBase getAction(int mode) {
            if (mActionMap.containsKey(mode)) {
                return mActionMap.get(mode);
            }
            return null;
        }

        public boolean doNext() {
            return doNext(mMode);
        }

        public boolean doNext(int mode) {
            if (DBG) {
                Log.d(TAG, "--- do the next action: " + mode + "," + mManager.getSelectState());
            }
            EditModeActionBase action = getAction(mode);
            if (action == null) {
                Log.e(TAG, "--- invalid action error.");
                return false;
            }
            switch (mManager.getSelectState()) {
                case EditStyledText.STATE_SELECT_OFF:
                    return action.doNotSelected();
                case EditStyledText.STATE_SELECT_ON:
                    return action.doStartPosIsSelected();
                case EditStyledText.STATE_SELECTED:
                    return action.doEndPosIsSelected();
                case EditStyledText.STATE_SELECT_FIX:
                    if (mManager.isWaitInput()) {
                        return action.doSelectionIsFixedAndWaitingInput();
                    } else {
                        return action.doSelectionIsFixed();
                    }
                default:
                    return false;
            }
        }

        public class EditModeActionBase {
            private Object[] mParams;

            protected boolean canOverWrap() {
                return false;
            }

            protected boolean canSelect() {
                return false;
            }

            protected boolean canWaitInput() {
                return false;
            }

            protected boolean needSelection() {
                return false;
            }

            protected boolean isLine() {
                return false;
            }

            protected boolean doNotSelected() {
                return false;
            }

            protected boolean doStartPosIsSelected() {
                return doNotSelected();
            }

            protected boolean doEndPosIsSelected() {
                return doStartPosIsSelected();
            }

            protected boolean doSelectionIsFixed() {
                return doEndPosIsSelected();
            }

            protected boolean doSelectionIsFixedAndWaitingInput() {
                return doEndPosIsSelected();
            }

            protected boolean fixSelection() {
                mEST.finishComposingText();
                mManager.setSelectState(EditStyledText.STATE_SELECT_FIX);
                return true;
            }

            protected void addParams(Object[] o) {
                mParams = o;
            }

            protected Object getParam(int num) {
                if (mParams == null || num > mParams.length) {
                    if (DBG) {
                        Log.d(TAG, "--- Number of the parameter is out of bound.");
                    }
                    return null;
                } else {
                    return mParams[num];
                }
            }
        }

        public class NothingAction extends EditModeActionBase {
        }

        public class TextViewActionBase extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                if (mManager.getEditMode() == EditStyledText.MODE_NOTHING
                        || mManager.getEditMode() == EditStyledText.MODE_SELECT) {
                    mManager.setEditMode(mMode);
                    onSelectAction();
                    return true;
                }
                return false;
            }

            @Override
            protected boolean doEndPosIsSelected() {
                if (mManager.getEditMode() == EditStyledText.MODE_NOTHING
                        || mManager.getEditMode() == EditStyledText.MODE_SELECT) {
                    mManager.setEditMode(mMode);
                    fixSelection();
                    doNext();
                    return true;
                } else if (mManager.getEditMode() != mMode) {
                    mManager.resetEdit();
                    mManager.setEditMode(mMode);
                    doNext();
                    return true;
                }
                return false;
            }
        }

        public class TextViewAction extends TextViewActionBase {
            @Override
            protected boolean doEndPosIsSelected() {
                if (super.doEndPosIsSelected()) {
                    return true;
                }
                Object param = getParam(0);
                if (param != null && param instanceof Integer) {
                    mEST.onTextContextMenuItem((Integer) param);
                }
                mManager.resetEdit();
                return true;
            }
        }

        public class CopyAction extends TextViewActionBase {
            @Override
            protected boolean doEndPosIsSelected() {
                if (super.doEndPosIsSelected()) {
                    return true;
                }
                mManager.copyToClipBoard();
                mManager.resetEdit();
                return true;
            }
        }

        public class CutAction extends TextViewActionBase {
            @Override
            protected boolean doEndPosIsSelected() {
                if (super.doEndPosIsSelected()) {
                    return true;
                }
                mManager.cutToClipBoard();
                mManager.resetEdit();
                return true;
            }
        }

        public class SelectAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                if (mManager.isTextSelected()) {
                    Log.e(TAG, "Selection is off, but selected");
                }
                mManager.setSelectStartPos();
                mEST.sendHintMessage(EditStyledText.HINT_MSG_SELECT_END);
                return true;
            }

            @Override
            protected boolean doStartPosIsSelected() {
                if (mManager.isTextSelected()) {
                    Log.e(TAG, "Selection now start, but selected");
                }
                mManager.setSelectEndPos();
                mEST.sendHintMessage(EditStyledText.HINT_MSG_PUSH_COMPETE);
                if (mManager.getEditMode() != EditStyledText.MODE_SELECT) {
                    doNext(mManager.getEditMode()); // doNextHandle needs edit mode in editor.
                }
                return true;
            }

            @Override
            protected boolean doSelectionIsFixed() {
                return false;
            }
        }

        public class PasteAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.pasteFromClipboard();
                mManager.resetEdit();
                return true;
            }
        }

        public class SelectAllAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.selectAll();
                return true;
            }
        }

        public class HorizontalLineAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.insertHorizontalLine();
                return true;
            }
        }

        public class ClearStylesAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.clearStyles();
                return true;
            }
        }

        public class StopSelectionAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.fixSelectionAndDoNextAction();
                return true;
            }
        }

        public class CancelAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mEST.cancelViewManagers();
                return true;
            }
        }

        public class ImageAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                Object param = getParam(0);
                if (param != null) {
                    if (param instanceof Uri) {
                        mManager.insertImageFromUri((Uri) param);
                    } else if (param instanceof Integer) {
                        mManager.insertImageFromResId((Integer) param);
                    }
                } else {
                    mEST.showInsertImageSelectAlertDialog();
                }
                return true;
            }
        }

        public class BackgroundColorAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mDialog.onShowBackgroundColorAlertDialog();
                return true;
            }
        }

        public class PreviewAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mEST.showPreview();
                return true;
            }
        }

        public class StartEditAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.startEdit();
                return true;
            }
        }

        public class EndEditAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.endEdit();
                return true;
            }
        }

        public class ResetAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mManager.resetEdit();
                return true;
            }
        }

        public class ShowMenuAction extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                mEST.showMenuAlertDialog();
                return true;
            }
        }

        public class SetSpanActionBase extends EditModeActionBase {
            @Override
            protected boolean doNotSelected() {
                if (mManager.getEditMode() == EditStyledText.MODE_NOTHING
                        || mManager.getEditMode() == EditStyledText.MODE_SELECT) {
                    mManager.setEditMode(mMode);
                    mManager.setInternalSelection(mEST.getSelectionStart(),
                            mEST.getSelectionEnd());
                    fixSelection();
                    doNext();
                    return true;
                } else if (mManager.getEditMode() != mMode) {
                    Log.d(TAG, "--- setspanactionbase" + mManager.getEditMode() + "," + mMode);
                    if (!mManager.isWaitInput()) {
                        mManager.resetEdit();
                        mManager.setEditMode(mMode);
                    } else {
                        mManager.setEditMode(EditStyledText.MODE_NOTHING);
                        mManager.setSelectState(EditStyledText.STATE_SELECT_OFF);
                    }
                    doNext();
                    return true;
                }
                return false;
            }

            @Override
            protected boolean doStartPosIsSelected() {
                if (mManager.getEditMode() == EditStyledText.MODE_NOTHING
                        || mManager.getEditMode() == EditStyledText.MODE_SELECT) {
                    mManager.setEditMode(mMode);
                    onSelectAction();
                    return true;
                }
                return doNotSelected();
            }

            @Override
            protected boolean doEndPosIsSelected() {
                if (mManager.getEditMode() == EditStyledText.MODE_NOTHING
                        || mManager.getEditMode() == EditStyledText.MODE_SELECT) {
                    mManager.setEditMode(mMode);
                    fixSelection();
                    doNext();
                    return true;
                }
                return doStartPosIsSelected();
            }

            @Override
            protected boolean doSelectionIsFixed() {
                if (doEndPosIsSelected()) {
                    return true;
                }
                mEST.sendHintMessage(EditStyledText.HINT_MSG_NULL);

                return false;
            }
        }

        public class AlignAction extends SetSpanActionBase {
            @Override
            protected boolean doSelectionIsFixed() {
                if (super.doSelectionIsFixed()) {
                    return true;
                }
                mDialog.onShowAlignAlertDialog();
                return true;
            }
        }

        public class TelopAction extends SetSpanActionBase {
            @Override
            protected boolean doSelectionIsFixed() {
                if (super.doSelectionIsFixed()) {
                    return true;
                }
                mManager.setTelop();
                return true;
            }
        }

        public class SwingAction extends SetSpanActionBase {
            @Override
            protected boolean doSelectionIsFixed() {
                if (super.doSelectionIsFixed()) {
                    return true;
                }
                mManager.setSwing();
                return true;
            }
        }

        public class MarqueeDialogAction extends SetSpanActionBase {
            @Override
            protected boolean doSelectionIsFixed() {
                if (super.doSelectionIsFixed()) {
                    return true;
                }
                mDialog.onShowMarqueeAlertDialog();
                return true;
            }
        }

        public class ColorAction extends SetSpanActionBase {
            @Override
            protected boolean doSelectionIsFixed() {
                if (super.doSelectionIsFixed()) {
                    return true;
                }
                mDialog.onShowForegroundColorAlertDialog();
                return true;
            }

            @Override
            protected boolean doSelectionIsFixedAndWaitingInput() {
                if (super.doSelectionIsFixedAndWaitingInput()) {
                    return true;
                }
                int size = mManager.getSizeWaitInput();
                mManager.setItemColor(mManager.getColorWaitInput(), false);
                // selection was resumed
                if (!mManager.isWaitInput()) {
                    mManager.setItemSize(size, false);
                    mManager.resetEdit();
                } else {
                    fixSelection();
                    mDialog.onShowForegroundColorAlertDialog();
                }
                return true;
            }
        }

        public class SizeAction extends SetSpanActionBase {
            @Override
            protected boolean doSelectionIsFixed() {
                if (super.doSelectionIsFixed()) {
                    return true;
                }
                mDialog.onShowSizeAlertDialog();
                return true;
            }

            @Override
            protected boolean doSelectionIsFixedAndWaitingInput() {
                if (super.doSelectionIsFixedAndWaitingInput()) {
                    return true;
                }
                int color = mManager.getColorWaitInput();
                mManager.setItemSize(mManager.getSizeWaitInput(), false);
                if (!mManager.isWaitInput()) {
                    mManager.setItemColor(color, false);
                    mManager.resetEdit();
                } else {
                    fixSelection();
                    mDialog.onShowSizeAlertDialog();
                }
                return true;
            }
        }
    }
}
