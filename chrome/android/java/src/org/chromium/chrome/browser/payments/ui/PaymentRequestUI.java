// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments.ui;

import static org.chromium.chrome.browser.payments.ui.PaymentRequestSection.EDIT_BUTTON_GONE;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Handler;
import android.support.annotation.IntDef;
import android.support.v4.view.animation.FastOutLinearInInterpolator;
import android.support.v4.view.animation.LinearOutSlowInInterpolator;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnLayoutChangeListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callback;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.chrome.browser.payments.ShippingStrings;
import org.chromium.chrome.browser.payments.ui.PaymentRequestSection.LineItemBreakdownSection;
import org.chromium.chrome.browser.payments.ui.PaymentRequestSection.OptionSection;
import org.chromium.chrome.browser.payments.ui.PaymentRequestSection.SectionSeparator;
import org.chromium.chrome.browser.widget.AlwaysDismissedDialog;
import org.chromium.chrome.browser.widget.FadingEdgeScrollView;
import org.chromium.chrome.browser.widget.animation.AnimatorProperties;
import org.chromium.chrome.browser.widget.animation.FocusAnimator;
import org.chromium.chrome.browser.widget.prefeditor.EditableOption;
import org.chromium.chrome.browser.widget.prefeditor.EditorDialog;
import org.chromium.chrome.browser.widget.prefeditor.EditorObserverForTest;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;
import org.chromium.ui.widget.TextViewWithClickableSpans;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

/**
 * The PaymentRequest UI.
 */
public class PaymentRequestUI implements DialogInterface.OnDismissListener, View.OnClickListener,
        PaymentRequestSection.SectionDelegate {
    public static final int TYPE_SHIPPING_ADDRESSES = 1;
    public static final int TYPE_SHIPPING_OPTIONS = 2;
    public static final int TYPE_CONTACT_DETAILS = 3;
    public static final int TYPE_PAYMENT_METHODS = 4;

    public static final int SELECTION_RESULT_ASYNCHRONOUS_VALIDATION = 1;
    public static final int SELECTION_RESULT_EDITOR_LAUNCH = 2;
    public static final int SELECTION_RESULT_NONE = 3;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        TYPE_SHIPPING_ADDRESSES,
        TYPE_SHIPPING_OPTIONS,
        TYPE_CONTACT_DETAILS,
        TYPE_PAYMENT_METHODS
    })
    public @interface DataType {}

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            SELECTION_RESULT_ASYNCHRONOUS_VALIDATION,
            SELECTION_RESULT_EDITOR_LAUNCH,
            SELECTION_RESULT_NONE,
    })
    public @interface SelectionResult {}

    /**
     * The interface to be implemented by the consumer of the PaymentRequest UI.
     */
    public interface Client {
        /**
         * Asynchronously returns the default payment information.
         */
        void getDefaultPaymentInformation(Callback<PaymentInformation> callback);

        /**
         * Asynchronously returns the full bill. Includes the total price and its breakdown into
         * individual line items.
         */
        void getShoppingCart(Callback<ShoppingCart> callback);

        /**
         * Asynchronously returns the full list of options for the given type.
         *
         * @param optionType Data being updated.
         * @param callback   Callback to run when the data has been fetched.
         */
        void getSectionInformation(
                @DataType int optionType, Callback<SectionInformation> callback);

        /**
         * Called when the user changes one of their payment options.
         *
         * If this method returns {@link SELECTION_RESULT_ASYNCHRONOUS_VALIDATION}, then:
         * + The added option should be asynchronously verified.
         * + The section should be disabled and a progress spinny should be shown while the option
         *   is being verified.
         * + The checkedCallback will be invoked with the results of the check and updated
         *   information.
         *
         * If this method returns {@link SELECTION_RESULT_EDITOR_LAUNCH}, then:
         * + Interaction with UI should be disabled until updateSection() is called.
         *
         * For example, if the website needs a shipping address to calculate shipping options, then
         * calling onSectionOptionSelected(TYPE_SHIPPING_ADDRESS, option, checkedCallback) will
         * return true. When the website updates the shipping options, the checkedCallback will be
         * invoked.
         *
         * @param optionType        Data being updated.
         * @param option            Value of the data being updated.
         * @param checkedCallback   The callback after an asynchronous check has completed.
         * @return The result of the selection.
         */
        @SelectionResult
        int onSectionOptionSelected(@DataType int optionType, EditableOption option,
                Callback<PaymentInformation> checkedCallback);

        /**
         * Called when the user clicks edit icon (pencil icon) on the payment option in a section.
         *
         * If this method returns {@link SELECTION_RESULT_ASYNCHRONOUS_VALIDATION}, then:
         * + The edited option should be asynchronously verified.
         * + The section should be disabled and a progress spinny should be shown while the option
         *   is being verified.
         * + The checkedCallback will be invoked with the results of the check and updated
         *   information.
         *
         * If this method returns {@link SELECTION_RESULT_EDITOR_LAUNCH}, then:
         * + Interaction with UI should be disabled until updateSection() is called.
         *
         * @param optionType      Data being updated.
         * @param option          The option to be edited.
         * @param checkedCallback The callback after an asynchronous check has completed.
         * @return The result of the edit request.
         */
        @SelectionResult
        int onSectionEditOption(@DataType int optionType, EditableOption option,
                Callback<PaymentInformation> checkedCallback);

        /**
         * Called when the user clicks on the "Add" button for a section.
         *
         * If this method returns {@link SELECTION_RESULT_ASYNCHRONOUS_VALIDATION}, then:
         * + The added option should be asynchronously verified.
         * + The section should be disabled and a progress spinny should be shown while the option
         *   is being verified.
         * + The checkedCallback will be invoked with the results of the check and updated
         *   information.
         *
         * If this method returns {@link SELECTION_RESULT_EDITOR_LAUNCH}, then:
         * + Interaction with UI should be disabled until updateSection() is called.
         *
         * @param optionType      Data being updated.
         * @param checkedCallback The callback after an asynchronous check has completed.
         * @return The result of the selection.
         */
        @SelectionResult int onSectionAddOption(
                @DataType int optionType, Callback<PaymentInformation> checkedCallback);

        /**
         * Called when the user clicks on the “Pay” button. If this method returns true, the UI is
         * disabled and is showing a spinner. Otherwise, the UI is hidden.
         */
        boolean onPayClicked(EditableOption selectedShippingAddress,
                EditableOption selectedShippingOption, EditableOption selectedPaymentMethod);

        /**
         * Called when the user dismisses the UI via the “back” button on their phone
         * or the “X” button in UI.
         */
        void onDismiss();

        /**
         * Called when the user clicks on 'Settings' to control card and address options.
         */
        void onCardAndAddressSettingsClicked();
    }

    /**
     * A test-only observer for PaymentRequest UI.
     */
    public interface PaymentRequestObserverForTest {
        /**
         * Called when clicks on the UI are possible.
         */
        void onPaymentRequestReadyForInput(PaymentRequestUI ui);

        /**
         * Called when clicks on the PAY button are possible.
         */
        void onPaymentRequestReadyToPay(PaymentRequestUI ui);

        /**
         * Called when the UI has been updated to reflect checking a selected option.
         */
        void onPaymentRequestSelectionChecked(PaymentRequestUI ui);

        /**
         * Called when the result UI is showing.
         */
        void onPaymentRequestResultReady(PaymentRequestUI ui);
    }

    /** Helper to notify tests of an event only once. */
    private static class NotifierForTest {
        private final Handler mHandler;
        private final Runnable mNotification;
        private boolean mNotificationPending;

        /**
         * Constructs the helper to notify tests for an event.
         *
         * @param notification The callback that notifies the test of an event.
         */
        public NotifierForTest(final Runnable notification) {
            mHandler = new Handler();
            mNotification = new Runnable() {
                @Override
                public void run() {
                    notification.run();
                    mNotificationPending = false;
                }
            };
        }

        /** Schedules a single notification for test, even if called only once. */
        public void run() {
            if (mNotificationPending) return;
            mNotificationPending = true;
            mHandler.post(mNotification);
        }
    }

    /**
     * Length of the animation to either show the UI or expand it to full height.
     * Note that click of 'Pay' button is not accepted until the animation is done, so this duration
     * also serves the function of preventing the user from accidently double-clicking on the screen
     * when triggering payment and thus authorizing unwanted transaction.
     */
    private static final int DIALOG_ENTER_ANIMATION_MS = 225;

    /** Length of the animation to hide the bottom sheet UI. */
    private static final int DIALOG_EXIT_ANIMATION_MS = 195;

    private static PaymentRequestObserverForTest sPaymentRequestObserverForTest;
    private static EditorObserverForTest sEditorObserverForTest;

    /** Notifies tests that the [PAY] button can be clicked. */
    private final NotifierForTest mReadyToPayNotifierForTest;

    private final Context mContext;
    private final Client mClient;
    private final boolean mRequestShipping;
    private final boolean mRequestContactDetails;
    private final boolean mShowDataSource;

    private final Dialog mDialog;
    private final EditorDialog mEditorDialog;
    private final EditorDialog mCardEditorDialog;
    private final ViewGroup mFullContainer;
    private final ViewGroup mRequestView;
    private final PaymentRequestUiErrorView mErrorView;
    private final Callback<PaymentInformation> mUpdateSectionsCallback;
    private final ShippingStrings mShippingStrings;

    private FadingEdgeScrollView mPaymentContainer;
    private LinearLayout mPaymentContainerLayout;
    private ViewGroup mBottomBar;
    private Button mEditButton;
    private Button mPayButton;
    private View mCloseButton;
    private View mSpinnyLayout;

    private LineItemBreakdownSection mOrderSummarySection;
    private OptionSection mShippingAddressSection;
    private OptionSection mShippingOptionSection;
    private OptionSection mContactDetailsSection;
    private OptionSection mPaymentMethodSection;
    private List<SectionSeparator> mSectionSeparators;

    private PaymentRequestSection mSelectedSection;
    private boolean mIsExpandedToFullHeight;
    private boolean mIsProcessingPayClicked;
    private boolean mIsClientClosing;
    private boolean mIsClientCheckingSelection;
    private boolean mIsShowingSpinner;
    private boolean mIsEditingPaymentItem;
    private boolean mIsClosing;

    private SectionInformation mPaymentMethodSectionInformation;
    private SectionInformation mShippingAddressSectionInformation;
    private SectionInformation mShippingOptionsSectionInformation;
    private SectionInformation mContactDetailsSectionInformation;

    private Animator mSheetAnimator;
    private FocusAnimator mSectionAnimator;
    private int mAnimatorTranslation;

    /**
     * Builds the UI for PaymentRequest.
     *
     * @param activity        The activity on top of which the UI should be displayed.
     * @param client          The consumer of the PaymentRequest UI.
     * @param requestShipping Whether the UI should show the shipping address and option selection.
     * @param requestContact  Whether the UI should show the payer name, email address and
     *                        phone number selection.
     * @param canAddCards     Whether the UI should show the [+ADD CARD] button. This can be false,
     *                        for example, when the merchant does not accept credit cards, so
     *                        there's no point in adding cards within PaymentRequest UI.
     * @param showDataSource  Whether the UI should describe the source of Autofill data.
     * @param title           The title to show at the top of the UI. This can be, for example, the
     *                        &lt;title&gt; of the merchant website. If the string is too long for
     *                        UI, it elides at the end.
     * @param origin          The origin (https://tools.ietf.org/html/rfc6454) to show under the
     *                        title. For example, "https://shop.momandpop.com". If the origin is too
     *                        long for the UI, it should elide according to:
     * https://www.chromium.org/Home/chromium-security/enamel#TOC-Eliding-Origin-Names-And-Hostnames
     * @param securityLevel   The security level of the page that invoked PaymentRequest.
     * @param shippingStrings The string resource identifiers to use in the shipping sections.
     */
    public PaymentRequestUI(Activity activity, Client client, boolean requestShipping,
            boolean requestContact, boolean canAddCards, boolean showDataSource, String title,
            String origin, int securityLevel, ShippingStrings shippingStrings) {
        mContext = activity;
        mClient = client;
        mRequestShipping = requestShipping;
        mRequestContactDetails = requestContact;
        mShowDataSource = showDataSource;
        mAnimatorTranslation = mContext.getResources().getDimensionPixelSize(
                R.dimen.payments_ui_translation);

        mErrorView = (PaymentRequestUiErrorView) LayoutInflater.from(mContext).inflate(
                R.layout.payment_request_error, null);
        mErrorView.initialize(title, origin, securityLevel);

        mReadyToPayNotifierForTest = new NotifierForTest(new Runnable() {
            @Override
            public void run() {
                if (sPaymentRequestObserverForTest != null && isAcceptingUserInput()
                        && mPayButton.isEnabled()) {
                    sPaymentRequestObserverForTest.onPaymentRequestReadyToPay(
                            PaymentRequestUI.this);
                }
            }
        });

        // This callback will be fired if mIsClientCheckingSelection is true.
        mUpdateSectionsCallback = new Callback<PaymentInformation>() {
            @Override
            public void onResult(PaymentInformation result) {
                mIsClientCheckingSelection = false;
                updateOrderSummarySection(result.getShoppingCart());
                if (mRequestShipping) {
                    updateSection(TYPE_SHIPPING_ADDRESSES, result.getShippingAddresses());
                    updateSection(TYPE_SHIPPING_OPTIONS, result.getShippingOptions());
                }
                if (mRequestContactDetails) {
                    updateSection(TYPE_CONTACT_DETAILS, result.getContactDetails());
                }
                updateSection(TYPE_PAYMENT_METHODS, result.getPaymentMethods());
                if (mShippingAddressSectionInformation.getSelectedItem() == null) {
                    expand(mShippingAddressSection);
                } else {
                    expand(null);
                }
                updatePayButtonEnabled();
                notifySelectionChecked();
            }
        };

        mShippingStrings = shippingStrings;

        mRequestView =
                (ViewGroup) LayoutInflater.from(mContext).inflate(R.layout.payment_request, null);
        prepareRequestView(mContext, title, origin, securityLevel, canAddCards);

        // To handle the specced animations, the dialog is entirely contained within a translucent
        // FrameLayout.  This could eventually be converted to a real BottomSheetDialog, but that
        // requires exploration of how interactions would work when the dialog can be sent back and
        // forth between the peeking and expanded state.
        mFullContainer = new FrameLayout(mContext);
        mFullContainer.setBackgroundColor(ApiCompatibilityUtils.getColor(
                mContext.getResources(), R.color.modal_dialog_scrim_color));
        FrameLayout.LayoutParams bottomSheetParams = new FrameLayout.LayoutParams(
                LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        bottomSheetParams.gravity = Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM;
        mFullContainer.addView(mRequestView, bottomSheetParams);

        mEditorDialog = new EditorDialog(activity, sEditorObserverForTest,
                /*deleteRunnable =*/null);
        mCardEditorDialog = new EditorDialog(activity, sEditorObserverForTest,
                /*deleteRunnable =*/null);

        // Allow screenshots of the credit card number in Canary, Dev, and developer builds.
        if (ChromeVersionInfo.isBetaBuild() || ChromeVersionInfo.isStableBuild()) {
            mCardEditorDialog.disableScreenshots();
        }

        // Set up the dialog.
        mDialog = new AlwaysDismissedDialog(activity, R.style.DialogWhenLarge);
        mDialog.setOnDismissListener(this);
        mDialog.addContentView(mFullContainer,
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

        Window dialogWindow = mDialog.getWindow();
        dialogWindow.setGravity(Gravity.CENTER);
        dialogWindow.setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        dialogWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
    }

    /**
     * Shows the PaymentRequest UI.
     */
    public void show() {
        mDialog.show();
        mClient.getDefaultPaymentInformation(new Callback<PaymentInformation>() {
            @Override
            public void onResult(PaymentInformation result) {
                updateOrderSummarySection(result.getShoppingCart());

                if (mRequestShipping) {
                    updateSection(TYPE_SHIPPING_ADDRESSES, result.getShippingAddresses());
                    updateSection(TYPE_SHIPPING_OPTIONS, result.getShippingOptions());

                    // Let the summary display a CHOOSE/ADD button for the first subsection that
                    // needs it.
                    PaymentRequestSection section =
                            mShippingAddressSection.getEditButtonState() == EDIT_BUTTON_GONE
                            ? mShippingOptionSection
                            : mShippingAddressSection;
                }

                if (mRequestContactDetails) {
                    updateSection(TYPE_CONTACT_DETAILS, result.getContactDetails());
                }

                mPaymentMethodSection.setDisplaySummaryInSingleLineInNormalMode(
                        result.getPaymentMethods()
                                .getDisplaySelectedItemSummaryInSingleLineInNormalMode());
                updateSection(TYPE_PAYMENT_METHODS, result.getPaymentMethods());
                updatePayButtonEnabled();

                // Hide the loading indicators and show the real sections.
                changeSpinnerVisibility(false);
                mRequestView.addOnLayoutChangeListener(new SheetEnlargingAnimator(false));
            }
        });
    }

    /**
     * Prepares the PaymentRequestUI for initial display.
     *
     * TODO(dfalcantara): Ideally, everything related to the request and its views would just be put
     *                    into its own class but that'll require yanking out a lot of this class.
     *
     * @param context       The application context.
     * @param title         Title of the page.
     * @param origin        The RFC6454 origin of the page.
     * @param securityLevel The security level of the page that invoked PaymentRequest.
     * @param canAddCards   Whether new cards can be added.
     */
    private void prepareRequestView(
            Context context, String title, String origin, int securityLevel, boolean canAddCards) {
        mSpinnyLayout = mRequestView.findViewById(R.id.payment_request_spinny);
        assert mSpinnyLayout.getVisibility() == View.VISIBLE;
        mIsShowingSpinner = true;

        // Indicate that we're preparing the dialog for display.
        TextView messageView = (TextView) mRequestView.findViewById(R.id.message);
        messageView.setText(R.string.payments_loading_message);

        ((PaymentRequestHeader) mRequestView.findViewById(R.id.header))
                .setTitleAndOrigin(title, origin, securityLevel);

        // Set up the buttons.
        mCloseButton = mRequestView.findViewById(R.id.close_button);
        mCloseButton.setOnClickListener(this);
        mBottomBar = (ViewGroup) mRequestView.findViewById(R.id.bottom_bar);
        mPayButton = (Button) mBottomBar.findViewById(R.id.button_primary);
        mPayButton.setOnClickListener(this);
        mEditButton = (Button) mBottomBar.findViewById(R.id.button_secondary);
        mEditButton.setOnClickListener(this);

        // Create all the possible sections.
        mSectionSeparators = new ArrayList<>();
        mPaymentContainer = (FadingEdgeScrollView) mRequestView.findViewById(R.id.option_container);
        mPaymentContainerLayout =
                (LinearLayout) mRequestView.findViewById(R.id.payment_container_layout);
        mOrderSummarySection = new LineItemBreakdownSection(context,
                context.getString(R.string.payments_order_summary_label), this,
                context.getString(R.string.payments_updated_label));
        mShippingAddressSection = new OptionSection(
                context, context.getString(mShippingStrings.getAddressLabel()), this);
        mShippingOptionSection = new OptionSection(
                context, context.getString(mShippingStrings.getOptionLabel()), this);
        mContactDetailsSection = new OptionSection(
                context, context.getString(R.string.payments_contact_details_label), this);
        mPaymentMethodSection = new OptionSection(
                context, context.getString(R.string.payments_method_of_payment_label), this);

        // Display the summary of the selected address in multiple lines on bottom sheet.
        mShippingAddressSection.setDisplaySummaryInSingleLineInNormalMode(false);

        // Display selected shipping option name in the left summary text view and
        // the cost in the right summary text view on bottom sheet.
        mShippingOptionSection.setSplitSummaryInDisplayModeNormal(true);

        // Some sections conditionally allow adding new options.
        mShippingOptionSection.setCanAddItems(false);
        mPaymentMethodSection.setCanAddItems(canAddCards);

        // Put payment method section on top of address section for
        // WEB_PAYMENTS_METHOD_SECTION_ORDER_V2.
        boolean methodSectionOrderV2 =
                ChromeFeatureList.isEnabled(ChromeFeatureList.WEB_PAYMENTS_METHOD_SECTION_ORDER_V2);

        // Add the necessary sections to the layout.
        mPaymentContainerLayout.addView(mOrderSummarySection, new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        if (methodSectionOrderV2) {
            mSectionSeparators.add(new SectionSeparator(mPaymentContainerLayout));
            mPaymentContainerLayout.addView(mPaymentMethodSection,
                    new LinearLayout.LayoutParams(
                            LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }
        if (mRequestShipping) {
            mSectionSeparators.add(new SectionSeparator(mPaymentContainerLayout));
            // The shipping breakout sections are only added if they are needed.
            mPaymentContainerLayout.addView(mShippingAddressSection,
                    new LinearLayout.LayoutParams(
                            LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }
        if (!methodSectionOrderV2) {
            mSectionSeparators.add(new SectionSeparator(mPaymentContainerLayout));
            mPaymentContainerLayout.addView(mPaymentMethodSection,
                    new LinearLayout.LayoutParams(
                            LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }
        if (mRequestContactDetails) {
            // Contact details are optional, depending on the merchant website.
            mSectionSeparators.add(new SectionSeparator(mPaymentContainerLayout));
            mPaymentContainerLayout.addView(mContactDetailsSection, new LinearLayout.LayoutParams(
                    LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }

        mRequestView.addOnLayoutChangeListener(new FadeInAnimator());
        mRequestView.addOnLayoutChangeListener(new PeekingAnimator());

        // Enabled in updatePayButtonEnabled() when the user has selected all payment options.
        mPayButton.setEnabled(false);
    }

    /**
     * Closes the UI. Can be invoked in response to, for example:
     * <ul>
     *  <li>Successfully processing the payment.</li>
     *  <li>Failure to process the payment.</li>
     *  <li>The JavaScript calling the abort() method in PaymentRequest API.</li>
     *  <li>The PaymentRequest JavaScript object being destroyed.</li>
     * </ul>
     *
     * Does not call Client.onDismissed().
     *
     * Should not be called multiple times.
     *
     * @param shouldCloseImmediately If true, this function will immediately dismiss the dialog
     *        without describing the error.
     * @param callback The callback to notify of finished animations.
     */
    public void close(boolean shouldCloseImmediately, final Runnable callback) {
        mIsClientClosing = true;

        Runnable dismissRunnable = new Runnable() {
            @Override
            public void run() {
                dismissDialog(false);
                if (callback != null) callback.run();
            }
        };

        if (shouldCloseImmediately) {
            // The shouldCloseImmediately boolean is true when the merchant calls
            // instrumentResponse.complete("success") or instrumentResponse.complete("")
            // in JavaScript.
            dismissRunnable.run();
        } else {
            // Animate the bottom sheet going away.
            new DisappearingAnimator(false);

            // Show the error dialog.
            mErrorView.show(mFullContainer, dismissRunnable);
        }

        if (sPaymentRequestObserverForTest != null) {
            sPaymentRequestObserverForTest.onPaymentRequestResultReady(this);
        }
    }

    /**
     * Sets the icon in the top left of the UI. This can be, for example, the favicon of the
     * merchant website. This is not a part of the constructor because favicon retrieval is
     * asynchronous.
     *
     * @param bitmap The bitmap to show next to the title.
     */
    public void setTitleBitmap(Bitmap bitmap) {
        ((PaymentRequestHeader) mRequestView.findViewById(R.id.header)).setTitleBitmap(bitmap);
        mErrorView.setBitmap(bitmap);
    }

    /**
     * Updates the line items in response to a changed shipping address or option.
     *
     * @param cart The shopping cart, including the line items and the total.
     */
    public void updateOrderSummarySection(ShoppingCart cart) {
        if (cart == null || cart.getTotal() == null) {
            mOrderSummarySection.setVisibility(View.GONE);
        } else {
            mOrderSummarySection.setVisibility(View.VISIBLE);
            mOrderSummarySection.update(cart);
        }
    }

    /**
     * Updates the UI to account for changes in payment information.
     *
     * @param section The shipping options.
     */
    public void updateSection(@DataType int whichSection, SectionInformation section) {
        if (whichSection == TYPE_SHIPPING_ADDRESSES) {
            mShippingAddressSectionInformation = section;
            mShippingAddressSection.update(section);
        } else if (whichSection == TYPE_SHIPPING_OPTIONS) {
            mShippingOptionsSectionInformation = section;
            mShippingOptionSection.update(section);
            showShippingOptionSectionIfNecessary();
        } else if (whichSection == TYPE_CONTACT_DETAILS) {
            mContactDetailsSectionInformation = section;
            mContactDetailsSection.update(section);
        } else if (whichSection == TYPE_PAYMENT_METHODS) {
            mPaymentMethodSectionInformation = section;
            mPaymentMethodSection.update(section);
        }

        boolean isFinishingEditItem = mIsEditingPaymentItem;
        mIsEditingPaymentItem = false;
        updateSectionButtons();
        updatePayButtonEnabled();

        // Notify ready for input for test if this is finishing editing item.
        if (isFinishingEditItem) notifyReadyForInput();
    }

    // Only show shipping option section once there are shipping options.
    private void showShippingOptionSectionIfNecessary() {
        if (!mRequestShipping || mShippingOptionsSectionInformation.isEmpty()
                || mPaymentContainerLayout.indexOfChild(mShippingOptionSection) != -1) {
            return;
        }

        // Shipping option section is added below shipping address section.
        int addressSectionIndex = mPaymentContainerLayout.indexOfChild(mShippingAddressSection);
        SectionSeparator sectionSeparator =
                new SectionSeparator(mPaymentContainerLayout, addressSectionIndex + 1);
        mSectionSeparators.add(sectionSeparator);
        if (mIsExpandedToFullHeight) sectionSeparator.expand();
        mPaymentContainerLayout.addView(mShippingOptionSection, addressSectionIndex + 2,
                new LinearLayout.LayoutParams(
                        LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        mPaymentContainerLayout.requestLayout();
    }

    @Override
    public void onEditableOptionChanged(
            final PaymentRequestSection section, EditableOption option) {
        @SelectionResult int result = SELECTION_RESULT_NONE;
        if (section == mShippingAddressSection
                && mShippingAddressSectionInformation.getSelectedItem() != option) {
            mShippingAddressSectionInformation.setSelectedItem(option);
            result = mClient.onSectionOptionSelected(
                    TYPE_SHIPPING_ADDRESSES, option, mUpdateSectionsCallback);
        } else if (section == mShippingOptionSection
                && mShippingOptionsSectionInformation.getSelectedItem() != option) {
            mShippingOptionsSectionInformation.setSelectedItem(option);
            result = mClient.onSectionOptionSelected(
                    TYPE_SHIPPING_OPTIONS, option, mUpdateSectionsCallback);
        } else if (section == mContactDetailsSection) {
            mContactDetailsSectionInformation.setSelectedItem(option);
            result = mClient.onSectionOptionSelected(TYPE_CONTACT_DETAILS, option, null);
        } else if (section == mPaymentMethodSection) {
            mPaymentMethodSectionInformation.setSelectedItem(option);
            result = mClient.onSectionOptionSelected(TYPE_PAYMENT_METHODS, option, null);
        }

        updateStateFromResult(section, result);
    }

    @Override
    public void onEditEditableOption(final PaymentRequestSection section, EditableOption option) {
        @SelectionResult int result = SELECTION_RESULT_NONE;

        assert section != mOrderSummarySection;
        assert section != mShippingOptionSection;
        if (section == mShippingAddressSection) {
            assert mShippingAddressSectionInformation.getSelectedItem() == option;
            result = mClient.onSectionEditOption(
                    TYPE_SHIPPING_ADDRESSES, option, mUpdateSectionsCallback);
        }

        if (section == mContactDetailsSection) {
            assert mContactDetailsSectionInformation.getSelectedItem() == option;
            result = mClient.onSectionEditOption(TYPE_CONTACT_DETAILS, option, null);
        }

        if (section == mPaymentMethodSection) {
            assert mPaymentMethodSectionInformation.getSelectedItem() == option;
            result = mClient.onSectionEditOption(TYPE_PAYMENT_METHODS, option, null);
        }

        updateStateFromResult(section, result);
    }

    @Override
    public void onAddEditableOption(PaymentRequestSection section) {
        assert section != mShippingOptionSection;

        @SelectionResult int result = SELECTION_RESULT_NONE;
        if (section == mShippingAddressSection) {
            result = mClient.onSectionAddOption(TYPE_SHIPPING_ADDRESSES, mUpdateSectionsCallback);
        } else if (section == mContactDetailsSection) {
            result = mClient.onSectionAddOption(TYPE_CONTACT_DETAILS, null);
        } else if (section == mPaymentMethodSection) {
            result = mClient.onSectionAddOption(TYPE_PAYMENT_METHODS, null);
        }

        updateStateFromResult(section, result);
    }

    void updateStateFromResult(PaymentRequestSection section, @SelectionResult int result) {
        mIsClientCheckingSelection = result == SELECTION_RESULT_ASYNCHRONOUS_VALIDATION;
        mIsEditingPaymentItem = result == SELECTION_RESULT_EDITOR_LAUNCH;

        if (mIsClientCheckingSelection) {
            mSelectedSection = section;
            updateSectionVisibility();
            section.setDisplayMode(PaymentRequestSection.DISPLAY_MODE_CHECKING);
        } else {
            expand(null);
        }

        updatePayButtonEnabled();
    }

    @Override
    public boolean isBoldLabelNeeded(PaymentRequestSection section) {
        return section == mShippingAddressSection;
    }

    /** @return The common editor user interface. */
    public EditorDialog getEditorDialog() {
        return mEditorDialog;
    }

    /** @return The card editor user interface. Distinct from the common editor user interface,
     * because the credit card editor can launch the address editor. */
    public EditorDialog getCardEditorDialog() {
        return mCardEditorDialog;
    }

    /**
     * Called when user clicks anything in the dialog.
     */
    @Override
    public void onClick(View v) {
        if (!isAcceptingCloseButton()) return;

        if (v == mCloseButton) {
            dismissDialog(true);
            return;
        }

        if (!isAcceptingUserInput()) return;

        // Users can only expand incomplete sections by clicking on their edit buttons.
        if (v instanceof PaymentRequestSection) {
            PaymentRequestSection section = (PaymentRequestSection) v;
            if (section.getEditButtonState() != EDIT_BUTTON_GONE) return;
        }

        if (v == mOrderSummarySection) {
            expand(mOrderSummarySection);
        } else if (v == mShippingAddressSection) {
            expand(mShippingAddressSection);
        } else if (v == mShippingOptionSection) {
            expand(mShippingOptionSection);
        } else if (v == mContactDetailsSection) {
            expand(mContactDetailsSection);
        } else if (v == mPaymentMethodSection) {
            expand(mPaymentMethodSection);
        } else if (v == mPayButton) {
            processPayButton();
        } else if (v == mEditButton) {
            if (mIsExpandedToFullHeight) {
                dismissDialog(true);
            } else {
                expand(mOrderSummarySection);
            }
        }

        updatePayButtonEnabled();
    }

    /**
     * Dismiss the dialog.
     *
     * @param isAnimated If true, the dialog dismissal is animated.
     */
    private void dismissDialog(boolean isAnimated) {
        mIsClosing = true;
        if (mDialog.isShowing()) {
            if (isAnimated) {
                new DisappearingAnimator(true);
            } else {
                mDialog.dismiss();
            }
        }
    }

    private void processPayButton() {
        assert !mIsShowingSpinner;
        mIsProcessingPayClicked = true;

        boolean shouldShowSpinner = mClient.onPayClicked(
                mShippingAddressSectionInformation == null
                        ? null : mShippingAddressSectionInformation.getSelectedItem(),
                mShippingOptionsSectionInformation == null
                        ? null : mShippingOptionsSectionInformation.getSelectedItem(),
                mPaymentMethodSectionInformation.getSelectedItem());

        if (shouldShowSpinner) {
            changeSpinnerVisibility(true);
        } else {
            mDialog.hide();
        }
    }

    /**
     * Called when user cancelled out of the UI that was shown after they clicked [PAY] button.
     */
    public void onPayButtonProcessingCancelled() {
        assert mIsProcessingPayClicked;
        mIsProcessingPayClicked = false;
        changeSpinnerVisibility(false);
        mDialog.show();
        updatePayButtonEnabled();
    }

    /**
     *  Called to show the processing message after instrument details have been loaded
     *  in the case the payment request UI has been skipped.
     */
    public void showProcessingMessageAfterUiSkip() {
        // Button was clicked before but not marked as clicked because we skipped the UI.
        mIsProcessingPayClicked = true;
        showProcessingMessage();
    }

    /**
     * Called when the user has clicked on pay. The message is shown while the payment information
     * is processed right until a confimation from the merchant is received.
     */
    public void showProcessingMessage() {
        assert mIsProcessingPayClicked;

        changeSpinnerVisibility(true);
        mDialog.show();
    }

    private void changeSpinnerVisibility(boolean showSpinner) {
        if (mIsShowingSpinner == showSpinner) return;
        mIsShowingSpinner = showSpinner;

        if (showSpinner) {
            mPaymentContainer.setVisibility(View.GONE);
            mBottomBar.setVisibility(View.GONE);
            mCloseButton.setVisibility(View.GONE);
            mSpinnyLayout.setVisibility(View.VISIBLE);

            // Turn the bottom sheet back into a collapsed bottom sheet showing only the spinner.
            // TODO(dfalcantara): Animate this: https://crbug.com/621955
            ((FrameLayout.LayoutParams) mRequestView.getLayoutParams()).height =
                    LayoutParams.WRAP_CONTENT;
            mRequestView.requestLayout();
        } else {
            mPaymentContainer.setVisibility(View.VISIBLE);
            mBottomBar.setVisibility(View.VISIBLE);
            mCloseButton.setVisibility(View.VISIBLE);
            mSpinnyLayout.setVisibility(View.GONE);

            if (mIsExpandedToFullHeight) {
                ((FrameLayout.LayoutParams) mRequestView.getLayoutParams()).height =
                        LayoutParams.MATCH_PARENT;
                mRequestView.requestLayout();
            }
        }
    }

    private void updatePayButtonEnabled() {
        boolean contactInfoOk = !mRequestContactDetails
                || (mContactDetailsSectionInformation != null
                           && mContactDetailsSectionInformation.getSelectedItem() != null);
        boolean shippingInfoOk = !mRequestShipping
                || (mShippingAddressSectionInformation != null
                           && mShippingAddressSectionInformation.getSelectedItem() != null
                           && mShippingOptionsSectionInformation != null
                           && mShippingOptionsSectionInformation.getSelectedItem() != null);
        mPayButton.setEnabled(contactInfoOk && shippingInfoOk
                && mPaymentMethodSectionInformation != null
                && mPaymentMethodSectionInformation.getSelectedItem() != null
                && !mIsClientCheckingSelection
                && !mIsEditingPaymentItem
                && !mIsClosing);
        mReadyToPayNotifierForTest.run();
    }

    /** @return Whether or not the dialog can be closed via the X close button. */
    private boolean isAcceptingCloseButton() {
        return mSheetAnimator == null && mSectionAnimator == null && !mIsProcessingPayClicked
                && !mIsEditingPaymentItem && !mIsClosing;
    }

    /** @return Whether or not the dialog is accepting user input. */
    @Override
    public boolean isAcceptingUserInput() {
        return isAcceptingCloseButton() && mPaymentMethodSectionInformation != null
                && !mIsClientCheckingSelection;
    }

    /**
     * Sets the observer to be called when the shipping address section gains or loses focus.
     *
     * @param observer The observer to notify.
     */
    public void setShippingAddressSectionFocusChangedObserver(
            OptionSection.FocusChangedObserver observer) {
        mShippingAddressSection.setOptionSectionFocusChangedObserver(observer);
    }

    private void expand(PaymentRequestSection section) {
        if (!mIsExpandedToFullHeight) {
            // Container now takes the full height of the screen, animating towards it.
            mRequestView.getLayoutParams().height = LayoutParams.MATCH_PARENT;
            mRequestView.addOnLayoutChangeListener(new SheetEnlargingAnimator(true));

            // New separators appear at the top and bottom of the list.
            mPaymentContainer.setEdgeVisibility(
                    FadingEdgeScrollView.DRAW_HARD_EDGE, FadingEdgeScrollView.DRAW_FADING_EDGE);
            mSectionSeparators.add(new SectionSeparator(mPaymentContainerLayout, -1));

            // Add a link to Autofill settings.
            addCardAndAddressOptionsSettingsView(mPaymentContainerLayout);

            // Expand all the dividers.
            for (int i = 0; i < mSectionSeparators.size(); i++) mSectionSeparators.get(i).expand();
            mPaymentContainerLayout.requestLayout();

            // Switch the 'edit' button to a 'cancel' button.
            mEditButton.setText(mContext.getString(R.string.cancel));

            // Make the dialog take the whole screen.
            mDialog.getWindow().setLayout(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

            // Disable all but the first button.
            updateSectionButtons();

            mIsExpandedToFullHeight = true;
        }

        // Update the section contents when they're selected.
        mSelectedSection = section;
        if (mSelectedSection == mOrderSummarySection) {
            mClient.getShoppingCart(new Callback<ShoppingCart>() {
                @Override
                public void onResult(ShoppingCart result) {
                    updateOrderSummarySection(result);
                    updateSectionVisibility();
                }
            });
        } else if (mSelectedSection == mShippingAddressSection) {
            mClient.getSectionInformation(
                    TYPE_SHIPPING_ADDRESSES, createUpdateSectionCallback(TYPE_SHIPPING_ADDRESSES));
        } else if (mSelectedSection == mShippingOptionSection) {
            mClient.getSectionInformation(
                    TYPE_SHIPPING_OPTIONS, createUpdateSectionCallback(TYPE_SHIPPING_OPTIONS));
        } else if (mSelectedSection == mContactDetailsSection) {
            mClient.getSectionInformation(
                    TYPE_CONTACT_DETAILS, createUpdateSectionCallback(TYPE_CONTACT_DETAILS));
        } else if (mSelectedSection == mPaymentMethodSection) {
            mClient.getSectionInformation(
                    TYPE_PAYMENT_METHODS, createUpdateSectionCallback(TYPE_PAYMENT_METHODS));
        } else {
            updateSectionVisibility();
        }
    }

    private void addCardAndAddressOptionsSettingsView(LinearLayout parent) {
        String message;
        if (!mShowDataSource) {
            message = mContext.getString(R.string.payments_card_and_address_settings);
        } else if (ChromeSigninController.get().isSignedIn()) {
            message = mContext.getString(R.string.payments_card_and_address_settings_signed_in,
                    ChromeSigninController.get().getSignedInAccountName());
        } else {
            message = mContext.getString(R.string.payments_card_and_address_settings_signed_out);
        }

        NoUnderlineClickableSpan settingsSpan =
                new NoUnderlineClickableSpan((widget) -> mClient.onCardAndAddressSettingsClicked());
        SpannableString spannableMessage = SpanApplier.applySpans(
                message, new SpanInfo("BEGIN_LINK", "END_LINK", settingsSpan));

        TextView view = new TextViewWithClickableSpans(mContext);
        view.setText(spannableMessage);
        view.setMovementMethod(LinkMovementMethod.getInstance());
        ApiCompatibilityUtils.setTextAppearance(view, R.style.PaymentsUiSectionDescriptiveText);

        // Add paddings instead of margin to let getMeasuredHeight return correct value for section
        // resize animation.
        int paddingSize = mContext.getResources().getDimensionPixelSize(
                R.dimen.editor_dialog_section_large_spacing);
        ApiCompatibilityUtils.setPaddingRelative(
                view, paddingSize, paddingSize, paddingSize, paddingSize);
        parent.addView(view);
    }

    private Callback<SectionInformation> createUpdateSectionCallback(@DataType final int type) {
        return new Callback<SectionInformation>() {
            @Override
            public void onResult(SectionInformation result) {
                updateSection(type, result);
                updateSectionVisibility();
            }
        };
    }

    /** Update the display status of each expandable section in the full dialog. */
    private void updateSectionVisibility() {
        startSectionResizeAnimation();
        mOrderSummarySection.focusSection(mSelectedSection == mOrderSummarySection);
        mShippingAddressSection.focusSection(mSelectedSection == mShippingAddressSection);
        mShippingOptionSection.focusSection(mSelectedSection == mShippingOptionSection);
        mContactDetailsSection.focusSection(mSelectedSection == mContactDetailsSection);
        mPaymentMethodSection.focusSection(mSelectedSection == mPaymentMethodSection);
        updateSectionButtons();
    }

    /**
     * Updates the enabled/disabled state of each section's edit button.
     *
     * Only the top-most button is enabled -- the others are disabled so the user is directed
     * through the form from top to bottom.
     */
    private void updateSectionButtons() {
        // Disable edit buttons when the client is checking a selection.
        boolean mayEnableButton = !mIsClientCheckingSelection;
        for (int i = 0; i < mPaymentContainerLayout.getChildCount(); i++) {
            View child = mPaymentContainerLayout.getChildAt(i);
            if (!(child instanceof PaymentRequestSection)) continue;

            PaymentRequestSection section = (PaymentRequestSection) child;
            section.setIsEditButtonEnabled(mayEnableButton);
            if (section.getEditButtonState() != EDIT_BUTTON_GONE) mayEnableButton = false;
        }
    }

    /**
     * Called when the dialog is dismissed. Can be caused by:
     * <ul>
     *  <li>User click on the "back" button on the phone.</li>
     *  <li>User click on the "X" button in the top-right corner of the dialog.</li>
     *  <li>User click on the "CANCEL" button on the bottom of the dialog.</li>
     *  <li>Successfully processing the payment.</li>
     *  <li>Failure to process the payment.</li>
     *  <li>The JavaScript calling the abort() method in PaymentRequest API.</li>
     *  <li>The PaymentRequest JavaScript object being destroyed.</li>
     *  <li>User closing all incognito windows with PaymentRequest UI open in an incognito
     *      window.</li>
     * </ul>
     */
    @Override
    public void onDismiss(DialogInterface dialog) {
        mIsClosing = true;
        if (mEditorDialog.isShowing()) mEditorDialog.dismiss();
        if (mCardEditorDialog.isShowing()) mCardEditorDialog.dismiss();
        if (sEditorObserverForTest != null) sEditorObserverForTest.onEditorDismiss();
        if (!mIsClientClosing) mClient.onDismiss();
    }

    @Override
    public String getAdditionalText(PaymentRequestSection section) {
        if (section == mShippingAddressSection) {
            int selectedItemIndex = mShippingAddressSectionInformation.getSelectedItemIndex();
            if (selectedItemIndex != SectionInformation.NO_SELECTION
                    && selectedItemIndex != SectionInformation.INVALID_SELECTION) {
                return null;
            }

            String customErrorMessage = mShippingAddressSectionInformation.getErrorMessage();
            if (selectedItemIndex == SectionInformation.INVALID_SELECTION
                    && !TextUtils.isEmpty(customErrorMessage)) {
                return customErrorMessage;
            }

            return mContext.getString(selectedItemIndex == SectionInformation.NO_SELECTION
                            ? mShippingStrings.getSelectPrompt()
                            : mShippingStrings.getUnsupported());
        } else if (section == mPaymentMethodSection) {
            return mPaymentMethodSectionInformation.getAdditionalText();
        } else {
            return null;
        }
    }

    @Override
    public boolean isAdditionalTextDisplayingWarning(PaymentRequestSection section) {
        return section == mShippingAddressSection
                && mShippingAddressSectionInformation != null
                && mShippingAddressSectionInformation.getSelectedItemIndex()
                        == SectionInformation.INVALID_SELECTION;
    }

    @Override
    public void onSectionClicked(PaymentRequestSection section) {
        expand(section);
    }

    /**
     * Animates the different sections of the dialog expanding and contracting into their final
     * positions.
     */
    private void startSectionResizeAnimation() {
        Runnable animationEndRunnable = new Runnable() {
            @Override
            public void run() {
                mSectionAnimator = null;
                notifyReadyForInput();
                mReadyToPayNotifierForTest.run();
            }
        };

        mSectionAnimator =
                new FocusAnimator(mPaymentContainerLayout, mSelectedSection, animationEndRunnable);
    }

    /**
     * Animates the whole dialog fading in and darkening everything else on screen.
     * This particular animation is not tracked because it is not meant to be cancellable.
     */
    private class FadeInAnimator
            extends AnimatorListenerAdapter implements OnLayoutChangeListener {
        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom,
                int oldLeft, int oldTop, int oldRight, int oldBottom) {
            mRequestView.removeOnLayoutChangeListener(this);

            Animator scrimFader = ObjectAnimator.ofInt(mFullContainer.getBackground(),
                    AnimatorProperties.DRAWABLE_ALPHA_PROPERTY, 0, 255);
            Animator alphaAnimator = ObjectAnimator.ofFloat(mFullContainer, View.ALPHA, 0f, 1f);

            AnimatorSet alphaSet = new AnimatorSet();
            alphaSet.playTogether(scrimFader, alphaAnimator);
            alphaSet.setDuration(DIALOG_ENTER_ANIMATION_MS);
            alphaSet.setInterpolator(new LinearOutSlowInInterpolator());
            alphaSet.start();
        }
    }

    /**
     * Animates the bottom sheet UI translating upwards from the bottom of the screen.
     * Can be canceled when a {@link SheetEnlargingAnimator} starts and expands the dialog.
     */
    private class PeekingAnimator
            extends AnimatorListenerAdapter implements OnLayoutChangeListener {
        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom,
                int oldLeft, int oldTop, int oldRight, int oldBottom) {
            mRequestView.removeOnLayoutChangeListener(this);

            mSheetAnimator = ObjectAnimator.ofFloat(
                    mRequestView, View.TRANSLATION_Y, mAnimatorTranslation, 0);
            mSheetAnimator.setDuration(DIALOG_ENTER_ANIMATION_MS);
            mSheetAnimator.setInterpolator(new LinearOutSlowInInterpolator());
            mSheetAnimator.addListener(this);
            mSheetAnimator.start();
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            mSheetAnimator = null;
        }
    }

    /** Animates the bottom sheet expanding to a larger sheet. */
    private class SheetEnlargingAnimator
            extends AnimatorListenerAdapter implements OnLayoutChangeListener {
        private final boolean mIsBottomBarLockedInPlace;
        private int mContainerHeightDifference;

        public SheetEnlargingAnimator(boolean isBottomBarLockedInPlace) {
            mIsBottomBarLockedInPlace = isBottomBarLockedInPlace;
        }

        /**
         * Updates the animation.
         *
         * @param progress How far along the animation is.  In the range [0,1], with 1 being done.
         */
        private void update(float progress) {
            // The dialog container initially starts off translated downward, gradually decreasing
            // the translation until it is in the right place on screen.
            float containerTranslation = mContainerHeightDifference * progress;
            mRequestView.setTranslationY(containerTranslation);

            if (mIsBottomBarLockedInPlace) {
                // The bottom bar is translated along the dialog so that is looks like it stays in
                // place at the bottom while the entire bottom sheet is translating upwards.
                mBottomBar.setTranslationY(-containerTranslation);

                // The payment container is sandwiched between the header and the bottom bar.
                // Expansion animates by changing where its "bottom" is, letting its shadows appear
                // and disappear as it changes size.
                int paymentContainerBottom =
                        Math.min(mPaymentContainer.getTop() + mPaymentContainer.getMeasuredHeight(),
                                mBottomBar.getTop());
                mPaymentContainer.setBottom(paymentContainerBottom);
            }
        }

        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom,
                int oldLeft, int oldTop, int oldRight, int oldBottom) {
            if (mSheetAnimator != null) mSheetAnimator.cancel();

            mRequestView.removeOnLayoutChangeListener(this);
            mContainerHeightDifference = (bottom - top) - (oldBottom - oldTop);

            ValueAnimator containerAnimator = ValueAnimator.ofFloat(1f, 0f);
            containerAnimator.addUpdateListener(new AnimatorUpdateListener() {
                @Override
                public void onAnimationUpdate(ValueAnimator animation) {
                    float alpha = (Float) animation.getAnimatedValue();
                    update(alpha);
                }
            });

            mSheetAnimator = containerAnimator;
            mSheetAnimator.setDuration(DIALOG_ENTER_ANIMATION_MS);
            mSheetAnimator.setInterpolator(new LinearOutSlowInInterpolator());
            mSheetAnimator.addListener(this);
            mSheetAnimator.start();
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            // Reset the layout so that everything is in the expected place.
            mRequestView.setTranslationY(0);
            mBottomBar.setTranslationY(0);
            mRequestView.requestLayout();

            // Indicate that the dialog is ready to use.
            mSheetAnimator = null;
            notifyReadyForInput();
            mReadyToPayNotifierForTest.run();
        }
    }

    /** Animates the bottom sheet (and optionally, the scrim) disappearing off screen. */
    private class DisappearingAnimator extends AnimatorListenerAdapter {
        private final boolean mIsDialogClosing;

        public DisappearingAnimator(boolean removeDialog) {
            mIsDialogClosing = removeDialog;

            Animator sheetFader = ObjectAnimator.ofFloat(
                    mRequestView, View.ALPHA, mRequestView.getAlpha(), 0f);
            Animator sheetTranslator = ObjectAnimator.ofFloat(
                    mRequestView, View.TRANSLATION_Y, 0f, mAnimatorTranslation);

            AnimatorSet current = new AnimatorSet();
            current.setDuration(DIALOG_EXIT_ANIMATION_MS);
            current.setInterpolator(new FastOutLinearInInterpolator());
            if (mIsDialogClosing) {
                Animator scrimFader = ObjectAnimator.ofInt(mFullContainer.getBackground(),
                        AnimatorProperties.DRAWABLE_ALPHA_PROPERTY, 127, 0);
                current.playTogether(sheetFader, sheetTranslator, scrimFader);
            } else {
                current.playTogether(sheetFader, sheetTranslator);
            }

            mSheetAnimator = current;
            mSheetAnimator.addListener(this);
            mSheetAnimator.start();
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            mSheetAnimator = null;
            mFullContainer.removeView(mRequestView);
            if (mIsDialogClosing && mDialog.isShowing()) mDialog.dismiss();
        }
    }

    @VisibleForTesting
    public static void setEditorObserverForTest(EditorObserverForTest editorObserverForTest) {
        sEditorObserverForTest = editorObserverForTest;
    }

    @VisibleForTesting
    public static void setPaymentRequestObserverForTest(
            PaymentRequestObserverForTest paymentRequestObserverForTest) {
        sPaymentRequestObserverForTest = paymentRequestObserverForTest;
    }

    @VisibleForTesting
    public Dialog getDialogForTest() {
        return mDialog;
    }

    @VisibleForTesting
    public TextView getOrderSummaryTotalTextViewForTest() {
        return mOrderSummarySection.getSummaryRightTextView();
    }

    @VisibleForTesting
    public OptionSection getShippingAddressSectionForTest() {
        return mShippingAddressSection;
    }

    @VisibleForTesting
    public OptionSection getShippingOptionSectionForTest() {
        return mShippingOptionSection;
    }

    @VisibleForTesting
    public ViewGroup getPaymentMethodSectionForTest() {
        return mPaymentMethodSection;
    }

    @VisibleForTesting
    public PaymentRequestSection getContactDetailsSectionForTest() {
        return mContactDetailsSection;
    }

    private void notifyReadyForInput() {
        if (sPaymentRequestObserverForTest != null && isAcceptingUserInput()) {
            sPaymentRequestObserverForTest.onPaymentRequestReadyForInput(this);
        }
    }

    private void notifySelectionChecked() {
        if (sPaymentRequestObserverForTest != null) {
            sPaymentRequestObserverForTest.onPaymentRequestSelectionChecked(this);
        }
    }
}
