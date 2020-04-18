package com.android.ex.photo.util;

import android.content.Context;
import android.os.Build;
import android.support.v4.view.accessibility.AccessibilityEventCompat;
import android.support.v4.view.accessibility.AccessibilityRecordCompat;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;

public class Util {
    public static boolean isTouchExplorationEnabled(AccessibilityManager accessibilityManager) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            return accessibilityManager.isTouchExplorationEnabled();
        } else {
            return false;
        }
    }

    /**
     * Make an announcement which is related to some sort of a context change. Also see
     * {@link android.view.View#announceForAccessibility}
     * @param view The view that triggered the announcement
     * @param accessibilityManager AccessibilityManager instance. If it is null, the method can
     *          obtain an instance itself.
     * @param text The announcement text
     */
    public static void announceForAccessibility(
            final View view, AccessibilityManager accessibilityManager,
            final CharSequence text) {
        // Jelly Bean added support for speaking text verbatim
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            view.announceForAccessibility(text);
            return;
        }

        final Context context = view.getContext().getApplicationContext();
        if (accessibilityManager == null) {
            accessibilityManager = (AccessibilityManager) context.getSystemService(
                    Context.ACCESSIBILITY_SERVICE);
        }

        if (!accessibilityManager.isEnabled()) {
            return;
        }

        final int eventType = AccessibilityEvent.TYPE_VIEW_FOCUSED;

        // Construct an accessibility event with the minimum recommended
        // attributes. An event without a class name or package may be dropped.
        final AccessibilityEvent event = AccessibilityEvent.obtain(eventType);
        event.getText().add(text);
        event.setEnabled(view.isEnabled());
        event.setClassName(view.getClass().getName());
        event.setPackageName(context.getPackageName());

        // JellyBean MR1 requires a source view to set the window ID.
        final AccessibilityRecordCompat record = AccessibilityEventCompat.asRecord(event);
        record.setSource(view);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            view.getParent().requestSendAccessibilityEvent(view, event);
        } else {
            // Sends the event directly through the accessibility manager.
            accessibilityManager.sendAccessibilityEvent(event);
        }
    }
}
