package com.android.internal.telephony;

import android.content.Context;
import android.provider.BlockedNumberContract;
import android.telephony.Rlog;

/**
 * {@hide} Checks for blocked phone numbers against {@link BlockedNumberContract}
 */
public class BlockChecker {
    private static final String TAG = "BlockChecker";
    private static final boolean VDBG = false; // STOPSHIP if true.

    /**
     * Returns {@code true} if {@code phoneNumber} is blocked.
     * <p>
     * This method catches all underlying exceptions to ensure that this method never throws any
     * exception.
     */
    public static boolean isBlocked(Context context, String phoneNumber) {
        boolean isBlocked = false;
        long startTimeNano = System.nanoTime();

        try {
            if (BlockedNumberContract.SystemContract.shouldSystemBlockNumber(
                    context, phoneNumber)) {
                Rlog.d(TAG, phoneNumber + " is blocked.");
                isBlocked = true;
            }
        } catch (Exception e) {
            Rlog.e(TAG, "Exception checking for blocked number: " + e);
        }

        int durationMillis = (int) ((System.nanoTime() - startTimeNano) / 1000000);
        if (durationMillis > 500 || VDBG) {
            Rlog.d(TAG, "Blocked number lookup took: " + durationMillis + " ms.");
        }
        return isBlocked;
    }
}
