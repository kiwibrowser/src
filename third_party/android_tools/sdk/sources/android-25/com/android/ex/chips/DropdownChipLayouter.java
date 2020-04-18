package com.android.ex.chips;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.net.Uri;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.annotation.Nullable;
import android.support.v4.view.MarginLayoutParamsCompat;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.util.Rfc822Tokenizer;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.ex.chips.Queries.Query;

/**
 * A class that inflates and binds the views in the dropdown list from
 * RecipientEditTextView.
 */
public class DropdownChipLayouter {
    /**
     * The type of adapter that is requesting a chip layout.
     */
    public enum AdapterType {
        BASE_RECIPIENT,
        RECIPIENT_ALTERNATES,
        SINGLE_RECIPIENT
    }

    public interface ChipDeleteListener {
        void onChipDelete();
    }

    /**
     * Listener that handles the dismisses of the entries of the
     * {@link RecipientEntry#ENTRY_TYPE_PERMISSION_REQUEST} type.
     */
    public interface PermissionRequestDismissedListener {

        /**
         * Callback that occurs when user dismisses the item that asks user to grant permissions to
         * the app.
         */
        void onPermissionRequestDismissed();
    }

    private final LayoutInflater mInflater;
    private final Context mContext;
    private ChipDeleteListener mDeleteListener;
    private PermissionRequestDismissedListener mPermissionRequestDismissedListener;
    private Query mQuery;
    private int mAutocompleteDividerMarginStart;

    public DropdownChipLayouter(LayoutInflater inflater, Context context) {
        mInflater = inflater;
        mContext = context;
        mAutocompleteDividerMarginStart =
                context.getResources().getDimensionPixelOffset(R.dimen.chip_wrapper_start_padding);
    }

    public void setQuery(Query query) {
        mQuery = query;
    }

    public void setDeleteListener(ChipDeleteListener listener) {
        mDeleteListener = listener;
    }

    public void setPermissionRequestDismissedListener(PermissionRequestDismissedListener listener) {
        mPermissionRequestDismissedListener = listener;
    }

    public void setAutocompleteDividerMarginStart(int autocompleteDividerMarginStart) {
        mAutocompleteDividerMarginStart = autocompleteDividerMarginStart;
    }

    /**
     * Layouts and binds recipient information to the view. If convertView is null, inflates a new
     * view with getItemLaytout().
     *
     * @param convertView The view to bind information to.
     * @param parent The parent to bind the view to if we inflate a new view.
     * @param entry The recipient entry to get information from.
     * @param position The position in the list.
     * @param type The adapter type that is requesting the bind.
     * @param constraint The constraint typed in the auto complete view.
     *
     * @return A view ready to be shown in the drop down list.
     */
    public View bindView(View convertView, ViewGroup parent, RecipientEntry entry, int position,
        AdapterType type, String constraint) {
        return bindView(convertView, parent, entry, position, type, constraint, null);
    }

    /**
     * See {@link #bindView(View, ViewGroup, RecipientEntry, int, AdapterType, String)}
     * @param deleteDrawable a {@link android.graphics.drawable.StateListDrawable} representing
     *     the delete icon. android.R.attr.state_activated should map to the delete icon, and the
     *     default state can map to a drawable of your choice (or null for no drawable).
     */
    public View bindView(View convertView, ViewGroup parent, RecipientEntry entry, int position,
            AdapterType type, String constraint, StateListDrawable deleteDrawable) {
        // Default to show all the information
        CharSequence[] styledResults = getStyledResults(constraint, entry);
        CharSequence displayName = styledResults[0];
        CharSequence destination = styledResults[1];
        boolean showImage = true;
        CharSequence destinationType = getDestinationType(entry);

        final View itemView = reuseOrInflateView(convertView, parent, type);

        final ViewHolder viewHolder = new ViewHolder(itemView);

        // Hide some information depending on the adapter type.
        switch (type) {
            case BASE_RECIPIENT:
                if (TextUtils.isEmpty(displayName) || TextUtils.equals(displayName, destination)) {
                    displayName = destination;

                    // We only show the destination for secondary entries, so clear it only for the
                    // first level.
                    if (entry.isFirstLevel()) {
                        destination = null;
                    }
                }

                if (!entry.isFirstLevel()) {
                    displayName = null;
                    showImage = false;
                }

                // For BASE_RECIPIENT set all top dividers except for the first one to be GONE.
                if (viewHolder.topDivider != null) {
                    viewHolder.topDivider.setVisibility(position == 0 ? View.VISIBLE : View.GONE);
                    MarginLayoutParamsCompat.setMarginStart(
                            (MarginLayoutParams) viewHolder.topDivider.getLayoutParams(),
                            mAutocompleteDividerMarginStart);
                }
                if (viewHolder.bottomDivider != null) {
                    MarginLayoutParamsCompat.setMarginStart(
                            (MarginLayoutParams) viewHolder.bottomDivider.getLayoutParams(),
                            mAutocompleteDividerMarginStart);
                }
                break;
            case RECIPIENT_ALTERNATES:
                if (position != 0) {
                    displayName = null;
                    showImage = false;
                }
                break;
            case SINGLE_RECIPIENT:
                if (!PhoneUtil.isPhoneNumber(entry.getDestination())) {
                    destination = Rfc822Tokenizer.tokenize(entry.getDestination())[0].getAddress();
                }
                destinationType = null;
        }

        // Bind the information to the view
        bindTextToView(displayName, viewHolder.displayNameView);
        bindTextToView(destination, viewHolder.destinationView);
        bindTextToView(destinationType, viewHolder.destinationTypeView);
        bindIconToView(showImage, entry, viewHolder.imageView, type);
        bindDrawableToDeleteView(deleteDrawable, entry.getDisplayName(), viewHolder.deleteView);
        bindIndicatorToView(
                entry.getIndicatorIconId(), entry.getIndicatorText(), viewHolder.indicatorView);
        bindPermissionRequestDismissView(viewHolder.permissionRequestDismissView);

        // Hide some view groups depending on the entry type
        final int entryType = entry.getEntryType();
        if (entryType == RecipientEntry.ENTRY_TYPE_PERSON) {
            setViewVisibility(viewHolder.personViewGroup, View.VISIBLE);
            setViewVisibility(viewHolder.permissionViewGroup, View.GONE);
            setViewVisibility(viewHolder.permissionBottomDivider, View.GONE);
        } else if (entryType == RecipientEntry.ENTRY_TYPE_PERMISSION_REQUEST) {
            setViewVisibility(viewHolder.personViewGroup, View.GONE);
            setViewVisibility(viewHolder.permissionViewGroup, View.VISIBLE);
            setViewVisibility(viewHolder.permissionBottomDivider, View.VISIBLE);
        }

        return itemView;
    }

    /**
     * Returns a new view with {@link #getItemLayoutResId(AdapterType)}.
     */
    public View newView(AdapterType type) {
        return mInflater.inflate(getItemLayoutResId(type), null);
    }

    /**
     * Returns the same view, or inflates a new one if the given view was null.
     */
    protected View reuseOrInflateView(View convertView, ViewGroup parent, AdapterType type) {
        int itemLayout = getItemLayoutResId(type);
        switch (type) {
            case BASE_RECIPIENT:
            case RECIPIENT_ALTERNATES:
                break;
            case SINGLE_RECIPIENT:
                itemLayout = getAlternateItemLayoutResId(type);
                break;
        }
        return convertView != null ? convertView : mInflater.inflate(itemLayout, parent, false);
    }

    /**
     * Binds the text to the given text view. If the text was null, hides the text view.
     */
    protected void bindTextToView(CharSequence text, TextView view) {
        if (view == null) {
            return;
        }

        if (text != null) {
            view.setText(text);
            view.setVisibility(View.VISIBLE);
        } else {
            view.setVisibility(View.GONE);
        }
    }

    /**
     * Binds the avatar icon to the image view. If we don't want to show the image, hides the
     * image view.
     */
    protected void bindIconToView(boolean showImage, RecipientEntry entry, ImageView view,
        AdapterType type) {
        if (view == null) {
            return;
        }

        if (showImage) {
            switch (type) {
                case BASE_RECIPIENT:
                    byte[] photoBytes = entry.getPhotoBytes();
                    if (photoBytes != null && photoBytes.length > 0) {
                        final Bitmap photo = BitmapFactory.decodeByteArray(photoBytes, 0,
                            photoBytes.length);
                        view.setImageBitmap(photo);
                    } else {
                        view.setImageResource(getDefaultPhotoResId());
                    }
                    break;
                case RECIPIENT_ALTERNATES:
                    Uri thumbnailUri = entry.getPhotoThumbnailUri();
                    if (thumbnailUri != null) {
                        // TODO: see if this needs to be done outside the main thread
                        // as it may be too slow to get immediately.
                        view.setImageURI(thumbnailUri);
                    } else {
                        view.setImageResource(getDefaultPhotoResId());
                    }
                    break;
                case SINGLE_RECIPIENT:
                default:
                    break;
            }
            view.setVisibility(View.VISIBLE);
        } else {
            view.setVisibility(View.GONE);
        }
    }

    protected void bindDrawableToDeleteView(final StateListDrawable drawable, String recipient,
            ImageView view) {
        if (view == null) {
            return;
        }
        if (drawable == null) {
            view.setVisibility(View.GONE);
        } else {
            final Resources res = mContext.getResources();
            view.setImageDrawable(drawable);
            view.setContentDescription(
                    res.getString(R.string.dropdown_delete_button_desc, recipient));
            if (mDeleteListener != null) {
                view.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (drawable.getCurrent() != null) {
                            mDeleteListener.onChipDelete();
                        }
                    }
                });
            }
        }
    }

    protected void bindIndicatorToView(
            @DrawableRes int indicatorIconId, String indicatorText, TextView view) {
        if (view != null) {
            if (indicatorText != null || indicatorIconId != 0) {
                view.setText(indicatorText);
                view.setVisibility(View.VISIBLE);
                final Drawable indicatorIcon;
                if (indicatorIconId != 0) {
                    indicatorIcon = mContext.getDrawable(indicatorIconId).mutate();
                    indicatorIcon.setColorFilter(Color.WHITE, PorterDuff.Mode.SRC_IN);
                } else {
                    indicatorIcon = null;
                }
                view.setCompoundDrawablesRelativeWithIntrinsicBounds(
                        indicatorIcon, null, null, null);
            } else {
                view.setVisibility(View.GONE);
            }
        }
    }

    protected void bindPermissionRequestDismissView(ImageView view) {
        if (view == null) {
            return;
        }
        view.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mPermissionRequestDismissedListener != null) {
                    mPermissionRequestDismissedListener.onPermissionRequestDismissed();
                }
            }
        });
    }

    protected void setViewVisibility(View view, int visibility) {
        if (view != null) {
            view.setVisibility(visibility);
        }
    }

    protected CharSequence getDestinationType(RecipientEntry entry) {
        return mQuery.getTypeLabel(mContext.getResources(), entry.getDestinationType(),
            entry.getDestinationLabel()).toString().toUpperCase();
    }

    /**
     * Returns a layout id for each item inside auto-complete list.
     *
     * Each View must contain two TextViews (for display name and destination) and one ImageView
     * (for photo). Ids for those should be available via {@link #getDisplayNameResId()},
     * {@link #getDestinationResId()}, and {@link #getPhotoResId()}.
     */
    protected @LayoutRes int getItemLayoutResId(AdapterType type) {
        switch (type) {
            case BASE_RECIPIENT:
                return R.layout.chips_autocomplete_recipient_dropdown_item;
            case RECIPIENT_ALTERNATES:
                return R.layout.chips_recipient_dropdown_item;
            default:
                return R.layout.chips_recipient_dropdown_item;
        }
    }

    /**
     * Returns a layout id for each item inside alternate auto-complete list.
     *
     * Each View must contain two TextViews (for display name and destination) and one ImageView
     * (for photo). Ids for those should be available via {@link #getDisplayNameResId()},
     * {@link #getDestinationResId()}, and {@link #getPhotoResId()}.
     */
    protected @LayoutRes int getAlternateItemLayoutResId(AdapterType type) {
        switch (type) {
            case BASE_RECIPIENT:
                return R.layout.chips_autocomplete_recipient_dropdown_item;
            case RECIPIENT_ALTERNATES:
                return R.layout.chips_recipient_dropdown_item;
            default:
                return R.layout.chips_recipient_dropdown_item;
        }
    }

    /**
     * Returns a resource ID representing an image which should be shown when ther's no relevant
     * photo is available.
     */
    protected @DrawableRes int getDefaultPhotoResId() {
        return R.drawable.ic_contact_picture;
    }

    /**
     * Returns an id for the ViewGroup in an item View that contains the person ui elements.
     */
    protected @IdRes int getPersonGroupResId() {
        return R.id.chip_person_wrapper;
    }

    /**
     * Returns an id for TextView in an item View for showing a display name. By default
     * {@link android.R.id#title} is returned.
     */
    protected @IdRes int getDisplayNameResId() {
        return android.R.id.title;
    }

    /**
     * Returns an id for TextView in an item View for showing a destination
     * (an email address or a phone number).
     * By default {@link android.R.id#text1} is returned.
     */
    protected @IdRes int getDestinationResId() {
        return android.R.id.text1;
    }

    /**
     * Returns an id for TextView in an item View for showing the type of the destination.
     * By default {@link android.R.id#text2} is returned.
     */
    protected @IdRes int getDestinationTypeResId() {
        return android.R.id.text2;
    }

    /**
     * Returns an id for ImageView in an item View for showing photo image for a person. In default
     * {@link android.R.id#icon} is returned.
     */
    protected @IdRes int getPhotoResId() {
        return android.R.id.icon;
    }

    /**
     * Returns an id for ImageView in an item View for showing the delete button. In default
     * {@link android.R.id#icon1} is returned.
     */
    protected @IdRes int getDeleteResId() { return android.R.id.icon1; }

    /**
     * Returns an id for the ViewGroup in an item View that contains the request permission ui
     * elements.
     */
    protected @IdRes int getPermissionGroupResId() {
        return R.id.chip_permission_wrapper;
    }

    /**
     * Returns an id for ImageView in an item View for dismissing the permission request. In default
     * {@link android.R.id#icon2} is returned.
     */
    protected @IdRes int getPermissionRequestDismissResId() {
        return android.R.id.icon2;
    }

    /**
     * Given a constraint and a recipient entry, tries to find the constraint in the name and
     * destination in the recipient entry. A foreground font color style will be applied to the
     * section that matches the constraint. As soon as a match has been found, no further matches
     * are attempted.
     *
     * @param constraint A string that we will attempt to find within the results.
     * @param entry The recipient entry to style results for.
     *
     * @return An array of CharSequences, the length determined by the length of results. Each
     *     CharSequence will either be a styled SpannableString or just the input String.
     */
    protected CharSequence[] getStyledResults(@Nullable String constraint, RecipientEntry entry) {
      return getStyledResults(constraint, entry.getDisplayName(), entry.getDestination());
    }

    /**
     * Given a constraint and results, tries to find the constraint in those results, one at a time.
     * A foreground font color style will be applied to the section that matches the constraint. As
     * soon as a match has been found, no further matches are attempted.
     *
     * @param constraint A string that we will attempt to find within the results.
     * @param results Strings that may contain the constraint. The order given is the order used to
     *     search for the constraint.
     *
     * @return An array of CharSequences, the length determined by the length of results. Each
     *     CharSequence will either be a styled SpannableString or just the input String.
     */
    protected CharSequence[] getStyledResults(@Nullable String constraint, String... results) {
        if (isAllWhitespace(constraint)) {
            return results;
        }

        CharSequence[] styledResults = new CharSequence[results.length];
        boolean foundMatch = false;
        for (int i = 0; i < results.length; i++) {
            String result = results[i];
            if (result == null) {
                continue;
            }

            if (!foundMatch) {
                int index = result.toLowerCase().indexOf(constraint.toLowerCase());
                if (index != -1) {
                    SpannableStringBuilder styled = SpannableStringBuilder.valueOf(result);
                    ForegroundColorSpan highlightSpan =
                            new ForegroundColorSpan(mContext.getResources().getColor(
                                    R.color.chips_dropdown_text_highlighted));
                    styled.setSpan(highlightSpan,
                            index, index + constraint.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    styledResults[i] = styled;
                    foundMatch = true;
                    continue;
                }
            }
            styledResults[i] = result;
        }
        return styledResults;
    }

    private static boolean isAllWhitespace(@Nullable String string) {
        if (TextUtils.isEmpty(string)) {
            return true;
        }

        for (int i = 0; i < string.length(); ++i) {
            if (!Character.isWhitespace(string.charAt(i))) {
                return false;
            }
        }

        return true;
    }

    /**
     * A holder class the view. Uses the getters in DropdownChipLayouter to find the id of the
     * corresponding views.
     */
    protected class ViewHolder {
        public final ViewGroup personViewGroup;
        public final TextView displayNameView;
        public final TextView destinationView;
        public final TextView destinationTypeView;
        public final TextView indicatorView;
        public final ImageView imageView;
        public final ImageView deleteView;
        public final View topDivider;
        public final View bottomDivider;
        public final View permissionBottomDivider;

        public final ViewGroup permissionViewGroup;
        public final ImageView permissionRequestDismissView;

        public ViewHolder(View view) {
            personViewGroup = (ViewGroup) view.findViewById(getPersonGroupResId());
            displayNameView = (TextView) view.findViewById(getDisplayNameResId());
            destinationView = (TextView) view.findViewById(getDestinationResId());
            destinationTypeView = (TextView) view.findViewById(getDestinationTypeResId());
            imageView = (ImageView) view.findViewById(getPhotoResId());
            deleteView = (ImageView) view.findViewById(getDeleteResId());
            topDivider = view.findViewById(R.id.chip_autocomplete_top_divider);

            bottomDivider = view.findViewById(R.id.chip_autocomplete_bottom_divider);
            permissionBottomDivider = view.findViewById(R.id.chip_permission_bottom_divider);

            indicatorView = (TextView) view.findViewById(R.id.chip_indicator_text);

            permissionViewGroup = (ViewGroup) view.findViewById(getPermissionGroupResId());
            permissionRequestDismissView =
                    (ImageView) view.findViewById(getPermissionRequestDismissResId());
        }
    }
}
