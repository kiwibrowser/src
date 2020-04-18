/*

 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.ex.chips;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.DialogFragment;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader.TileMode;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.text.Editable;
import android.text.InputType;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.method.QwertyKeyListener;
import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ActionMode;
import android.view.ActionMode.Callback;
import android.view.DragEvent;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewParent;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Filterable;
import android.widget.ListAdapter;
import android.widget.ListPopupWindow;
import android.widget.ListView;
import android.widget.MultiAutoCompleteTextView;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.TextView;

import com.android.ex.chips.DropdownChipLayouter.PermissionRequestDismissedListener;
import com.android.ex.chips.RecipientAlternatesAdapter.RecipientMatchCallback;
import com.android.ex.chips.recipientchip.DrawableRecipientChip;
import com.android.ex.chips.recipientchip.InvisibleRecipientChip;
import com.android.ex.chips.recipientchip.ReplacementDrawableSpan;
import com.android.ex.chips.recipientchip.VisibleRecipientChip;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * RecipientEditTextView is an auto complete text view for use with applications
 * that use the new Chips UI for addressing a message to recipients.
 */
public class RecipientEditTextView extends MultiAutoCompleteTextView implements
        OnItemClickListener, Callback, RecipientAlternatesAdapter.OnCheckedItemChangedListener,
        GestureDetector.OnGestureListener, TextView.OnEditorActionListener,
        DropdownChipLayouter.ChipDeleteListener, PermissionRequestDismissedListener {
    private static final String TAG = "RecipientEditTextView";

    private static final char COMMIT_CHAR_COMMA = ',';
    private static final char COMMIT_CHAR_SEMICOLON = ';';
    private static final char COMMIT_CHAR_SPACE = ' ';
    private static final String SEPARATOR = String.valueOf(COMMIT_CHAR_COMMA)
            + String.valueOf(COMMIT_CHAR_SPACE);

    private static final int DISMISS = "dismiss".hashCode();
    private static final long DISMISS_DELAY = 300;

    // TODO: get correct number/ algorithm from with UX.
    // Visible for testing.
    /*package*/ static final int CHIP_LIMIT = 2;

    private static final int MAX_CHIPS_PARSED = 50;

    private int mUnselectedChipTextColor;
    private int mUnselectedChipBackgroundColor;

    // Work variables to avoid re-allocation on every typed character.
    private final Rect mRect = new Rect();
    private final int[] mCoords = new int[2];

    // Resources for displaying chips.
    private Drawable mChipBackground = null;
    private Drawable mChipDelete = null;
    private Drawable mInvalidChipBackground;

    // Possible attr overrides
    private float mChipHeight;
    private float mChipFontSize;
    private float mLineSpacingExtra;
    private int mChipTextStartPadding;
    private int mChipTextEndPadding;
    private final int mTextHeight;
    private boolean mDisableDelete;
    private int mMaxLines;

    /**
     * Enumerator for avatar position. See attr.xml for more details.
     * 0 for end, 1 for start.
     */
    private int mAvatarPosition;
    private static final int AVATAR_POSITION_END = 0;
    private static final int AVATAR_POSITION_START = 1;

    private Paint mWorkPaint = new Paint();

    private Tokenizer mTokenizer;
    private Validator mValidator;
    private Handler mHandler;
    private TextWatcher mTextWatcher;
    protected DropdownChipLayouter mDropdownChipLayouter;

    private View mDropdownAnchor = this;
    private ListPopupWindow mAlternatesPopup;
    private ListPopupWindow mAddressPopup;
    private View mAlternatePopupAnchor;
    private OnItemClickListener mAlternatesListener;

    private DrawableRecipientChip mSelectedChip;
    private Bitmap mDefaultContactPhoto;
    private ReplacementDrawableSpan mMoreChip;
    private TextView mMoreItem;

    private int mCurrentSuggestionCount;

    // VisibleForTesting
    final ArrayList<String> mPendingChips = new ArrayList<String>();

    private int mPendingChipsCount = 0;
    private int mCheckedItem;
    private boolean mNoChipMode = false;
    private boolean mShouldShrink = true;
    private boolean mRequiresShrinkWhenNotGone = false;

    // VisibleForTesting
    ArrayList<DrawableRecipientChip> mTemporaryRecipients;

    private ArrayList<DrawableRecipientChip> mHiddenSpans;

    // Chip copy fields.
    private GestureDetector mGestureDetector;

    // Obtain the enclosing scroll view, if it exists, so that the view can be
    // scrolled to show the last line of chips content.
    private ScrollView mScrollView;
    private boolean mTriedGettingScrollView;
    private boolean mDragEnabled = false;

    private boolean mAttachedToWindow;

    private final Runnable mAddTextWatcher = new Runnable() {
        @Override
        public void run() {
            if (mTextWatcher == null) {
                mTextWatcher = new RecipientTextWatcher();
                addTextChangedListener(mTextWatcher);
            }
        }
    };

    private IndividualReplacementTask mIndividualReplacements;

    private Runnable mHandlePendingChips = new Runnable() {

        @Override
        public void run() {
            handlePendingChips();
        }

    };

    private Runnable mDelayedShrink = new Runnable() {

        @Override
        public void run() {
            shrink();
        }

    };

    private RecipientEntryItemClickedListener mRecipientEntryItemClickedListener;

    private RecipientChipAddedListener mRecipientChipAddedListener;
    private RecipientChipDeletedListener mRecipientChipDeletedListener;

    public interface RecipientEntryItemClickedListener {
        /**
         * Callback that occurs whenever an auto-complete suggestion is clicked.
         * @param charactersTyped the number of characters typed by the user to provide the
         *                        auto-complete suggestions.
         * @param position the position in the dropdown list that the user clicked
         */
        void onRecipientEntryItemClicked(int charactersTyped, int position);
    }

    private PermissionsRequestItemClickedListener mPermissionsRequestItemClickedListener;

    /**
     * Listener for handling clicks on the {@link RecipientEntry} that have
     * {@link RecipientEntry#ENTRY_TYPE_PERMISSION_REQUEST} type.
     */
    public interface PermissionsRequestItemClickedListener {

        /**
         * Callback that occurs when user clicks the item that asks user to grant permissions to
         * the app.
         *
         * @param view View that asks for permission.
         */
        void onPermissionsRequestItemClicked(RecipientEditTextView view, String[] permissions);

        /**
         * Callback that occurs when user dismisses the item that asks user to grant permissions to
         * the app.
         */
        void onPermissionRequestDismissed();
    }

    /**
     * Listener for handling deletion of chips in the recipient edit text.
     */
    public interface RecipientChipDeletedListener {
        /**
         * Callback that occurs when a chip is deleted.
         * @param entry RecipientEntry that contains information about the chip.
         */
        void onRecipientChipDeleted(RecipientEntry entry);
    }

    /**
     * Listener for handling addition of chips in the recipient edit text.
     */
    public interface RecipientChipAddedListener {
        /**
         * Callback that occurs when a chip is added.
         *
         * @param entry RecipientEntry that contains information about the chip.
         */
        void onRecipientChipAdded(RecipientEntry entry);
    }

    public RecipientEditTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setChipDimensions(context, attrs);
        mTextHeight = calculateTextHeight();
        mAlternatesPopup = new ListPopupWindow(context);
        setupPopupWindow(mAlternatesPopup);
        mAddressPopup = new ListPopupWindow(context);
        setupPopupWindow(mAddressPopup);
        mAlternatesListener = new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView,View view, int position,
                    long rowId) {
                mAlternatesPopup.setOnItemClickListener(null);
                replaceChip(mSelectedChip, ((RecipientAlternatesAdapter) adapterView.getAdapter())
                        .getRecipientEntry(position));
                Message delayed = Message.obtain(mHandler, DISMISS);
                delayed.obj = mAlternatesPopup;
                mHandler.sendMessageDelayed(delayed, DISMISS_DELAY);
                clearComposingText();
            }
        };
        setInputType(getInputType() | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        setOnItemClickListener(this);
        setCustomSelectionActionModeCallback(this);
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == DISMISS) {
                    ((ListPopupWindow) msg.obj).dismiss();
                    return;
                }
                super.handleMessage(msg);
            }
        };
        mTextWatcher = new RecipientTextWatcher();
        addTextChangedListener(mTextWatcher);
        mGestureDetector = new GestureDetector(context, this);
        setOnEditorActionListener(this);

        setDropdownChipLayouter(new DropdownChipLayouter(LayoutInflater.from(context), context));
    }

    private void setupPopupWindow(ListPopupWindow popup) {
        popup.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                clearSelectedChip();
            }
        });
    }

    private int calculateTextHeight() {
        final TextPaint paint = getPaint();

        mRect.setEmpty();
        // First measure the bounds of a sample text.
        final String textHeightSample = "a";
        paint.getTextBounds(textHeightSample, 0, textHeightSample.length(), mRect);

        mRect.left = 0;
        mRect.right = 0;

        return mRect.height();
    }

    public void setDropdownChipLayouter(DropdownChipLayouter dropdownChipLayouter) {
        mDropdownChipLayouter = dropdownChipLayouter;
        mDropdownChipLayouter.setDeleteListener(this);
        mDropdownChipLayouter.setPermissionRequestDismissedListener(this);
    }

    public void setRecipientEntryItemClickedListener(RecipientEntryItemClickedListener listener) {
        mRecipientEntryItemClickedListener = listener;
    }

    public void setPermissionsRequestItemClickedListener(
            PermissionsRequestItemClickedListener listener) {
        mPermissionsRequestItemClickedListener = listener;
    }

    public void setRecipientChipAddedListener(RecipientChipAddedListener listener) {
        mRecipientChipAddedListener = listener;
    }

    public void setRecipientChipDeletedListener(RecipientChipDeletedListener listener) {
        mRecipientChipDeletedListener = listener;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mAttachedToWindow = false;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mAttachedToWindow = true;

        final int anchorId = getDropDownAnchor();
        if (anchorId != View.NO_ID) {
            mDropdownAnchor = getRootView().findViewById(anchorId);
        }
    }

    @Override
    public void setDropDownAnchor(int anchorId) {
        super.setDropDownAnchor(anchorId);
        if (anchorId != View.NO_ID) {
          mDropdownAnchor = getRootView().findViewById(anchorId);
        }
    }

    @Override
    public boolean onEditorAction(TextView view, int action, KeyEvent keyEvent) {
        if (action == EditorInfo.IME_ACTION_DONE) {
            if (commitDefault()) {
                return true;
            }
            if (mSelectedChip != null) {
                clearSelectedChip();
                return true;
            } else if (hasFocus()) {
                if (focusNext()) {
                    return true;
                }
            }
        }
        return false;
    }

    @Override
    public InputConnection onCreateInputConnection(@NonNull EditorInfo outAttrs) {
        InputConnection connection = super.onCreateInputConnection(outAttrs);
        int imeActions = outAttrs.imeOptions&EditorInfo.IME_MASK_ACTION;
        if ((imeActions&EditorInfo.IME_ACTION_DONE) != 0) {
            // clear the existing action
            outAttrs.imeOptions ^= imeActions;
            // set the DONE action
            outAttrs.imeOptions |= EditorInfo.IME_ACTION_DONE;
        }
        if ((outAttrs.imeOptions&EditorInfo.IME_FLAG_NO_ENTER_ACTION) != 0) {
            outAttrs.imeOptions &= ~EditorInfo.IME_FLAG_NO_ENTER_ACTION;
        }

        outAttrs.actionId = EditorInfo.IME_ACTION_DONE;

        // Custom action labels are discouraged in L; a checkmark icon is shown in place of the
        // custom text in this case.
        outAttrs.actionLabel = Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP ? null :
            getContext().getString(R.string.action_label);
        return connection;
    }

    /*package*/ DrawableRecipientChip getLastChip() {
        DrawableRecipientChip last = null;
        DrawableRecipientChip[] chips = getSortedRecipients();
        if (chips != null && chips.length > 0) {
            last = chips[chips.length - 1];
        }
        return last;
    }

    /**
     * @return The list of {@link RecipientEntry}s that have been selected by the user.
     */
    public List<RecipientEntry> getSelectedRecipients() {
        DrawableRecipientChip[] chips =
                getText().getSpans(0, getText().length(), DrawableRecipientChip.class);
        List<RecipientEntry> results = new ArrayList<RecipientEntry>();
        if (chips == null) {
            return results;
        }

        for (DrawableRecipientChip c : chips) {
            results.add(c.getEntry());
        }

        return results;
    }

    /**
     * @return The list of {@link RecipientEntry}s that have been selected by the user and also
     *         hidden due to {@link #mMoreChip} span.
     */
    public List<RecipientEntry> getAllRecipients() {
        List<RecipientEntry> results = getSelectedRecipients();

        if (mHiddenSpans != null) {
            for (DrawableRecipientChip chip : mHiddenSpans) {
                results.add(chip.getEntry());
            }
        }

        return results;
    }

    @Override
    public void onSelectionChanged(int start, int end) {
        // When selection changes, see if it is inside the chips area.
        // If so, move the cursor back after the chips again.
        // Only exception is when we change the selection due to a selected chip.
        DrawableRecipientChip last = getLastChip();
        if (mSelectedChip == null && last != null && start < getSpannable().getSpanEnd(last)) {
            // Grab the last chip and set the cursor to after it.
            setSelection(Math.min(getSpannable().getSpanEnd(last) + 1, getText().length()));
        }
        super.onSelectionChanged(start, end);
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        if (!TextUtils.isEmpty(getText())) {
            super.onRestoreInstanceState(null);
        } else {
            super.onRestoreInstanceState(state);
        }
    }

    @Override
    public Parcelable onSaveInstanceState() {
        // If the user changes orientation while they are editing, just roll back the selection.
        clearSelectedChip();
        return super.onSaveInstanceState();
    }

    /**
     * Convenience method: Append the specified text slice to the TextView's
     * display buffer, upgrading it to BufferType.EDITABLE if it was
     * not already editable. Commas are excluded as they are added automatically
     * by the view.
     */
    @Override
    public void append(CharSequence text, int start, int end) {
        // We don't care about watching text changes while appending.
        if (mTextWatcher != null) {
            removeTextChangedListener(mTextWatcher);
        }
        super.append(text, start, end);
        if (!TextUtils.isEmpty(text) && TextUtils.getTrimmedLength(text) > 0) {
            String displayString = text.toString();

            if (!displayString.trim().endsWith(String.valueOf(COMMIT_CHAR_COMMA))) {
                // We have no separator, so we should add it
                super.append(SEPARATOR, 0, SEPARATOR.length());
                displayString += SEPARATOR;
            }

            if (!TextUtils.isEmpty(displayString)
                    && TextUtils.getTrimmedLength(displayString) > 0) {
                mPendingChipsCount++;
                mPendingChips.add(displayString);
            }
        }
        // Put a message on the queue to make sure we ALWAYS handle pending
        // chips.
        if (mPendingChipsCount > 0) {
            postHandlePendingChips();
        }
        mHandler.post(mAddTextWatcher);
    }

    @Override
    public void onFocusChanged(boolean hasFocus, int direction, Rect previous) {
        super.onFocusChanged(hasFocus, direction, previous);
        if (!hasFocus) {
            shrink();
        } else {
            expand();
        }
    }

    @Override
    public <T extends ListAdapter & Filterable> void setAdapter(@NonNull T adapter) {
        super.setAdapter(adapter);
        BaseRecipientAdapter baseAdapter = (BaseRecipientAdapter) adapter;
        baseAdapter.registerUpdateObserver(new BaseRecipientAdapter.EntriesUpdatedObserver() {
            @Override
            public void onChanged(List<RecipientEntry> entries) {
                int suggestionCount = entries == null ? 0 : entries.size();

                // Scroll the chips field to the top of the screen so
                // that the user can see as many results as possible.
                if (entries != null && entries.size() > 0) {
                    scrollBottomIntoView();
                    // Here the current suggestion count is still the old one since we update
                    // the count at the bottom of this function.
                    if (mCurrentSuggestionCount == 0) {
                        // Announce the new number of possible choices for accessibility.
                        announceForAccessibilityCompat(
                                getSuggestionDropdownOpenedVerbalization(suggestionCount));
                    }
                }

                // Is the dropdown closing?
                if ((entries == null || entries.size() == 0)
                        // Here the current suggestion count is still the old one since we update
                        // the count at the bottom of this function.
                        && mCurrentSuggestionCount != 0
                        // If there is no text, there's no need to know if no suggestions are
                        // available.
                        && getText().length() > 0) {
                    announceForAccessibilityCompat(getResources().getString(
                            R.string.accessbility_suggestion_dropdown_closed));
                }

                if ((entries != null)
                        && (entries.size() == 1)
                        && (entries.get(0).getEntryType() ==
                                RecipientEntry.ENTRY_TYPE_PERMISSION_REQUEST)) {
                    // Do nothing; showing a single permissions entry. Resizing not required.
                } else {
                    // Set the dropdown height to be the remaining height from the anchor to the
                    // bottom.
                    mDropdownAnchor.getLocationOnScreen(mCoords);
                    getWindowVisibleDisplayFrame(mRect);
                    setDropDownHeight(mRect.bottom - mCoords[1] - mDropdownAnchor.getHeight() -
                            getDropDownVerticalOffset());
                }

                mCurrentSuggestionCount = suggestionCount;
            }
        });
        baseAdapter.setDropdownChipLayouter(mDropdownChipLayouter);
    }

    /**
     * Return the accessibility verbalization when the suggestion dropdown is opened.
     */
    public String getSuggestionDropdownOpenedVerbalization(int suggestionCount) {
        return getResources().getString(R.string.accessbility_suggestion_dropdown_opened);
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void announceForAccessibilityCompat(String text) {
        final AccessibilityManager accessibilityManager =
                (AccessibilityManager) getContext().getSystemService(Context.ACCESSIBILITY_SERVICE);
        final boolean isAccessibilityOn = accessibilityManager.isEnabled();

        if (isAccessibilityOn && Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            final ViewParent parent = getParent();
            if (parent != null) {
                AccessibilityEvent event = AccessibilityEvent.obtain(
                        AccessibilityEvent.TYPE_ANNOUNCEMENT);
                onInitializeAccessibilityEvent(event);
                event.getText().add(text);
                event.setContentDescription(null);
                parent.requestSendAccessibilityEvent(this, event);
            }
        }
    }

    protected void scrollBottomIntoView() {
        if (mScrollView != null && mShouldShrink) {
            getLocationInWindow(mCoords);
            // Desired position shows at least 1 line of chips below the action
            // bar. We add excess padding to make sure this is always below other
            // content.
            final int height = getHeight();
            final int currentPos = mCoords[1] + height;
            mScrollView.getLocationInWindow(mCoords);
            final int desiredPos = mCoords[1] + height / getLineCount();
            if (currentPos > desiredPos) {
                mScrollView.scrollBy(0, currentPos - desiredPos);
            }
        }
    }

    protected ScrollView getScrollView() {
        return mScrollView;
    }

    @Override
    public void performValidation() {
        // Do nothing. Chips handles its own validation.
    }

    private void shrink() {
        if (mTokenizer == null) {
            return;
        }
        long contactId = mSelectedChip != null ? mSelectedChip.getEntry().getContactId() : -1;
        if (mSelectedChip != null && contactId != RecipientEntry.INVALID_CONTACT
                && (!isPhoneQuery() && contactId != RecipientEntry.GENERATED_CONTACT)) {
            clearSelectedChip();
        } else {
            if (getWidth() <= 0) {
                mHandler.removeCallbacks(mDelayedShrink);

                if (getVisibility() == GONE) {
                    // We aren't going to have a width any time soon, so defer
                    // this until we're not GONE.
                    mRequiresShrinkWhenNotGone = true;
                } else {
                    // We don't have the width yet which means the view hasn't been drawn yet
                    // and there is no reason to attempt to commit chips yet.
                    // This focus lost must be the result of an orientation change
                    // or an initial rendering.
                    // Re-post the shrink for later.
                    mHandler.post(mDelayedShrink);
                }
                return;
            }
            // Reset any pending chips as they would have been handled
            // when the field lost focus.
            if (mPendingChipsCount > 0) {
                postHandlePendingChips();
            } else {
                Editable editable = getText();
                int end = getSelectionEnd();
                int start = mTokenizer.findTokenStart(editable, end);
                DrawableRecipientChip[] chips =
                        getSpannable().getSpans(start, end, DrawableRecipientChip.class);
                if ((chips == null || chips.length == 0)) {
                    Editable text = getText();
                    int whatEnd = mTokenizer.findTokenEnd(text, start);
                    // This token was already tokenized, so skip past the ending token.
                    if (whatEnd < text.length() && text.charAt(whatEnd) == ',') {
                        whatEnd = movePastTerminators(whatEnd);
                    }
                    // In the middle of chip; treat this as an edit
                    // and commit the whole token.
                    int selEnd = getSelectionEnd();
                    if (whatEnd != selEnd) {
                        handleEdit(start, whatEnd);
                    } else {
                        commitChip(start, end, editable);
                    }
                }
            }
            mHandler.post(mAddTextWatcher);
        }
        createMoreChip();
    }

    private void expand() {
        if (mShouldShrink) {
            setMaxLines(Integer.MAX_VALUE);
        }
        removeMoreChip();
        setCursorVisible(true);
        Editable text = getText();
        setSelection(text != null && text.length() > 0 ? text.length() : 0);
        // If there are any temporary chips, try replacing them now that the user
        // has expanded the field.
        if (mTemporaryRecipients != null && mTemporaryRecipients.size() > 0) {
            new RecipientReplacementTask().execute();
            mTemporaryRecipients = null;
        }
    }

    private CharSequence ellipsizeText(CharSequence text, TextPaint paint, float maxWidth) {
        paint.setTextSize(mChipFontSize);
        if (maxWidth <= 0 && Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "Max width is negative: " + maxWidth);
        }
        return TextUtils.ellipsize(text, paint, maxWidth,
                TextUtils.TruncateAt.END);
    }

    /**
     * Creates a bitmap of the given contact on a selected chip.
     *
     * @param contact The recipient entry to pull data from.
     * @param paint The paint to use to draw the bitmap.
     */
    private Bitmap createChipBitmap(RecipientEntry contact, TextPaint paint) {
        paint.setColor(getDefaultChipTextColor(contact));
        ChipBitmapContainer bitmapContainer = createChipBitmap(contact, paint,
                getChipBackground(contact), getDefaultChipBackgroundColor(contact));

        if (bitmapContainer.loadIcon) {
            loadAvatarIcon(contact, bitmapContainer);
        }
        return bitmapContainer.bitmap;
    }

    private ChipBitmapContainer createChipBitmap(RecipientEntry contact, TextPaint paint,
            Drawable overrideBackgroundDrawable, int backgroundColor) {
        final ChipBitmapContainer result = new ChipBitmapContainer();

        Drawable indicatorIcon = null;
        int indicatorPadding = 0;
        if (contact.getIndicatorIconId() != 0) {
            indicatorIcon = getContext().getDrawable(contact.getIndicatorIconId());
            indicatorIcon.setBounds(0, 0,
                    indicatorIcon.getIntrinsicWidth(), indicatorIcon.getIntrinsicHeight());
            indicatorPadding = indicatorIcon.getBounds().width() + mChipTextEndPadding;
        }

        Rect backgroundPadding = new Rect();
        if (overrideBackgroundDrawable != null) {
            overrideBackgroundDrawable.getPadding(backgroundPadding);
        }

        // Ellipsize the text so that it takes AT MOST the entire width of the
        // autocomplete text entry area. Make sure to leave space for padding
        // on the sides.
        int height = (int) mChipHeight;
        // Since the icon is a square, it's width is equal to the maximum height it can be inside
        // the chip. Don't include iconWidth for invalid contacts and when not displaying photos.
        boolean displayIcon = contact.isValid() && contact.shouldDisplayIcon();
        int iconWidth = displayIcon ?
                height - backgroundPadding.top - backgroundPadding.bottom : 0;
        float[] widths = new float[1];
        paint.getTextWidths(" ", widths);
        CharSequence ellipsizedText = ellipsizeText(createChipDisplayText(contact), paint,
                calculateAvailableWidth() - iconWidth - widths[0] - backgroundPadding.left
                - backgroundPadding.right - indicatorPadding);
        int textWidth = (int) paint.measureText(ellipsizedText, 0, ellipsizedText.length());

        // Chip start padding is the same as the end padding if there is no contact image.
        final int startPadding = displayIcon ? mChipTextStartPadding : mChipTextEndPadding;
        // Make sure there is a minimum chip width so the user can ALWAYS
        // tap a chip without difficulty.
        int width = Math.max(iconWidth * 2, textWidth + startPadding + mChipTextEndPadding
                + iconWidth + backgroundPadding.left + backgroundPadding.right + indicatorPadding);

        // Create the background of the chip.
        result.bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        final Canvas canvas = new Canvas(result.bitmap);

        // Check if the background drawable is set via attr
        if (overrideBackgroundDrawable != null) {
            overrideBackgroundDrawable.setBounds(0, 0, width, height);
            overrideBackgroundDrawable.draw(canvas);
        } else {
            // Draw the default chip background
            mWorkPaint.reset();
            mWorkPaint.setColor(backgroundColor);
            final float radius = height / 2;
            canvas.drawRoundRect(new RectF(0, 0, width, height), radius, radius,
                    mWorkPaint);
        }

        // Draw the text vertically aligned
        int textX = shouldPositionAvatarOnRight() ?
                mChipTextEndPadding + backgroundPadding.left + indicatorPadding :
                width - backgroundPadding.right - mChipTextEndPadding - textWidth -
                indicatorPadding;
        canvas.drawText(ellipsizedText, 0, ellipsizedText.length(),
                textX, getTextYOffset(height), paint);

        if (indicatorIcon != null) {
            int indicatorX = shouldPositionAvatarOnRight()
                ? backgroundPadding.left + mChipTextEndPadding
                : width - backgroundPadding.right - indicatorIcon.getBounds().width()
                        - mChipTextEndPadding;
            int indicatorY = height / 2 - indicatorIcon.getBounds().height() / 2;
            indicatorIcon.getBounds().offsetTo(indicatorX, indicatorY);
            indicatorIcon.draw(canvas);
        }

        // Set the variables that are needed to draw the icon bitmap once it's loaded
        int iconX = shouldPositionAvatarOnRight() ? width - backgroundPadding.right - iconWidth :
                backgroundPadding.left;
        result.left = iconX;
        result.top = backgroundPadding.top;
        result.right = iconX + iconWidth;
        result.bottom = height - backgroundPadding.bottom;
        result.loadIcon = displayIcon;

        return result;
    }

    /**
     * Helper function that draws the loaded icon bitmap into the chips bitmap
     */
    private void drawIcon(ChipBitmapContainer bitMapResult, Bitmap icon) {
        final Canvas canvas = new Canvas(bitMapResult.bitmap);
        final RectF src = new RectF(0, 0, icon.getWidth(), icon.getHeight());
        final RectF dst = new RectF(bitMapResult.left, bitMapResult.top, bitMapResult.right,
                bitMapResult.bottom);
        drawIconOnCanvas(icon, canvas, src, dst);
    }

    /**
     * Returns true if the avatar should be positioned at the right edge of the chip.
     * Takes into account both the set avatar position (start or end) as well as whether
     * the layout direction is LTR or RTL.
     */
    private boolean shouldPositionAvatarOnRight() {
        final boolean isRtl = Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1 &&
                getLayoutDirection() == LAYOUT_DIRECTION_RTL;
        final boolean assignedPosition = mAvatarPosition == AVATAR_POSITION_END;
        // If in Rtl mode, the position should be flipped.
        return isRtl ? !assignedPosition : assignedPosition;
    }

    /**
     * Returns the avatar icon to use for this recipient entry. Returns null if we don't want to
     * draw an icon for this recipient.
     */
    private void loadAvatarIcon(final RecipientEntry contact,
            final ChipBitmapContainer bitmapContainer) {
        // Don't draw photos for recipients that have been typed in OR generated on the fly.
        long contactId = contact.getContactId();
        boolean drawPhotos = isPhoneQuery() ?
                contactId != RecipientEntry.INVALID_CONTACT
                : (contactId != RecipientEntry.INVALID_CONTACT
                        && contactId != RecipientEntry.GENERATED_CONTACT);

        if (drawPhotos) {
            final byte[] origPhotoBytes = contact.getPhotoBytes();
            // There may not be a photo yet if anything but the first contact address
            // was selected.
            if (origPhotoBytes == null) {
                // TODO: cache this in the recipient entry?
                getAdapter().fetchPhoto(contact, new PhotoManager.PhotoManagerCallback() {
                    @Override
                    public void onPhotoBytesPopulated() {
                        // Call through to the async version which will ensure
                        // proper threading.
                        onPhotoBytesAsynchronouslyPopulated();
                    }

                    @Override
                    public void onPhotoBytesAsynchronouslyPopulated() {
                        final byte[] loadedPhotoBytes = contact.getPhotoBytes();
                        final Bitmap icon = BitmapFactory.decodeByteArray(loadedPhotoBytes, 0,
                                loadedPhotoBytes.length);
                        tryDrawAndInvalidate(icon);
                    }

                    @Override
                    public void onPhotoBytesAsyncLoadFailed() {
                        // TODO: can the scaled down default photo be cached?
                        tryDrawAndInvalidate(mDefaultContactPhoto);
                    }

                    private void tryDrawAndInvalidate(Bitmap icon) {
                        drawIcon(bitmapContainer, icon);
                        // The caller might originated from a background task. However, if the
                        // background task has already completed, the view might be already drawn
                        // on the UI but the callback would happen on the background thread.
                        // So if we are on a background thread, post an invalidate call to the UI.
                        if (Looper.myLooper() == Looper.getMainLooper()) {
                            // The view might not redraw itself since it's loaded asynchronously
                            invalidate();
                        } else {
                            post(new Runnable() {
                                @Override
                                public void run() {
                                    invalidate();
                                }
                            });
                        }
                    }
                });
            } else {
                final Bitmap icon = BitmapFactory.decodeByteArray(origPhotoBytes, 0,
                        origPhotoBytes.length);
                drawIcon(bitmapContainer, icon);
            }
        }
    }

    /**
     * Get the background drawable for a RecipientChip.
     */
    // Visible for testing.
    /* package */Drawable getChipBackground(RecipientEntry contact) {
        return contact.isValid() ? mChipBackground : mInvalidChipBackground;
    }

    private int getDefaultChipTextColor(RecipientEntry contact) {
        return contact.isValid() ? mUnselectedChipTextColor :
                getResources().getColor(android.R.color.black);
    }

    private int getDefaultChipBackgroundColor(RecipientEntry contact) {
        return contact.isValid() ? mUnselectedChipBackgroundColor :
                getResources().getColor(R.color.chip_background_invalid);
    }

    /**
     * Given a height, returns a Y offset that will draw the text in the middle of the height.
     */
    protected float getTextYOffset(int height) {
        return height - ((height - mTextHeight) / 2);
    }

    /**
     * Draws the icon onto the canvas given the source rectangle of the bitmap and the destination
     * rectangle of the canvas.
     */
    protected void drawIconOnCanvas(Bitmap icon, Canvas canvas, RectF src, RectF dst) {
        final Matrix matrix = new Matrix();

        // Draw bitmap through shader first.
        final BitmapShader shader = new BitmapShader(icon, TileMode.CLAMP, TileMode.CLAMP);
        matrix.reset();

        // Fit bitmap to bounds.
        matrix.setRectToRect(src, dst, Matrix.ScaleToFit.FILL);

        shader.setLocalMatrix(matrix);
        mWorkPaint.reset();
        mWorkPaint.setShader(shader);
        mWorkPaint.setAntiAlias(true);
        mWorkPaint.setFilterBitmap(true);
        mWorkPaint.setDither(true);
        canvas.drawCircle(dst.centerX(), dst.centerY(), dst.width() / 2f, mWorkPaint);

        // Then draw the border.
        final float borderWidth = 1f;
        mWorkPaint.reset();
        mWorkPaint.setColor(Color.TRANSPARENT);
        mWorkPaint.setStyle(Style.STROKE);
        mWorkPaint.setStrokeWidth(borderWidth);
        mWorkPaint.setAntiAlias(true);
        canvas.drawCircle(dst.centerX(), dst.centerY(), dst.width() / 2f - borderWidth / 2,
                mWorkPaint);

        mWorkPaint.reset();
    }

    private DrawableRecipientChip constructChipSpan(RecipientEntry contact) {
        TextPaint paint = getPaint();
        float defaultSize = paint.getTextSize();
        int defaultColor = paint.getColor();

        Bitmap tmpBitmap = createChipBitmap(contact, paint);

        // Pass the full text, un-ellipsized, to the chip.
        Drawable result = new BitmapDrawable(getResources(), tmpBitmap);
        result.setBounds(0, 0, tmpBitmap.getWidth(), tmpBitmap.getHeight());
        VisibleRecipientChip recipientChip =
                new VisibleRecipientChip(result, contact);
        recipientChip.setExtraMargin(mLineSpacingExtra);
        // Return text to the original size.
        paint.setTextSize(defaultSize);
        paint.setColor(defaultColor);
        return recipientChip;
    }

    /**
     * Calculate the offset from bottom of the EditText to top of the provided line.
     */
    private int calculateOffsetFromBottomToTop(int line) {
        return -(int) ((mChipHeight + (2 * mLineSpacingExtra)) * (Math
                .abs(getLineCount() - line)) + getPaddingBottom());
    }

    /**
     * Get the max amount of space a chip can take up. The formula takes into
     * account the width of the EditTextView, any view padding, and padding
     * that will be added to the chip.
     */
    private float calculateAvailableWidth() {
        return getWidth() - getPaddingLeft() - getPaddingRight() - mChipTextStartPadding
                - mChipTextEndPadding;
    }


    private void setChipDimensions(Context context, AttributeSet attrs) {
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.RecipientEditTextView, 0,
                0);
        Resources r = getContext().getResources();

        mChipBackground = a.getDrawable(R.styleable.RecipientEditTextView_chipBackground);
        mInvalidChipBackground = a
                .getDrawable(R.styleable.RecipientEditTextView_invalidChipBackground);
        mChipDelete = a.getDrawable(R.styleable.RecipientEditTextView_chipDelete);
        if (mChipDelete == null) {
            mChipDelete = r.getDrawable(R.drawable.ic_cancel_wht_24dp);
        }
        mChipTextStartPadding = mChipTextEndPadding
                = a.getDimensionPixelSize(R.styleable.RecipientEditTextView_chipPadding, -1);
        if (mChipTextStartPadding == -1) {
            mChipTextStartPadding = mChipTextEndPadding =
                    (int) r.getDimension(R.dimen.chip_padding);
        }
        // xml-overrides for each individual padding
        // TODO: add these to attr?
        int overridePadding = (int) r.getDimension(R.dimen.chip_padding_start);
        if (overridePadding >= 0) {
            mChipTextStartPadding = overridePadding;
        }
        overridePadding = (int) r.getDimension(R.dimen.chip_padding_end);
        if (overridePadding >= 0) {
            mChipTextEndPadding = overridePadding;
        }

        mDefaultContactPhoto = BitmapFactory.decodeResource(r, R.drawable.ic_contact_picture);

        mMoreItem = (TextView) LayoutInflater.from(getContext()).inflate(R.layout.more_item, null);

        mChipHeight = a.getDimensionPixelSize(R.styleable.RecipientEditTextView_chipHeight, -1);
        if (mChipHeight == -1) {
            mChipHeight = r.getDimension(R.dimen.chip_height);
        }
        mChipFontSize = a.getDimensionPixelSize(R.styleable.RecipientEditTextView_chipFontSize, -1);
        if (mChipFontSize == -1) {
            mChipFontSize = r.getDimension(R.dimen.chip_text_size);
        }
        mAvatarPosition =
                a.getInt(R.styleable.RecipientEditTextView_avatarPosition, AVATAR_POSITION_START);
        mDisableDelete = a.getBoolean(R.styleable.RecipientEditTextView_disableDelete, false);

        mMaxLines = r.getInteger(R.integer.chips_max_lines);
        mLineSpacingExtra = r.getDimensionPixelOffset(R.dimen.line_spacing_extra);

        mUnselectedChipTextColor = a.getColor(
                R.styleable.RecipientEditTextView_unselectedChipTextColor,
                r.getColor(android.R.color.black));

        mUnselectedChipBackgroundColor = a.getColor(
                R.styleable.RecipientEditTextView_unselectedChipBackgroundColor,
                r.getColor(R.color.chip_background));

        a.recycle();
    }

    // Visible for testing.
    /* package */ void setMoreItem(TextView moreItem) {
        mMoreItem = moreItem;
    }


    // Visible for testing.
    /* package */ void setChipBackground(Drawable chipBackground) {
        mChipBackground = chipBackground;
    }

    // Visible for testing.
    /* package */ void setChipHeight(int height) {
        mChipHeight = height;
    }

    public float getChipHeight() {
        return mChipHeight;
    }

    /** Returns whether view is in no-chip or chip mode. */
    public boolean isNoChipMode() {
        return mNoChipMode;
    }

    /**
     * Set whether to shrink the recipients field such that at most
     * one line of recipients chips are shown when the field loses
     * focus. By default, the number of displayed recipients will be
     * limited and a "more" chip will be shown when focus is lost.
     * @param shrink
     */
    public void setOnFocusListShrinkRecipients(boolean shrink) {
        mShouldShrink = shrink;
    }

    @Override
    public void onSizeChanged(int width, int height, int oldw, int oldh) {
        super.onSizeChanged(width, height, oldw, oldh);
        if (width != 0 && height != 0) {
            if (mPendingChipsCount > 0) {
                postHandlePendingChips();
            } else {
                checkChipWidths();
            }
        }
        // Try to find the scroll view parent, if it exists.
        if (mScrollView == null && !mTriedGettingScrollView) {
            ViewParent parent = getParent();
            while (parent != null && !(parent instanceof ScrollView)) {
                parent = parent.getParent();
            }
            if (parent != null) {
                mScrollView = (ScrollView) parent;
            }
            mTriedGettingScrollView = true;
        }
    }

    private void postHandlePendingChips() {
        mHandler.removeCallbacks(mHandlePendingChips);
        mHandler.post(mHandlePendingChips);
    }

    private void checkChipWidths() {
        // Check the widths of the associated chips.
        DrawableRecipientChip[] chips = getSortedRecipients();
        if (chips != null) {
            Rect bounds;
            for (DrawableRecipientChip chip : chips) {
                bounds = chip.getBounds();
                if (getWidth() > 0 && bounds.right - bounds.left >
                        getWidth() - getPaddingLeft() - getPaddingRight()) {
                    // Need to redraw that chip.
                    replaceChip(chip, chip.getEntry());
                }
            }
        }
    }

    // Visible for testing.
    /*package*/ void handlePendingChips() {
        if (getViewWidth() <= 0) {
            // The widget has not been sized yet.
            // This will be called as a result of onSizeChanged
            // at a later point.
            return;
        }
        if (mPendingChipsCount <= 0) {
            return;
        }

        synchronized (mPendingChips) {
            Editable editable = getText();
            // Tokenize!
            if (mPendingChipsCount <= MAX_CHIPS_PARSED) {
                for (int i = 0; i < mPendingChips.size(); i++) {
                    String current = mPendingChips.get(i);
                    int tokenStart = editable.toString().indexOf(current);
                    // Always leave a space at the end between tokens.
                    int tokenEnd = tokenStart + current.length() - 1;
                    if (tokenStart >= 0) {
                        // When we have a valid token, include it with the token
                        // to the left.
                        if (tokenEnd < editable.length() - 2
                                && editable.charAt(tokenEnd) == COMMIT_CHAR_COMMA) {
                            tokenEnd++;
                        }
                        createReplacementChip(tokenStart, tokenEnd, editable, i < CHIP_LIMIT
                                || !mShouldShrink);
                    }
                    mPendingChipsCount--;
                }
                sanitizeEnd();
            } else {
                mNoChipMode = true;
            }

            if (mTemporaryRecipients != null && mTemporaryRecipients.size() > 0
                    && mTemporaryRecipients.size() <= RecipientAlternatesAdapter.MAX_LOOKUPS) {
                if (hasFocus() || mTemporaryRecipients.size() < CHIP_LIMIT) {
                    new RecipientReplacementTask().execute();
                    mTemporaryRecipients = null;
                } else {
                    // Create the "more" chip
                    mIndividualReplacements = new IndividualReplacementTask();
                    mIndividualReplacements.execute(new ArrayList<DrawableRecipientChip>(
                            mTemporaryRecipients.subList(0, CHIP_LIMIT)));
                    if (mTemporaryRecipients.size() > CHIP_LIMIT) {
                        mTemporaryRecipients = new ArrayList<DrawableRecipientChip>(
                                mTemporaryRecipients.subList(CHIP_LIMIT,
                                        mTemporaryRecipients.size()));
                    } else {
                        mTemporaryRecipients = null;
                    }
                    createMoreChip();
                }
            } else {
                // There are too many recipients to look up, so just fall back
                // to showing addresses for all of them.
                mTemporaryRecipients = null;
                createMoreChip();
            }
            mPendingChipsCount = 0;
            mPendingChips.clear();
        }
    }

    // Visible for testing.
    /*package*/ int getViewWidth() {
        return getWidth();
    }

    /**
     * Remove any characters after the last valid chip.
     */
    // Visible for testing.
    /*package*/ void sanitizeEnd() {
        // Don't sanitize while we are waiting for pending chips to complete.
        if (mPendingChipsCount > 0) {
            return;
        }
        // Find the last chip; eliminate any commit characters after it.
        DrawableRecipientChip[] chips = getSortedRecipients();
        Spannable spannable = getSpannable();
        if (chips != null && chips.length > 0) {
            int end;
            mMoreChip = getMoreChip();
            if (mMoreChip != null) {
                end = spannable.getSpanEnd(mMoreChip);
            } else {
                end = getSpannable().getSpanEnd(getLastChip());
            }
            Editable editable = getText();
            int length = editable.length();
            if (length > end) {
                // See what characters occur after that and eliminate them.
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "There were extra characters after the last tokenizable entry."
                            + editable);
                }
                editable.delete(end + 1, length);
            }
        }
    }

    /**
     * Create a chip that represents just the email address of a recipient. At some later
     * point, this chip will be attached to a real contact entry, if one exists.
     */
    // VisibleForTesting
    void createReplacementChip(int tokenStart, int tokenEnd, Editable editable,
            boolean visible) {
        if (alreadyHasChip(tokenStart, tokenEnd)) {
            // There is already a chip present at this location.
            // Don't recreate it.
            return;
        }
        String token = editable.toString().substring(tokenStart, tokenEnd);
        final String trimmedToken = token.trim();
        int commitCharIndex = trimmedToken.lastIndexOf(COMMIT_CHAR_COMMA);
        if (commitCharIndex != -1 && commitCharIndex == trimmedToken.length() - 1) {
            token = trimmedToken.substring(0, trimmedToken.length() - 1);
        }
        RecipientEntry entry = createTokenizedEntry(token);
        if (entry != null) {
            DrawableRecipientChip chip = null;
            try {
                if (!mNoChipMode) {
                    chip = visible ? constructChipSpan(entry) : new InvisibleRecipientChip(entry);
                }
            } catch (NullPointerException e) {
                Log.e(TAG, e.getMessage(), e);
            }
            editable.setSpan(chip, tokenStart, tokenEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            // Add this chip to the list of entries "to replace"
            if (chip != null) {
                if (mTemporaryRecipients == null) {
                    mTemporaryRecipients = new ArrayList<DrawableRecipientChip>();
                }
                chip.setOriginalText(token);
                mTemporaryRecipients.add(chip);
            }
        }
    }

    // VisibleForTesting
    RecipientEntry createTokenizedEntry(final String token) {
        if (TextUtils.isEmpty(token)) {
            return null;
        }
        if (isPhoneQuery() && PhoneUtil.isPhoneNumber(token)) {
            return RecipientEntry.constructFakePhoneEntry(token, true);
        }
        Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(token);
        boolean isValid = isValid(token);
        if (isValid && tokens != null && tokens.length > 0) {
            // If we can get a name from tokenizing, then generate an entry from
            // this.
            String display = tokens[0].getName();
            if (!TextUtils.isEmpty(display)) {
                return RecipientEntry.constructGeneratedEntry(display, tokens[0].getAddress(),
                        isValid);
            } else {
                display = tokens[0].getAddress();
                if (!TextUtils.isEmpty(display)) {
                    return RecipientEntry.constructFakeEntry(display, isValid);
                }
            }
        }
        // Unable to validate the token or to create a valid token from it.
        // Just create a chip the user can edit.
        String validatedToken = null;
        if (mValidator != null && !isValid) {
            // Try fixing up the entry using the validator.
            validatedToken = mValidator.fixText(token).toString();
            if (!TextUtils.isEmpty(validatedToken)) {
                if (validatedToken.contains(token)) {
                    // protect against the case of a validator with a null
                    // domain,
                    // which doesn't add a domain to the token
                    Rfc822Token[] tokenized = Rfc822Tokenizer.tokenize(validatedToken);
                    if (tokenized.length > 0) {
                        validatedToken = tokenized[0].getAddress();
                        isValid = true;
                    }
                } else {
                    // We ran into a case where the token was invalid and
                    // removed
                    // by the validator. In this case, just use the original
                    // token
                    // and let the user sort out the error chip.
                    validatedToken = null;
                    isValid = false;
                }
            }
        }
        // Otherwise, fallback to just creating an editable email address chip.
        return RecipientEntry.constructFakeEntry(
                !TextUtils.isEmpty(validatedToken) ? validatedToken : token, isValid);
    }

    private boolean isValid(String text) {
        return mValidator == null ? true : mValidator.isValid(text);
    }

    private static String tokenizeAddress(String destination) {
        Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(destination);
        if (tokens != null && tokens.length > 0) {
            return tokens[0].getAddress();
        }
        return destination;
    }

    @Override
    public void setTokenizer(Tokenizer tokenizer) {
        mTokenizer = tokenizer;
        super.setTokenizer(mTokenizer);
    }

    @Override
    public void setValidator(Validator validator) {
        mValidator = validator;
        super.setValidator(validator);
    }

    /**
     * We cannot use the default mechanism for replaceText. Instead,
     * we override onItemClickListener so we can get all the associated
     * contact information including display text, address, and id.
     */
    @Override
    protected void replaceText(CharSequence text) {
        return;
    }

    /**
     * Dismiss any selected chips when the back key is pressed.
     */
    @Override
    public boolean onKeyPreIme(int keyCode, @NonNull KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && mSelectedChip != null) {
            clearSelectedChip();
            return true;
        }
        return super.onKeyPreIme(keyCode, event);
    }

    /**
     * Monitor key presses in this view to see if the user types
     * any commit keys, which consist of ENTER, TAB, or DPAD_CENTER.
     * If the user has entered text that has contact matches and types
     * a commit key, create a chip from the topmost matching contact.
     * If the user has entered text that has no contact matches and types
     * a commit key, then create a chip from the text they have entered.
     */
    @Override
    public boolean onKeyUp(int keyCode, @NonNull KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_TAB:
                if (event.hasNoModifiers()) {
                    if (mSelectedChip != null) {
                        clearSelectedChip();
                    } else {
                        commitDefault();
                    }
                }
                break;
        }
        return super.onKeyUp(keyCode, event);
    }

    private boolean focusNext() {
        View next = focusSearch(View.FOCUS_DOWN);
        if (next != null) {
            next.requestFocus();
            return true;
        }
        return false;
    }

    /**
     * Create a chip from the default selection. If the popup is showing, the
     * default is the selected item (if one is selected), or the first item, in the popup
     * suggestions list. Otherwise, it is whatever the user had typed in. End represents where the
     * tokenizer should search for a token to turn into a chip.
     * @return If a chip was created from a real contact.
     */
    private boolean commitDefault() {
        // If there is no tokenizer, don't try to commit.
        if (mTokenizer == null) {
            return false;
        }
        Editable editable = getText();
        int end = getSelectionEnd();
        int start = mTokenizer.findTokenStart(editable, end);

        if (shouldCreateChip(start, end)) {
            int whatEnd = mTokenizer.findTokenEnd(getText(), start);
            // In the middle of chip; treat this as an edit
            // and commit the whole token.
            whatEnd = movePastTerminators(whatEnd);
            if (whatEnd != getSelectionEnd()) {
                handleEdit(start, whatEnd);
                return true;
            }
            return commitChip(start, end , editable);
        }
        return false;
    }

    private void commitByCharacter() {
        // We can't possibly commit by character if we can't tokenize.
        if (mTokenizer == null) {
            return;
        }
        Editable editable = getText();
        int end = getSelectionEnd();
        int start = mTokenizer.findTokenStart(editable, end);
        if (shouldCreateChip(start, end)) {
            commitChip(start, end, editable);
        }
        setSelection(getText().length());
    }

    private boolean commitChip(int start, int end, Editable editable) {
        int position = positionOfFirstEntryWithTypePerson();
        if (position != -1 && enoughToFilter()
                && end == getSelectionEnd() && !isPhoneQuery()
                && !isValidEmailAddress(editable.toString().substring(start, end).trim())) {
            // let's choose the selected or first entry if only the input text is NOT an email
            // address so we won't try to replace the user's potentially correct but
            // new/unencountered email input
            final int selectedPosition = getListSelection();
            if (selectedPosition == -1 || !isEntryAtPositionTypePerson(selectedPosition)) {
                // Nothing is selected or selected item is not type person; use the first item
                submitItemAtPosition(position);
            } else {
                submitItemAtPosition(selectedPosition);
            }
            dismissDropDown();
            return true;
        } else {
            int tokenEnd = mTokenizer.findTokenEnd(editable, start);
            if (editable.length() > tokenEnd + 1) {
                char charAt = editable.charAt(tokenEnd + 1);
                if (charAt == COMMIT_CHAR_COMMA || charAt == COMMIT_CHAR_SEMICOLON) {
                    tokenEnd++;
                }
            }
            String text = editable.toString().substring(start, tokenEnd).trim();
            clearComposingText();
            if (text.length() > 0 && !text.equals(" ")) {
                RecipientEntry entry = createTokenizedEntry(text);
                if (entry != null) {
                    QwertyKeyListener.markAsReplaced(editable, start, end, "");
                    CharSequence chipText = createChip(entry);
                    if (chipText != null && start > -1 && end > -1) {
                        editable.replace(start, end, chipText);
                    }
                }
                // Only dismiss the dropdown if it is related to the text we
                // just committed.
                // For paste, it may not be as there are possibly multiple
                // tokens being added.
                if (end == getSelectionEnd()) {
                    dismissDropDown();
                }
                sanitizeBetween();
                return true;
            }
        }
        return false;
    }

    private int positionOfFirstEntryWithTypePerson() {
        ListAdapter adapter = getAdapter();
        int itemCount = adapter != null ? adapter.getCount() : 0;
        for (int i = 0; i < itemCount; i++) {
            if (isEntryAtPositionTypePerson(i)) {
                return i;
            }
        }
        return -1;
    }

    private boolean isEntryAtPositionTypePerson(int position) {
        return getAdapter().getItem(position).getEntryType() == RecipientEntry.ENTRY_TYPE_PERSON;
    }

    // Visible for testing.
    /* package */ void sanitizeBetween() {
        // Don't sanitize while we are waiting for content to chipify.
        if (mPendingChipsCount > 0) {
            return;
        }
        // Find the last chip.
        DrawableRecipientChip[] recips = getSortedRecipients();
        if (recips != null && recips.length > 0) {
            DrawableRecipientChip last = recips[recips.length - 1];
            DrawableRecipientChip beforeLast = null;
            if (recips.length > 1) {
                beforeLast = recips[recips.length - 2];
            }
            int startLooking = 0;
            int end = getSpannable().getSpanStart(last);
            if (beforeLast != null) {
                startLooking = getSpannable().getSpanEnd(beforeLast);
                Editable text = getText();
                if (startLooking == -1 || startLooking > text.length() - 1) {
                    // There is nothing after this chip.
                    return;
                }
                if (text.charAt(startLooking) == ' ') {
                    startLooking++;
                }
            }
            if (startLooking >= 0 && end >= 0 && startLooking < end) {
                getText().delete(startLooking, end);
            }
        }
    }

    private boolean shouldCreateChip(int start, int end) {
        return !mNoChipMode && hasFocus() && enoughToFilter() && !alreadyHasChip(start, end);
    }

    private boolean alreadyHasChip(int start, int end) {
        if (mNoChipMode) {
            return true;
        }
        DrawableRecipientChip[] chips =
                getSpannable().getSpans(start, end, DrawableRecipientChip.class);
        return chips != null && chips.length > 0;
    }

    private void handleEdit(int start, int end) {
        if (start == -1 || end == -1) {
            // This chip no longer exists in the field.
            dismissDropDown();
            return;
        }
        // This is in the middle of a chip, so select out the whole chip
        // and commit it.
        Editable editable = getText();
        setSelection(end);
        String text = getText().toString().substring(start, end);
        if (!TextUtils.isEmpty(text)) {
            RecipientEntry entry = RecipientEntry.constructFakeEntry(text, isValid(text));
            QwertyKeyListener.markAsReplaced(editable, start, end, "");
            CharSequence chipText = createChip(entry);
            int selEnd = getSelectionEnd();
            if (chipText != null && start > -1 && selEnd > -1) {
                editable.replace(start, selEnd, chipText);
            }
        }
        dismissDropDown();
    }

    /**
     * If there is a selected chip, delegate the key events
     * to the selected chip.
     */
    @Override
    public boolean onKeyDown(int keyCode, @NonNull KeyEvent event) {
        if (mSelectedChip != null && keyCode == KeyEvent.KEYCODE_DEL) {
            if (mAlternatesPopup != null && mAlternatesPopup.isShowing()) {
                mAlternatesPopup.dismiss();
            }
            removeChip(mSelectedChip);
        }

        switch (keyCode) {
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_DPAD_CENTER:
                if (event.hasNoModifiers()) {
                    if (commitDefault()) {
                        return true;
                    }
                    if (mSelectedChip != null) {
                        clearSelectedChip();
                        return true;
                    } else if (focusNext()) {
                        return true;
                    }
                }
                break;
        }

        return super.onKeyDown(keyCode, event);
    }

    // Visible for testing.
    /* package */ Spannable getSpannable() {
        return getText();
    }

    private int getChipStart(DrawableRecipientChip chip) {
        return getSpannable().getSpanStart(chip);
    }

    private int getChipEnd(DrawableRecipientChip chip) {
        return getSpannable().getSpanEnd(chip);
    }

    /**
     * Instead of filtering on the entire contents of the edit box,
     * this subclass method filters on the range from
     * {@link Tokenizer#findTokenStart} to {@link #getSelectionEnd}
     * if the length of that range meets or exceeds {@link #getThreshold}
     * and makes sure that the range is not already a Chip.
     */
    @Override
    public void performFiltering(@NonNull CharSequence text, int keyCode) {
        boolean isCompletedToken = isCompletedToken(text);
        if (enoughToFilter() && !isCompletedToken) {
            int end = getSelectionEnd();
            int start = mTokenizer.findTokenStart(text, end);
            // If this is a RecipientChip, don't filter
            // on its contents.
            Spannable span = getSpannable();
            DrawableRecipientChip[] chips = span.getSpans(start, end, DrawableRecipientChip.class);
            if (chips != null && chips.length > 0) {
                dismissDropDown();
                return;
            }
        } else if (isCompletedToken) {
            dismissDropDown();
            return;
        }
        super.performFiltering(text, keyCode);
    }

    // Visible for testing.
    /*package*/ boolean isCompletedToken(CharSequence text) {
        if (TextUtils.isEmpty(text)) {
            return false;
        }
        // Check to see if this is a completed token before filtering.
        int end = text.length();
        int start = mTokenizer.findTokenStart(text, end);
        String token = text.toString().substring(start, end).trim();
        if (!TextUtils.isEmpty(token)) {
            char atEnd = token.charAt(token.length() - 1);
            return atEnd == COMMIT_CHAR_COMMA || atEnd == COMMIT_CHAR_SEMICOLON;
        }
        return false;
    }

    /**
     * Clears the selected chip if there is one (and dismissing any popups related to the selected
     * chip in the process).
     */
    public void clearSelectedChip() {
        if (mSelectedChip != null) {
            unselectChip(mSelectedChip);
            mSelectedChip = null;
        }
        setCursorVisible(true);
        setSelection(getText().length());
    }

    /**
     * Monitor touch events in the RecipientEditTextView.
     * If the view does not have focus, any tap on the view
     * will just focus the view. If the view has focus, determine
     * if the touch target is a recipient chip. If it is and the chip
     * is not selected, select it and clear any other selected chips.
     * If it isn't, then select that chip.
     */
    @Override
    public boolean onTouchEvent(@NonNull MotionEvent event) {
        if (!isFocused()) {
            // Ignore any chip taps until this view is focused.
            return super.onTouchEvent(event);
        }
        boolean handled = super.onTouchEvent(event);
        int action = event.getAction();
        boolean chipWasSelected = false;
        if (mSelectedChip == null) {
            mGestureDetector.onTouchEvent(event);
        }
        if (action == MotionEvent.ACTION_UP) {
            float x = event.getX();
            float y = event.getY();
            int offset = putOffsetInRange(x, y);
            DrawableRecipientChip currentChip = findChip(offset);
            if (currentChip != null) {
                if (mSelectedChip != null && mSelectedChip != currentChip) {
                    clearSelectedChip();
                    selectChip(currentChip);
                } else if (mSelectedChip == null) {
                    commitDefault();
                    selectChip(currentChip);
                } else {
                    onClick(mSelectedChip);
                }
                chipWasSelected = true;
                handled = true;
            } else if (mSelectedChip != null && shouldShowEditableText(mSelectedChip)) {
                chipWasSelected = true;
            }
        }
        if (action == MotionEvent.ACTION_UP && !chipWasSelected) {
            clearSelectedChip();
        }
        return handled;
    }

    private void showAlternates(final DrawableRecipientChip currentChip,
            final ListPopupWindow alternatesPopup) {
        new AsyncTask<Void, Void, ListAdapter>() {
            @Override
            protected ListAdapter doInBackground(final Void... params) {
                return createAlternatesAdapter(currentChip);
            }

            @Override
            protected void onPostExecute(final ListAdapter result) {
                if (!mAttachedToWindow) {
                    return;
                }
                int line = getLayout().getLineForOffset(getChipStart(currentChip));
                int bottomOffset = calculateOffsetFromBottomToTop(line);

                // Align the alternates popup with the left side of the View,
                // regardless of the position of the chip tapped.
                alternatesPopup.setAnchorView((mAlternatePopupAnchor != null) ?
                        mAlternatePopupAnchor : RecipientEditTextView.this);
                alternatesPopup.setVerticalOffset(bottomOffset);
                alternatesPopup.setAdapter(result);
                alternatesPopup.setOnItemClickListener(mAlternatesListener);
                // Clear the checked item.
                mCheckedItem = -1;
                alternatesPopup.show();
                ListView listView = alternatesPopup.getListView();
                listView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
                // Checked item would be -1 if the adapter has not
                // loaded the view that should be checked yet. The
                // variable will be set correctly when onCheckedItemChanged
                // is called in a separate thread.
                if (mCheckedItem != -1) {
                    listView.setItemChecked(mCheckedItem, true);
                    mCheckedItem = -1;
                }
            }
        }.execute((Void[]) null);
    }

    protected ListAdapter createAlternatesAdapter(DrawableRecipientChip chip) {
        return new RecipientAlternatesAdapter(getContext(), chip.getContactId(),
                chip.getDirectoryId(), chip.getLookupKey(), chip.getDataId(),
                getAdapter().getQueryType(), this, mDropdownChipLayouter,
                constructStateListDeleteDrawable(), getAdapter().getPermissionsCheckListener());
    }

    private ListAdapter createSingleAddressAdapter(DrawableRecipientChip currentChip) {
        return new SingleRecipientArrayAdapter(getContext(), currentChip.getEntry(),
                mDropdownChipLayouter, constructStateListDeleteDrawable());
    }

    private StateListDrawable constructStateListDeleteDrawable() {
        // Construct the StateListDrawable from deleteDrawable
        StateListDrawable deleteDrawable = new StateListDrawable();
        if (!mDisableDelete) {
            deleteDrawable.addState(new int[]{android.R.attr.state_activated}, mChipDelete);
        }
        deleteDrawable.addState(new int[0], null);
        return deleteDrawable;
    }

    @Override
    public void onCheckedItemChanged(int position) {
        ListView listView = mAlternatesPopup.getListView();
        if (listView != null && listView.getCheckedItemCount() == 0) {
            listView.setItemChecked(position, true);
        }
        mCheckedItem = position;
    }

    private int putOffsetInRange(final float x, final float y) {
        final int offset;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            offset = getOffsetForPosition(x, y);
        } else {
            offset = supportGetOffsetForPosition(x, y);
        }

        return putOffsetInRange(offset);
    }

    // TODO: This algorithm will need a lot of tweaking after more people have used
    // the chips ui. This attempts to be "forgiving" to fat finger touches by favoring
    // what comes before the finger.
    private int putOffsetInRange(int o) {
        int offset = o;
        Editable text = getText();
        int length = text.length();
        // Remove whitespace from end to find "real end"
        int realLength = length;
        for (int i = length - 1; i >= 0; i--) {
            if (text.charAt(i) == ' ') {
                realLength--;
            } else {
                break;
            }
        }

        // If the offset is beyond or at the end of the text,
        // leave it alone.
        if (offset >= realLength) {
            return offset;
        }
        Editable editable = getText();
        while (offset >= 0 && findText(editable, offset) == -1 && findChip(offset) == null) {
            // Keep walking backward!
            offset--;
        }
        return offset;
    }

    private static int findText(Editable text, int offset) {
        if (text.charAt(offset) != ' ') {
            return offset;
        }
        return -1;
    }

    private DrawableRecipientChip findChip(int offset) {
        final Spannable span = getSpannable();
        final DrawableRecipientChip[] chips =
                span.getSpans(0, span.length(), DrawableRecipientChip.class);
        // Find the chip that contains this offset.
        for (DrawableRecipientChip chip : chips) {
            int start = getChipStart(chip);
            int end = getChipEnd(chip);
            if (offset >= start && offset <= end) {
                return chip;
            }
        }
        return null;
    }

    // Visible for testing.
    // Use this method to generate text to add to the list of addresses.
    /* package */String createAddressText(RecipientEntry entry) {
        String display = entry.getDisplayName();
        String address = entry.getDestination();
        if (TextUtils.isEmpty(display) || TextUtils.equals(display, address)) {
            display = null;
        }
        String trimmedDisplayText;
        if (isPhoneQuery() && PhoneUtil.isPhoneNumber(address)) {
            trimmedDisplayText = address.trim();
        } else {
            if (address != null) {
                // Tokenize out the address in case the address already
                // contained the username as well.
                Rfc822Token[] tokenized = Rfc822Tokenizer.tokenize(address);
                if (tokenized != null && tokenized.length > 0) {
                    address = tokenized[0].getAddress();
                }
            }
            Rfc822Token token = new Rfc822Token(display, address, null);
            trimmedDisplayText = token.toString().trim();
        }
        int index = trimmedDisplayText.indexOf(",");
        return mTokenizer != null && !TextUtils.isEmpty(trimmedDisplayText)
                && index < trimmedDisplayText.length() - 1 ? (String) mTokenizer
                .terminateToken(trimmedDisplayText) : trimmedDisplayText;
    }

    // Visible for testing.
    // Use this method to generate text to display in a chip.
    /*package*/ String createChipDisplayText(RecipientEntry entry) {
        String display = entry.getDisplayName();
        String address = entry.getDestination();
        if (TextUtils.isEmpty(display) || TextUtils.equals(display, address)) {
            display = null;
        }
        if (!TextUtils.isEmpty(display)) {
            return display;
        } else if (!TextUtils.isEmpty(address)){
            return address;
        } else {
            return new Rfc822Token(display, address, null).toString();
        }
    }

    private CharSequence createChip(RecipientEntry entry) {
        final String displayText = createAddressText(entry);
        if (TextUtils.isEmpty(displayText)) {
            return null;
        }
        // Always leave a blank space at the end of a chip.
        final int textLength = displayText.length() - 1;
        final SpannableString  chipText = new SpannableString(displayText);
        if (!mNoChipMode) {
            try {
                DrawableRecipientChip chip = constructChipSpan(entry);
                chipText.setSpan(chip, 0, textLength,
                        Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                chip.setOriginalText(chipText.toString());
            } catch (NullPointerException e) {
                Log.e(TAG, e.getMessage(), e);
                return null;
            }
        }
        onChipCreated(entry);
        return chipText;
    }

    /**
     * A callback for subclasses to use to know when a chip was created with the
     * given RecipientEntry.
     */
    protected void onChipCreated(RecipientEntry entry) {
        if (!mNoChipMode && mRecipientChipAddedListener != null) {
            mRecipientChipAddedListener.onRecipientChipAdded(entry);
        }
    }

    /**
     * When an item in the suggestions list has been clicked, create a chip from the
     * contact information of the selected item.
     */
    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (position < 0) {
            return;
        }

        final RecipientEntry entry = getAdapter().getItem(position);
        if (entry.getEntryType() == RecipientEntry.ENTRY_TYPE_PERMISSION_REQUEST) {
            if (mPermissionsRequestItemClickedListener != null) {
                mPermissionsRequestItemClickedListener
                        .onPermissionsRequestItemClicked(this, entry.getPermissions());
            }
            return;
        }

        final int charactersTyped = submitItemAtPosition(position);
        if (charactersTyped > -1 && mRecipientEntryItemClickedListener != null) {
            mRecipientEntryItemClickedListener
                    .onRecipientEntryItemClicked(charactersTyped, position);
        }
    }

    private int submitItemAtPosition(int position) {
        RecipientEntry entry = createValidatedEntry(getAdapter().getItem(position));
        if (entry == null) {
            return -1;
        }
        clearComposingText();

        int end = getSelectionEnd();
        int start = mTokenizer.findTokenStart(getText(), end);

        Editable editable = getText();
        QwertyKeyListener.markAsReplaced(editable, start, end, "");
        CharSequence chip = createChip(entry);
        if (chip != null && start >= 0 && end >= 0) {
            editable.replace(start, end, chip);
        }
        sanitizeBetween();

        return end - start;
    }

    private RecipientEntry createValidatedEntry(RecipientEntry item) {
        if (item == null) {
            return null;
        }
        final RecipientEntry entry;
        // If the display name and the address are the same, or if this is a
        // valid contact, but the destination is invalid, then make this a fake
        // recipient that is editable.
        String destination = item.getDestination();
        if (!isPhoneQuery() && item.getContactId() == RecipientEntry.GENERATED_CONTACT) {
            entry = RecipientEntry.constructGeneratedEntry(item.getDisplayName(),
                    destination, item.isValid());
        } else if (RecipientEntry.isCreatedRecipient(item.getContactId())
                && (TextUtils.isEmpty(item.getDisplayName())
                        || TextUtils.equals(item.getDisplayName(), destination)
                        || (mValidator != null && !mValidator.isValid(destination)))) {
            entry = RecipientEntry.constructFakeEntry(destination, item.isValid());
        } else {
            entry = item;
        }
        return entry;
    }

    // Visible for testing.
    /* package */DrawableRecipientChip[] getSortedRecipients() {
        DrawableRecipientChip[] recips = getSpannable()
                .getSpans(0, getText().length(), DrawableRecipientChip.class);
        ArrayList<DrawableRecipientChip> recipientsList = new ArrayList<DrawableRecipientChip>(
                Arrays.asList(recips));
        final Spannable spannable = getSpannable();
        Collections.sort(recipientsList, new Comparator<DrawableRecipientChip>() {

            @Override
            public int compare(DrawableRecipientChip first, DrawableRecipientChip second) {
                int firstStart = spannable.getSpanStart(first);
                int secondStart = spannable.getSpanStart(second);
                if (firstStart < secondStart) {
                    return -1;
                } else if (firstStart > secondStart) {
                    return 1;
                } else {
                    return 0;
                }
            }
        });
        return recipientsList.toArray(new DrawableRecipientChip[recipientsList.size()]);
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
        return false;
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
        return false;
    }

    /**
     * No chips are selectable.
     */
    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
        return false;
    }

    // Visible for testing.
    /* package */ReplacementDrawableSpan getMoreChip() {
        MoreImageSpan[] moreSpans = getSpannable().getSpans(0, getText().length(),
                MoreImageSpan.class);
        return moreSpans != null && moreSpans.length > 0 ? moreSpans[0] : null;
    }

    private MoreImageSpan createMoreSpan(int count) {
        String moreText = String.format(mMoreItem.getText().toString(), count);
        mWorkPaint.set(getPaint());
        mWorkPaint.setTextSize(mMoreItem.getTextSize());
        mWorkPaint.setColor(mMoreItem.getCurrentTextColor());
        final int width = (int) mWorkPaint.measureText(moreText) + mMoreItem.getPaddingLeft()
                + mMoreItem.getPaddingRight();
        final int height = (int) mChipHeight;
        Bitmap drawable = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(drawable);
        int adjustedHeight = height;
        Layout layout = getLayout();
        if (layout != null) {
            adjustedHeight -= layout.getLineDescent(0);
        }
        canvas.drawText(moreText, 0, moreText.length(), 0, adjustedHeight, mWorkPaint);

        Drawable result = new BitmapDrawable(getResources(), drawable);
        result.setBounds(0, 0, width, height);
        return new MoreImageSpan(result);
    }

    // Visible for testing.
    /*package*/ void createMoreChipPlainText() {
        // Take the first <= CHIP_LIMIT addresses and get to the end of the second one.
        Editable text = getText();
        int start = 0;
        int end = start;
        for (int i = 0; i < CHIP_LIMIT; i++) {
            end = movePastTerminators(mTokenizer.findTokenEnd(text, start));
            start = end; // move to the next token and get its end.
        }
        // Now, count total addresses.
        int tokenCount = countTokens(text);
        MoreImageSpan moreSpan = createMoreSpan(tokenCount - CHIP_LIMIT);
        SpannableString chipText = new SpannableString(text.subSequence(end, text.length()));
        chipText.setSpan(moreSpan, 0, chipText.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        text.replace(end, text.length(), chipText);
        mMoreChip = moreSpan;
    }

    // Visible for testing.
    /* package */int countTokens(Editable text) {
        int tokenCount = 0;
        int start = 0;
        while (start < text.length()) {
            start = movePastTerminators(mTokenizer.findTokenEnd(text, start));
            tokenCount++;
            if (start >= text.length()) {
                break;
            }
        }
        return tokenCount;
    }

    /**
     * Create the more chip. The more chip is text that replaces any chips that
     * do not fit in the pre-defined available space when the
     * RecipientEditTextView loses focus.
     */
    // Visible for testing.
    /* package */ void createMoreChip() {
        if (mNoChipMode) {
            createMoreChipPlainText();
            return;
        }

        if (!mShouldShrink) {
            return;
        }
        ReplacementDrawableSpan[] tempMore = getSpannable().getSpans(0, getText().length(),
                MoreImageSpan.class);
        if (tempMore.length > 0) {
            getSpannable().removeSpan(tempMore[0]);
        }
        DrawableRecipientChip[] recipients = getSortedRecipients();

        if (recipients == null || recipients.length <= CHIP_LIMIT) {
            mMoreChip = null;
            return;
        }
        Spannable spannable = getSpannable();
        int numRecipients = recipients.length;
        int overage = numRecipients - CHIP_LIMIT;
        MoreImageSpan moreSpan = createMoreSpan(overage);
        mHiddenSpans = new ArrayList<DrawableRecipientChip>();
        int totalReplaceStart = 0;
        int totalReplaceEnd = 0;
        Editable text = getText();
        for (int i = numRecipients - overage; i < recipients.length; i++) {
            mHiddenSpans.add(recipients[i]);
            if (i == numRecipients - overage) {
                totalReplaceStart = spannable.getSpanStart(recipients[i]);
            }
            if (i == recipients.length - 1) {
                totalReplaceEnd = spannable.getSpanEnd(recipients[i]);
            }
            if (mTemporaryRecipients == null || !mTemporaryRecipients.contains(recipients[i])) {
                int spanStart = spannable.getSpanStart(recipients[i]);
                int spanEnd = spannable.getSpanEnd(recipients[i]);
                recipients[i].setOriginalText(text.toString().substring(spanStart, spanEnd));
            }
            spannable.removeSpan(recipients[i]);
        }
        if (totalReplaceEnd < text.length()) {
            totalReplaceEnd = text.length();
        }
        int end = Math.max(totalReplaceStart, totalReplaceEnd);
        int start = Math.min(totalReplaceStart, totalReplaceEnd);
        SpannableString chipText = new SpannableString(text.subSequence(start, end));
        chipText.setSpan(moreSpan, 0, chipText.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        text.replace(start, end, chipText);
        mMoreChip = moreSpan;
        // If adding the +more chip goes over the limit, resize accordingly.
        if (!isPhoneQuery() && getLineCount() > mMaxLines) {
            setMaxLines(getLineCount());
        }
    }

    /**
     * Replace the more chip, if it exists, with all of the recipient chips it had
     * replaced when the RecipientEditTextView gains focus.
     */
    // Visible for testing.
    /*package*/ void removeMoreChip() {
        if (mMoreChip != null) {
            Spannable span = getSpannable();
            span.removeSpan(mMoreChip);
            mMoreChip = null;
            // Re-add the spans that were hidden.
            if (mHiddenSpans != null && mHiddenSpans.size() > 0) {
                // Recreate each hidden span.
                DrawableRecipientChip[] recipients = getSortedRecipients();
                // Start the search for tokens after the last currently visible
                // chip.
                if (recipients == null || recipients.length == 0) {
                    return;
                }
                int end = span.getSpanEnd(recipients[recipients.length - 1]);
                Editable editable = getText();
                for (DrawableRecipientChip chip : mHiddenSpans) {
                    int chipStart;
                    int chipEnd;
                    String token;
                    // Need to find the location of the chip, again.
                    token = (String) chip.getOriginalText();
                    // As we find the matching recipient for the hidden spans,
                    // reduce the size of the string we need to search.
                    // That way, if there are duplicates, we always find the correct
                    // recipient.
                    chipStart = editable.toString().indexOf(token, end);
                    end = chipEnd = Math.min(editable.length(), chipStart + token.length());
                    // Only set the span if we found a matching token.
                    if (chipStart != -1) {
                        editable.setSpan(chip, chipStart, chipEnd,
                                Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                    }
                }
                mHiddenSpans.clear();
            }
        }
    }

    /**
     * Show specified chip as selected. If the RecipientChip is just an email address,
     * selecting the chip will take the contents of the chip and place it at
     * the end of the RecipientEditTextView for inline editing. If the
     * RecipientChip is a complete contact, then selecting the chip
     * will show a popup window with the address in use highlighted and any other
     * alternate addresses for the contact.
     * @param currentChip Chip to select.
     */
    private void selectChip(DrawableRecipientChip currentChip) {
        if (shouldShowEditableText(currentChip)) {
            CharSequence text = currentChip.getValue();
            Editable editable = getText();
            Spannable spannable = getSpannable();
            int spanStart = spannable.getSpanStart(currentChip);
            int spanEnd = spannable.getSpanEnd(currentChip);
            spannable.removeSpan(currentChip);
            // Don't need leading space if it's the only chip
            if (spanEnd - spanStart == editable.length() - 1) {
                spanEnd++;
            }
            editable.delete(spanStart, spanEnd);
            setCursorVisible(true);
            setSelection(editable.length());
            editable.append(text);
            mSelectedChip = constructChipSpan(
                    RecipientEntry.constructFakeEntry((String) text, isValid(text.toString())));

            /*
             * Because chip is destroyed and converted into an editable text, we call
             * {@link RecipientChipDeletedListener#onRecipientChipDeleted}. For the cases where
             * editable text is not shown (i.e. chip is in user's contact list), chip is focused
             * and below callback is not called.
             */
            if (!mNoChipMode && mRecipientChipDeletedListener != null) {
                mRecipientChipDeletedListener.onRecipientChipDeleted(currentChip.getEntry());
            }
        } else {
            final boolean showAddress =
                    currentChip.getContactId() == RecipientEntry.GENERATED_CONTACT ||
                    getAdapter().forceShowAddress();
            if (showAddress && mNoChipMode) {
                return;
            }

            if (isTouchExplorationEnabled()) {
                // The chips cannot be touch-explored. However, doing a double-tap results in
                // the popup being shown for the last chip, which is of no value.
                return;
            }

            mSelectedChip = currentChip;
            setSelection(getText().getSpanEnd(mSelectedChip));
            setCursorVisible(false);

            if (showAddress) {
                showAddress(currentChip, mAddressPopup);
            } else {
                showAlternates(currentChip, mAlternatesPopup);
            }
        }
    }

    private boolean isTouchExplorationEnabled() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            return false;
        }

        final AccessibilityManager accessibilityManager = (AccessibilityManager)
                getContext().getSystemService(Context.ACCESSIBILITY_SERVICE);
        return accessibilityManager.isTouchExplorationEnabled();
    }

    private boolean shouldShowEditableText(DrawableRecipientChip currentChip) {
        long contactId = currentChip.getContactId();
        return contactId == RecipientEntry.INVALID_CONTACT
                || (!isPhoneQuery() && contactId == RecipientEntry.GENERATED_CONTACT);
    }

    private void showAddress(final DrawableRecipientChip currentChip, final ListPopupWindow popup) {
        if (!mAttachedToWindow) {
            return;
        }
        int line = getLayout().getLineForOffset(getChipStart(currentChip));
        int bottomOffset = calculateOffsetFromBottomToTop(line);
        // Align the alternates popup with the left side of the View,
        // regardless of the position of the chip tapped.
        popup.setAnchorView((mAlternatePopupAnchor != null) ? mAlternatePopupAnchor : this);
        popup.setVerticalOffset(bottomOffset);
        popup.setAdapter(createSingleAddressAdapter(currentChip));
        popup.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                unselectChip(currentChip);
                popup.dismiss();
            }
        });
        popup.show();
        ListView listView = popup.getListView();
        listView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        listView.setItemChecked(0, true);
    }

    /**
     * Remove selection from this chip. Unselecting a RecipientChip will render
     * the chip without a delete icon and with an unfocused background. This is
     * called when the RecipientChip no longer has focus.
     */
    private void unselectChip(DrawableRecipientChip chip) {
        int start = getChipStart(chip);
        int end = getChipEnd(chip);
        Editable editable = getText();
        mSelectedChip = null;
        if (start == -1 || end == -1) {
            Log.w(TAG, "The chip doesn't exist or may be a chip a user was editing");
            setSelection(editable.length());
            commitDefault();
        } else {
            getSpannable().removeSpan(chip);
            QwertyKeyListener.markAsReplaced(editable, start, end, "");
            editable.removeSpan(chip);
            try {
                if (!mNoChipMode) {
                    editable.setSpan(constructChipSpan(chip.getEntry()),
                            start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
            } catch (NullPointerException e) {
                Log.e(TAG, e.getMessage(), e);
            }
        }
        setCursorVisible(true);
        setSelection(editable.length());
        if (mAlternatesPopup != null && mAlternatesPopup.isShowing()) {
            mAlternatesPopup.dismiss();
        }
    }

    @Override
    public void onChipDelete() {
        if (mSelectedChip != null) {
            if (!mNoChipMode && mRecipientChipDeletedListener != null) {
                mRecipientChipDeletedListener.onRecipientChipDeleted(mSelectedChip.getEntry());
            }
            removeChip(mSelectedChip);
        }
        dismissPopups();
    }

    @Override
    public void onPermissionRequestDismissed() {
        if (mPermissionsRequestItemClickedListener != null) {
            mPermissionsRequestItemClickedListener.onPermissionRequestDismissed();
        }
        dismissDropDown();
    }

    private void dismissPopups() {
        if (mAlternatesPopup != null && mAlternatesPopup.isShowing()) {
            mAlternatesPopup.dismiss();
        }
        if (mAddressPopup != null && mAddressPopup.isShowing()) {
            mAddressPopup.dismiss();
        }
        setSelection(getText().length());
    }

    /**
     * Remove the chip and any text associated with it from the RecipientEditTextView.
     */
    // Visible for testing.
    /* package */void removeChip(DrawableRecipientChip chip) {
        Spannable spannable = getSpannable();
        int spanStart = spannable.getSpanStart(chip);
        int spanEnd = spannable.getSpanEnd(chip);
        Editable text = getText();
        int toDelete = spanEnd;
        boolean wasSelected = chip == mSelectedChip;
        // Clear that there is a selected chip before updating any text.
        if (wasSelected) {
            mSelectedChip = null;
        }
        // Always remove trailing spaces when removing a chip.
        while (toDelete >= 0 && toDelete < text.length() && text.charAt(toDelete) == ' ') {
            toDelete++;
        }
        spannable.removeSpan(chip);
        if (spanStart >= 0 && toDelete > 0) {
            text.delete(spanStart, toDelete);
        }
        if (wasSelected) {
            clearSelectedChip();
        }
    }

    /**
     * Replace this currently selected chip with a new chip
     * that uses the contact data provided.
     */
    // Visible for testing.
    /*package*/ void replaceChip(DrawableRecipientChip chip, RecipientEntry entry) {
        boolean wasSelected = chip == mSelectedChip;
        if (wasSelected) {
            mSelectedChip = null;
        }
        int start = getChipStart(chip);
        int end = getChipEnd(chip);
        getSpannable().removeSpan(chip);
        Editable editable = getText();
        CharSequence chipText = createChip(entry);
        if (chipText != null) {
            if (start == -1 || end == -1) {
                Log.e(TAG, "The chip to replace does not exist but should.");
                editable.insert(0, chipText);
            } else {
                if (!TextUtils.isEmpty(chipText)) {
                    // There may be a space to replace with this chip's new
                    // associated space. Check for it
                    int toReplace = end;
                    while (toReplace >= 0 && toReplace < editable.length()
                            && editable.charAt(toReplace) == ' ') {
                        toReplace++;
                    }
                    editable.replace(start, toReplace, chipText);
                }
            }
        }
        setCursorVisible(true);
        if (wasSelected) {
            clearSelectedChip();
        }
    }

    /**
     * Handle click events for a chip. When a selected chip receives a click
     * event, see if that event was in the delete icon. If so, delete it.
     * Otherwise, unselect the chip.
     */
    public void onClick(DrawableRecipientChip chip) {
        if (chip.isSelected()) {
            clearSelectedChip();
        }
    }

    private boolean chipsPending() {
        return mPendingChipsCount > 0 || (mHiddenSpans != null && mHiddenSpans.size() > 0);
    }

    @Override
    public void removeTextChangedListener(TextWatcher watcher) {
        mTextWatcher = null;
        super.removeTextChangedListener(watcher);
    }

    private boolean isValidEmailAddress(String input) {
        return !TextUtils.isEmpty(input) && mValidator != null &&
                mValidator.isValid(input);
    }

    private class RecipientTextWatcher implements TextWatcher {

        @Override
        public void afterTextChanged(Editable s) {
            // If the text has been set to null or empty, make sure we remove
            // all the spans we applied.
            if (TextUtils.isEmpty(s)) {
                // Remove all the chips spans.
                Spannable spannable = getSpannable();
                DrawableRecipientChip[] chips = spannable.getSpans(0, getText().length(),
                        DrawableRecipientChip.class);
                for (DrawableRecipientChip chip : chips) {
                    spannable.removeSpan(chip);
                }
                if (mMoreChip != null) {
                    spannable.removeSpan(mMoreChip);
                }
                clearSelectedChip();
                return;
            }
            // Get whether there are any recipients pending addition to the
            // view. If there are, don't do anything in the text watcher.
            if (chipsPending()) {
                return;
            }
            // If the user is editing a chip, don't clear it.
            if (mSelectedChip != null) {
                if (!isGeneratedContact(mSelectedChip)) {
                    setCursorVisible(true);
                    setSelection(getText().length());
                    clearSelectedChip();
                } else {
                    return;
                }
            }
            int length = s.length();
            // Make sure there is content there to parse and that it is
            // not just the commit character.
            if (length > 1) {
                if (lastCharacterIsCommitCharacter(s)) {
                    commitByCharacter();
                    return;
                }
                char last;
                int end = getSelectionEnd() == 0 ? 0 : getSelectionEnd() - 1;
                int len = length() - 1;
                if (end != len) {
                    last = s.charAt(end);
                } else {
                    last = s.charAt(len);
                }
                if (last == COMMIT_CHAR_SPACE) {
                    if (!isPhoneQuery()) {
                        // Check if this is a valid email address. If it is,
                        // commit it.
                        String text = getText().toString();
                        int tokenStart = mTokenizer.findTokenStart(text, getSelectionEnd());
                        String sub = text.substring(tokenStart, mTokenizer.findTokenEnd(text,
                                tokenStart));
                        if (isValidEmailAddress(sub)) {
                            commitByCharacter();
                        }
                    }
                }
            }
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            // The user deleted some text OR some text was replaced; check to
            // see if the insertion point is on a space
            // following a chip.
            if (before - count == 1) {
                // If the item deleted is a space, and the thing before the
                // space is a chip, delete the entire span.
                int selStart = getSelectionStart();
                DrawableRecipientChip[] repl = getSpannable().getSpans(selStart, selStart,
                        DrawableRecipientChip.class);
                if (repl.length > 0) {
                    // There is a chip there! Just remove it.
                    DrawableRecipientChip toDelete = repl[0];
                    Editable editable = getText();
                    // Add the separator token.
                    int deleteStart = editable.getSpanStart(toDelete);
                    int deleteEnd = editable.getSpanEnd(toDelete) + 1;
                    if (deleteEnd > editable.length()) {
                        deleteEnd = editable.length();
                    }
                    if (!mNoChipMode && mRecipientChipDeletedListener != null) {
                        mRecipientChipDeletedListener.onRecipientChipDeleted(toDelete.getEntry());
                    }
                    editable.removeSpan(toDelete);
                    editable.delete(deleteStart, deleteEnd);
                }
            } else if (count > before) {
                if (mSelectedChip != null
                    && isGeneratedContact(mSelectedChip)) {
                    if (lastCharacterIsCommitCharacter(s)) {
                        commitByCharacter();
                        return;
                    }
                }
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            // Do nothing.
        }
    }

    public boolean lastCharacterIsCommitCharacter(CharSequence s) {
        char last;
        int end = getSelectionEnd() == 0 ? 0 : getSelectionEnd() - 1;
        int len = length() - 1;
        if (end != len) {
            last = s.charAt(end);
        } else {
            last = s.charAt(len);
        }
        return last == COMMIT_CHAR_COMMA || last == COMMIT_CHAR_SEMICOLON;
    }

    public boolean isGeneratedContact(DrawableRecipientChip chip) {
        long contactId = chip.getContactId();
        return contactId == RecipientEntry.INVALID_CONTACT
                || (!isPhoneQuery() && contactId == RecipientEntry.GENERATED_CONTACT);
    }

    /**
     * Handles pasting a {@link ClipData} to this {@link RecipientEditTextView}.
     */
    // Visible for testing.
    void handlePasteClip(ClipData clip) {
        if (clip == null) {
            // Do nothing.
            return;
        }

        final ClipDescription clipDesc = clip.getDescription();
        boolean containsSupportedType = clipDesc.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN) ||
                clipDesc.hasMimeType(ClipDescription.MIMETYPE_TEXT_HTML);
        if (!containsSupportedType) {
            return;
        }

        removeTextChangedListener(mTextWatcher);

        final ClipDescription clipDescription = clip.getDescription();
        for (int i = 0; i < clip.getItemCount(); i++) {
            final String mimeType = clipDescription.getMimeType(i);
            final boolean supportedType = ClipDescription.MIMETYPE_TEXT_PLAIN.equals(mimeType) ||
                    ClipDescription.MIMETYPE_TEXT_HTML.equals(mimeType);
            if (!supportedType) {
                // Only plain text and html can be pasted.
                continue;
            }

            final CharSequence pastedItem = clip.getItemAt(i).getText();
            if (!TextUtils.isEmpty(pastedItem)) {
                final Editable editable = getText();
                final int start = getSelectionStart();
                final int end = getSelectionEnd();
                if (start < 0 || end < 1) {
                    // No selection.
                    editable.append(pastedItem);
                } else if (start == end) {
                    // Insert at position.
                    editable.insert(start, pastedItem);
                } else {
                    editable.append(pastedItem, start, end);
                }
                handlePasteAndReplace();
            }
        }

        mHandler.post(mAddTextWatcher);
    }

    @Override
    public boolean onTextContextMenuItem(int id) {
        if (id == android.R.id.paste) {
            ClipboardManager clipboard = (ClipboardManager) getContext().getSystemService(
                    Context.CLIPBOARD_SERVICE);
            handlePasteClip(clipboard.getPrimaryClip());
            return true;
        }
        return super.onTextContextMenuItem(id);
    }

    private void handlePasteAndReplace() {
        ArrayList<DrawableRecipientChip> created = handlePaste();
        if (created != null && created.size() > 0) {
            // Perform reverse lookups on the pasted contacts.
            IndividualReplacementTask replace = new IndividualReplacementTask();
            replace.execute(created);
        }
    }

    // Visible for testing.
    /* package */ArrayList<DrawableRecipientChip> handlePaste() {
        String text = getText().toString();
        int originalTokenStart = mTokenizer.findTokenStart(text, getSelectionEnd());
        String lastAddress = text.substring(originalTokenStart);
        int tokenStart = originalTokenStart;
        int prevTokenStart = 0;
        DrawableRecipientChip findChip = null;
        ArrayList<DrawableRecipientChip> created = new ArrayList<DrawableRecipientChip>();
        if (tokenStart != 0) {
            // There are things before this!
            while (tokenStart != 0 && findChip == null && tokenStart != prevTokenStart) {
                prevTokenStart = tokenStart;
                tokenStart = mTokenizer.findTokenStart(text, tokenStart);
                findChip = findChip(tokenStart);
                if (tokenStart == originalTokenStart && findChip == null) {
                    break;
                }
            }
            if (tokenStart != originalTokenStart) {
                if (findChip != null) {
                    tokenStart = prevTokenStart;
                }
                int tokenEnd;
                DrawableRecipientChip createdChip;
                while (tokenStart < originalTokenStart) {
                    tokenEnd = movePastTerminators(mTokenizer.findTokenEnd(getText().toString(),
                            tokenStart));
                    commitChip(tokenStart, tokenEnd, getText());
                    createdChip = findChip(tokenStart);
                    if (createdChip == null) {
                        break;
                    }
                    // +1 for the space at the end.
                    tokenStart = getSpannable().getSpanEnd(createdChip) + 1;
                    created.add(createdChip);
                }
            }
        }
        // Take a look at the last token. If the token has been completed with a
        // commit character, create a chip.
        if (isCompletedToken(lastAddress)) {
            Editable editable = getText();
            tokenStart = editable.toString().indexOf(lastAddress, originalTokenStart);
            commitChip(tokenStart, editable.length(), editable);
            created.add(findChip(tokenStart));
        }
        return created;
    }

    // Visible for testing.
    /* package */int movePastTerminators(int tokenEnd) {
        if (tokenEnd >= length()) {
            return tokenEnd;
        }
        char atEnd = getText().toString().charAt(tokenEnd);
        if (atEnd == COMMIT_CHAR_COMMA || atEnd == COMMIT_CHAR_SEMICOLON) {
            tokenEnd++;
        }
        // This token had not only an end token character, but also a space
        // separating it from the next token.
        if (tokenEnd < length() && getText().toString().charAt(tokenEnd) == ' ') {
            tokenEnd++;
        }
        return tokenEnd;
    }

    private class RecipientReplacementTask extends AsyncTask<Void, Void, Void> {
        private DrawableRecipientChip createFreeChip(RecipientEntry entry) {
            try {
                if (mNoChipMode) {
                    return null;
                }
                return constructChipSpan(entry);
            } catch (NullPointerException e) {
                Log.e(TAG, e.getMessage(), e);
                return null;
            }
        }

        @Override
        protected void onPreExecute() {
            // Ensure everything is in chip-form already, so we don't have text that slowly gets
            // replaced
            final List<DrawableRecipientChip> originalRecipients =
                    new ArrayList<DrawableRecipientChip>();
            final DrawableRecipientChip[] existingChips = getSortedRecipients();
            Collections.addAll(originalRecipients, existingChips);
            if (mHiddenSpans != null) {
                originalRecipients.addAll(mHiddenSpans);
            }

            final List<DrawableRecipientChip> replacements =
                    new ArrayList<DrawableRecipientChip>(originalRecipients.size());

            for (final DrawableRecipientChip chip : originalRecipients) {
                if (RecipientEntry.isCreatedRecipient(chip.getEntry().getContactId())
                        && getSpannable().getSpanStart(chip) != -1) {
                    replacements.add(createFreeChip(chip.getEntry()));
                } else {
                    replacements.add(null);
                }
            }

            processReplacements(originalRecipients, replacements);
        }

        @Override
        protected Void doInBackground(Void... params) {
            if (mIndividualReplacements != null) {
                mIndividualReplacements.cancel(true);
            }
            // For each chip in the list, look up the matching contact.
            // If there is a match, replace that chip with the matching
            // chip.
            final ArrayList<DrawableRecipientChip> recipients =
                    new ArrayList<DrawableRecipientChip>();
            DrawableRecipientChip[] existingChips = getSortedRecipients();
            Collections.addAll(recipients, existingChips);
            if (mHiddenSpans != null) {
                recipients.addAll(mHiddenSpans);
            }
            ArrayList<String> addresses = new ArrayList<String>();
            for (DrawableRecipientChip chip : recipients) {
                if (chip != null) {
                    addresses.add(createAddressText(chip.getEntry()));
                }
            }
            final BaseRecipientAdapter adapter = getAdapter();
            adapter.getMatchingRecipients(addresses, new RecipientMatchCallback() {
                        @Override
                        public void matchesFound(Map<String, RecipientEntry> entries) {
                            final ArrayList<DrawableRecipientChip> replacements =
                                    new ArrayList<DrawableRecipientChip>();
                            for (final DrawableRecipientChip temp : recipients) {
                                RecipientEntry entry = null;
                                if (temp != null && RecipientEntry.isCreatedRecipient(
                                        temp.getEntry().getContactId())
                                        && getSpannable().getSpanStart(temp) != -1) {
                                    // Replace this.
                                    entry = createValidatedEntry(
                                            entries.get(tokenizeAddress(temp.getEntry()
                                                    .getDestination())));
                                }
                                if (entry != null) {
                                    replacements.add(createFreeChip(entry));
                                } else {
                                    replacements.add(null);
                                }
                            }
                            processReplacements(recipients, replacements);
                        }

                        @Override
                        public void matchesNotFound(final Set<String> unfoundAddresses) {
                            final List<DrawableRecipientChip> replacements =
                                    new ArrayList<DrawableRecipientChip>(unfoundAddresses.size());

                            for (final DrawableRecipientChip temp : recipients) {
                                if (temp != null && RecipientEntry.isCreatedRecipient(
                                        temp.getEntry().getContactId())
                                        && getSpannable().getSpanStart(temp) != -1) {
                                    if (unfoundAddresses.contains(
                                            temp.getEntry().getDestination())) {
                                        replacements.add(createFreeChip(temp.getEntry()));
                                    } else {
                                        replacements.add(null);
                                    }
                                } else {
                                    replacements.add(null);
                                }
                            }

                            processReplacements(recipients, replacements);
                        }
                    });
            return null;
        }

        private void processReplacements(final List<DrawableRecipientChip> recipients,
                final List<DrawableRecipientChip> replacements) {
            if (replacements != null && replacements.size() > 0) {
                final Runnable runnable = new Runnable() {
                    @Override
                    public void run() {
                        final Editable text = new SpannableStringBuilder(getText());
                        int i = 0;
                        for (final DrawableRecipientChip chip : recipients) {
                            final DrawableRecipientChip replacement = replacements.get(i);
                            if (replacement != null) {
                                final RecipientEntry oldEntry = chip.getEntry();
                                final RecipientEntry newEntry = replacement.getEntry();
                                final boolean isBetter =
                                        RecipientAlternatesAdapter.getBetterRecipient(
                                                oldEntry, newEntry) == newEntry;

                                if (isBetter) {
                                    // Find the location of the chip in the text currently shown.
                                    final int start = text.getSpanStart(chip);
                                    if (start != -1) {
                                        // Replacing the entirety of what the chip represented,
                                        // including the extra space dividing it from other chips.
                                        final int end =
                                                Math.min(text.getSpanEnd(chip) + 1, text.length());
                                        text.removeSpan(chip);
                                        // Make sure we always have just 1 space at the end to
                                        // separate this chip from the next chip.
                                        final SpannableString displayText =
                                                new SpannableString(createAddressText(
                                                        replacement.getEntry()).trim() + " ");
                                        displayText.setSpan(replacement, 0,
                                                displayText.length() - 1,
                                                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                                        // Replace the old text we found with with the new display
                                        // text, which now may also contain the display name of the
                                        // recipient.
                                        text.replace(start, end, displayText);
                                        replacement.setOriginalText(displayText.toString());
                                        replacements.set(i, null);

                                        recipients.set(i, replacement);
                                    }
                                }
                            }
                            i++;
                        }
                        setText(text);
                    }
                };

                if (Looper.myLooper() == Looper.getMainLooper()) {
                    runnable.run();
                } else {
                    mHandler.post(runnable);
                }
            }
        }
    }

    private class IndividualReplacementTask
            extends AsyncTask<ArrayList<DrawableRecipientChip>, Void, Void> {
        @Override
        protected Void doInBackground(ArrayList<DrawableRecipientChip>... params) {
            // For each chip in the list, look up the matching contact.
            // If there is a match, replace that chip with the matching
            // chip.
            final ArrayList<DrawableRecipientChip> originalRecipients = params[0];
            ArrayList<String> addresses = new ArrayList<String>();
            for (DrawableRecipientChip chip : originalRecipients) {
                if (chip != null) {
                    addresses.add(createAddressText(chip.getEntry()));
                }
            }
            final BaseRecipientAdapter adapter = getAdapter();
            adapter.getMatchingRecipients(addresses, new RecipientMatchCallback() {

                        @Override
                        public void matchesFound(Map<String, RecipientEntry> entries) {
                            for (final DrawableRecipientChip temp : originalRecipients) {
                                if (RecipientEntry.isCreatedRecipient(temp.getEntry()
                                        .getContactId())
                                        && getSpannable().getSpanStart(temp) != -1) {
                                    // Replace this.
                                    final RecipientEntry entry = createValidatedEntry(entries
                                            .get(tokenizeAddress(temp.getEntry().getDestination())
                                                    .toLowerCase()));
                                    if (entry != null) {
                                        mHandler.post(new Runnable() {
                                            @Override
                                            public void run() {
                                                replaceChip(temp, entry);
                                            }
                                        });
                                    }
                                }
                            }
                        }

                        @Override
                        public void matchesNotFound(final Set<String> unfoundAddresses) {
                            // No action required
                        }
                    });
            return null;
        }
    }


    /**
     * MoreImageSpan is a simple class created for tracking the existence of a
     * more chip across activity restarts/
     */
    private class MoreImageSpan extends ReplacementDrawableSpan {
        public MoreImageSpan(Drawable b) {
            super(b);
            setExtraMargin(mLineSpacingExtra);
        }
    }

    @Override
    public boolean onDown(MotionEvent e) {
        return false;
    }

    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        // Do nothing.
        return false;
    }

    @Override
    public void onLongPress(MotionEvent event) {
        if (mSelectedChip != null) {
            return;
        }
        float x = event.getX();
        float y = event.getY();
        final int offset = putOffsetInRange(x, y);
        DrawableRecipientChip currentChip = findChip(offset);
        if (currentChip != null) {
            if (mDragEnabled) {
                // Start drag-and-drop for the selected chip.
                startDrag(currentChip);
            } else {
                // Copy the selected chip email address.
                showCopyDialog(currentChip.getEntry().getDestination());
            }
        }
    }

    // The following methods are used to provide some functionality on older versions of Android
    // These methods were copied out of JB MR2's TextView
    /////////////////////////////////////////////////
    private int supportGetOffsetForPosition(float x, float y) {
        if (getLayout() == null) return -1;
        final int line = supportGetLineAtCoordinate(y);
        return supportGetOffsetAtCoordinate(line, x);
    }

    private float supportConvertToLocalHorizontalCoordinate(float x) {
        x -= getTotalPaddingLeft();
        // Clamp the position to inside of the view.
        x = Math.max(0.0f, x);
        x = Math.min(getWidth() - getTotalPaddingRight() - 1, x);
        x += getScrollX();
        return x;
    }

    private int supportGetLineAtCoordinate(float y) {
        y -= getTotalPaddingLeft();
        // Clamp the position to inside of the view.
        y = Math.max(0.0f, y);
        y = Math.min(getHeight() - getTotalPaddingBottom() - 1, y);
        y += getScrollY();
        return getLayout().getLineForVertical((int) y);
    }

    private int supportGetOffsetAtCoordinate(int line, float x) {
        x = supportConvertToLocalHorizontalCoordinate(x);
        return getLayout().getOffsetForHorizontal(line, x);
    }
    /////////////////////////////////////////////////

    /**
     * Enables drag-and-drop for chips.
     */
    public void enableDrag() {
        mDragEnabled = true;
    }

    /**
     * Starts drag-and-drop for the selected chip.
     */
    private void startDrag(DrawableRecipientChip currentChip) {
        String address = currentChip.getEntry().getDestination();
        ClipData data = ClipData.newPlainText(address, address + COMMIT_CHAR_COMMA);

        // Start drag mode.
        startDrag(data, new RecipientChipShadow(currentChip), null, 0);

        // Remove the current chip, so drag-and-drop will result in a move.
        // TODO (phamm): consider readd this chip if it's dropped outside a target.
        removeChip(currentChip);
    }

    /**
     * Handles drag event.
     */
    @Override
    public boolean onDragEvent(@NonNull DragEvent event) {
        switch (event.getAction()) {
            case DragEvent.ACTION_DRAG_STARTED:
                // Only handle plain text drag and drop.
                return event.getClipDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN);
            case DragEvent.ACTION_DRAG_ENTERED:
                requestFocus();
                return true;
            case DragEvent.ACTION_DROP:
                handlePasteClip(event.getClipData());
                return true;
        }
        return false;
    }

    /**
     * Drag shadow for a {@link DrawableRecipientChip}.
     */
    private final class RecipientChipShadow extends DragShadowBuilder {
        private final DrawableRecipientChip mChip;

        public RecipientChipShadow(DrawableRecipientChip chip) {
            mChip = chip;
        }

        @Override
        public void onProvideShadowMetrics(@NonNull Point shadowSize,
                @NonNull Point shadowTouchPoint) {
            Rect rect = mChip.getBounds();
            shadowSize.set(rect.width(), rect.height());
            shadowTouchPoint.set(rect.centerX(), rect.centerY());
        }

        @Override
        public void onDrawShadow(@NonNull Canvas canvas) {
            mChip.draw(canvas);
        }
    }

    private void showCopyDialog(final String address) {
        final Context context = getContext();
        if (!mAttachedToWindow || context == null || !(context instanceof Activity)) {
            return;
        }

        final DialogFragment fragment = CopyDialog.newInstance(address);
        fragment.show(((Activity) context).getFragmentManager(), CopyDialog.TAG);
    }

    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        // Do nothing.
        return false;
    }

    @Override
    public void onShowPress(MotionEvent e) {
        // Do nothing.
    }

    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        // Do nothing.
        return false;
    }

    protected boolean isPhoneQuery() {
        return getAdapter() != null
                && getAdapter().getQueryType() == BaseRecipientAdapter.QUERY_TYPE_PHONE;
    }

    @Override
    public BaseRecipientAdapter getAdapter() {
        return (BaseRecipientAdapter) super.getAdapter();
    }

    /**
     * Append a new {@link RecipientEntry} to the end of the recipient chips, leaving any
     * unfinished text at the end.
     */
    public void appendRecipientEntry(final RecipientEntry entry) {
        clearComposingText();

        final Editable editable = getText();
        int chipInsertionPoint = 0;

        // Find the end of last chip and see if there's any unchipified text.
        final DrawableRecipientChip[] recips = getSortedRecipients();
        if (recips != null && recips.length > 0) {
            final DrawableRecipientChip last = recips[recips.length - 1];
            // The chip will be inserted at the end of last chip + 1. All the unfinished text after
            // the insertion point will be kept untouched.
            chipInsertionPoint = editable.getSpanEnd(last) + 1;
        }

        final CharSequence chip = createChip(entry);
        if (chip != null) {
            editable.insert(chipInsertionPoint, chip);
        }
    }

    /**
     * Remove all chips matching the given RecipientEntry.
     */
    public void removeRecipientEntry(final RecipientEntry entry) {
        final DrawableRecipientChip[] recips = getText()
                .getSpans(0, getText().length(), DrawableRecipientChip.class);

        for (final DrawableRecipientChip recipient : recips) {
            final RecipientEntry existingEntry = recipient.getEntry();
            if (existingEntry != null && existingEntry.isValid() &&
                    existingEntry.isSamePerson(entry)) {
                removeChip(recipient);
            }
        }
    }

    public void setAlternatePopupAnchor(View v) {
        mAlternatePopupAnchor = v;
    }

    @Override
    public void setVisibility(int visibility) {
        super.setVisibility(visibility);

        if (visibility != GONE && mRequiresShrinkWhenNotGone) {
            mRequiresShrinkWhenNotGone = false;
            mHandler.post(mDelayedShrink);
        }
    }

    private static class ChipBitmapContainer {
        Bitmap bitmap;
        // information used for positioning the loaded icon
        boolean loadIcon = true;
        float left;
        float top;
        float right;
        float bottom;
    }
}
