package org.chromium.chrome.browser.mises;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

public class AutoStartUpBootReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent.getAction().equals("android.intent.action.BOOT_COMPLETED")) {
            //if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            //    context.startForegroundService(new Intent(context, MisesLCDService.class));
            //} else {
            //    context.startService(new Intent(context, MisesLCDService.class));
            //}
        }
    }
}
