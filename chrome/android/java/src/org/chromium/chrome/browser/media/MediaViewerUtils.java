// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.provider.Browser;
import android.support.customtabs.CustomTabsIntent;
import android.text.TextUtils;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;

import java.util.Locale;

/**
 * A class containing some utility static methods.
 */
public class MediaViewerUtils {
    private static final String DEFAULT_MIME_TYPE = "*/*";
    private static final String MIMETYPE_IMAGE = "image";

    /**
     * Creates an Intent that allows viewing the given file in an internal media viewer.
     * @param displayUri               URI to display to the user, ideally in file:// form.
     * @param contentUri               content:// URI pointing at the file.
     * @param mimeType                 MIME type of the file.
     * @param allowExternalAppHandlers Whether the viewer should allow the user to open with another
     *                                 app.
     * @return Intent that can be fired to open the file.
     */
    public static Intent getMediaViewerIntent(
            Uri displayUri, Uri contentUri, String mimeType, boolean allowExternalAppHandlers) {
        Context context = ContextUtils.getApplicationContext();

        Bitmap closeIcon = BitmapFactory.decodeResource(
                context.getResources(), R.drawable.ic_arrow_back_white_24dp);
        Bitmap shareIcon = BitmapFactory.decodeResource(
                context.getResources(), R.drawable.ic_share_white_24dp);

        CustomTabsIntent.Builder builder = new CustomTabsIntent.Builder();
        builder.setToolbarColor(Color.BLACK);
        builder.setCloseButtonIcon(closeIcon);
        builder.setShowTitle(true);

        if (allowExternalAppHandlers) {
            // Create a PendingIntent that can be used to view the file externally.
            // TODO(https://crbug.com/795968): Check if this is problematic in multi-window mode,
            //                                 where two different viewers could be visible at the
            //                                 same time.
            Intent viewIntent = createViewIntentForUri(contentUri, mimeType, null, null);
            Intent chooserIntent = Intent.createChooser(viewIntent, null);
            chooserIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            String openWithStr = context.getString(R.string.download_manager_open_with);
            PendingIntent pendingViewIntent = PendingIntent.getActivity(
                    context, 0, chooserIntent, PendingIntent.FLAG_CANCEL_CURRENT);
            builder.addMenuItem(openWithStr, pendingViewIntent);
        }

        // Create a PendingIntent that shares the file with external apps.
        PendingIntent pendingShareIntent = PendingIntent.getActivity(context, 0,
                createShareIntent(contentUri, mimeType), PendingIntent.FLAG_CANCEL_CURRENT);
        builder.setActionButton(
                shareIcon, context.getString(R.string.share), pendingShareIntent, true);

        // The color of the media viewer is dependent on the file type.
        int backgroundRes;
        if (isImageType(mimeType)) {
            backgroundRes = R.color.image_viewer_bg;
        } else {
            backgroundRes = R.color.media_viewer_bg;
        }
        int mediaColor = ApiCompatibilityUtils.getColor(context.getResources(), backgroundRes);

        // Build up the Intent further.
        Intent intent = builder.build().intent;
        intent.setPackage(context.getPackageName());
        intent.setData(contentUri);
        intent.putExtra(CustomTabIntentDataProvider.EXTRA_UI_TYPE,
                CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_MEDIA_VIEWER);
        intent.putExtra(CustomTabIntentDataProvider.EXTRA_MEDIA_VIEWER_URL, displayUri.toString());
        intent.putExtra(CustomTabIntentDataProvider.EXTRA_ENABLE_EMBEDDED_MEDIA_EXPERIENCE, true);
        intent.putExtra(CustomTabIntentDataProvider.EXTRA_INITIAL_BACKGROUND_COLOR, mediaColor);
        intent.putExtra(CustomTabsIntent.EXTRA_TOOLBAR_COLOR, mediaColor);
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        IntentHandler.addTrustedIntentExtras(intent);

        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(context, ChromeLauncherActivity.class);
        return intent;
    }

    /**
     * Creates an Intent to open the file in another app by firing an Intent to Android.
     * @param fileUri  Uri pointing to the file.
     * @param mimeType MIME type for the file.
     * @param originalUrl The original url of the downloaded file.
     * @param referrer Referrer of the downloaded file.
     * @return Intent that can be used to start an Activity for the file.
     */
    public static Intent createViewIntentForUri(
            Uri fileUri, String mimeType, String originalUrl, String referrer) {
        Intent fileIntent = new Intent(Intent.ACTION_VIEW);
        String normalizedMimeType = Intent.normalizeMimeType(mimeType);
        if (TextUtils.isEmpty(normalizedMimeType)) {
            fileIntent.setData(fileUri);
        } else {
            fileIntent.setDataAndType(fileUri, normalizedMimeType);
        }
        fileIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        fileIntent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        fileIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        setOriginalUrlAndReferralExtraToIntent(fileIntent, originalUrl, referrer);
        return fileIntent;
    }

    /**
     * Adds the originating Uri and referrer extras to an intent if they are not null.
     * @param intent      Intent for adding extras.
     * @param originalUrl The original url of the downloaded file.
     * @param referrer    Referrer of the downloaded file.
     */
    public static void setOriginalUrlAndReferralExtraToIntent(
            Intent intent, String originalUrl, String referrer) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) return;
        if (originalUrl != null) intent.putExtra(Intent.EXTRA_ORIGINATING_URI, originalUrl);
        if (referrer != null) intent.putExtra(Intent.EXTRA_REFERRER, referrer);
    }

    private static Intent createShareIntent(Uri fileUri, String mimeType) {
        if (TextUtils.isEmpty(mimeType)) mimeType = DEFAULT_MIME_TYPE;

        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.putExtra(Intent.EXTRA_STREAM, fileUri);
        intent.setType(mimeType);
        return intent;
    }

    private static boolean isImageType(String mimeType) {
        if (TextUtils.isEmpty(mimeType)) return false;

        String[] pieces = mimeType.toLowerCase(Locale.getDefault()).split("/");
        if (pieces.length != 2) return false;

        return MIMETYPE_IMAGE.equals(pieces[0]);
    }
}
