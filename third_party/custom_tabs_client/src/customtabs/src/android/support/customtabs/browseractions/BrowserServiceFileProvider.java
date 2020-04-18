/*
 * Copyright 2018 The Android Open Source Project
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
package android.support.customtabs.browseractions;

import android.content.ClipData;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.support.annotation.UiThread;
import android.support.annotation.WorkerThread;
import android.support.annotation.GuardedBy;
import android.support.v4.content.FileProvider;
import android.support.v4.util.AtomicFile;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * The class to pass images asynchronously between different applications.
 * Call {@link generateUri(Context, Bitmap, String, int, List<String>)} to save the image
 * and generate a uri to access the image.
 * To access the image, pass the uri to {@link BrowserServiceImageReadTask}.
 */
public class BrowserServiceFileProvider extends FileProvider {
    private static final String TAG = "BrowserServiceFileProvider";
    private static final String AUTHORITY_SUFFIX = ".image_provider";
    private static final String CONTENT_SCHEME = "content";
    private static final String FILE_SUB_DIR = "image_provider";
    private static final String FILE_SUB_DIR_NAME = "image_provider_images/";
    private static final String FILE_EXTENSION = ".png";
    private static final String CLIP_DATA_LABEL = "image_provider_uris";
    private static final String LAST_CLEANUP_TIME_KEY = "last_cleanup_time";

    // A Set tracks the urls whose images are in serialization.
    @GuardedBy("sLatchMapLock")
    private static Set<Uri> sFilesInSerialization = new HashSet<>();

    /**
     * Map the uri with the {@link CountDownLatch} for waiting the image finish serialization.
    /* When the image finishes serialization, the latch is used to notify all await threads to
    /* access the image.
     */
    @GuardedBy("sLatchMapLock")
    private static Map<Uri, CountDownLatch> sUriLatchMap = new HashMap<>();
    private static Object sLatchMapLock = new Object();
    private static Object sFileCleanupLock = new Object();

    private static class FileCleanupTask extends AsyncTask<Void, Void, Void> {
        private final WeakReference<Context> mContextRef;
        private static final long IMAGE_RETENTION_DURATION = TimeUnit.DAYS.toMillis(7);
        private static final long CLEANUP_REQUIRED_TIME_SPAN = TimeUnit.DAYS.toMillis(7);
        private static final long DELETION_FAILED_REATTEMPT_DURATION = TimeUnit.DAYS.toMillis(1);

        public FileCleanupTask(WeakReference<Context> contextRef) {
            super();
            mContextRef = contextRef;
        }

        @Override
        protected Void doInBackground(Void... params) {
            Context context = mContextRef.get();
            if (context == null) return null;
            SharedPreferences prefs = context.getSharedPreferences(
                    context.getPackageName() + AUTHORITY_SUFFIX, Context.MODE_PRIVATE);
            if (!shouldCleanUp(prefs)) return null;
            synchronized (sFileCleanupLock) {
                boolean allFilesDeletedSuccessfully = true;
                File path = new File(context.getFilesDir(), FILE_SUB_DIR);
                if (!path.exists()) return null;
                File[] files = path.listFiles();
                long retentionDate = System.currentTimeMillis() - IMAGE_RETENTION_DURATION;
                for (File file : files) {
                    if (!isImageFile(file)) continue;
                    long lastModified = file.lastModified();
                    if (lastModified < retentionDate && !file.delete()) {
                        Log.e(TAG, "Fail to delete image: " + file.getAbsoluteFile());
                        allFilesDeletedSuccessfully = false;
                    }
                }
                // If fail to delete some files, kill off clean up task after one day.
                long lastCleanUpTime;
                if (allFilesDeletedSuccessfully) {
                    lastCleanUpTime = System.currentTimeMillis();
                } else {
                    lastCleanUpTime = System.currentTimeMillis() - CLEANUP_REQUIRED_TIME_SPAN
                            + DELETION_FAILED_REATTEMPT_DURATION;
                }
                Editor editor = prefs.edit();
                editor.putLong(LAST_CLEANUP_TIME_KEY, lastCleanUpTime);
                editor.apply();
            }
            return null;
        }

        private boolean isImageFile(File file) {
            String filename = file.getName();
            return filename.endsWith("." + FILE_EXTENSION);
        }

        private boolean shouldCleanUp(SharedPreferences prefs) {
            long lastCleanup = prefs.getLong(LAST_CLEANUP_TIME_KEY, System.currentTimeMillis());
            return System.currentTimeMillis() > lastCleanup + CLEANUP_REQUIRED_TIME_SPAN;
        }
    }

    private static class FileSaveTask extends AsyncTask<String, Void, Void> {
        private final WeakReference<Context> mContextRef;
        private final String mFilename;
        private final Bitmap mBitmap;
        private final Uri mFileUri;

        public FileSaveTask(Context context, String filename, Bitmap bitmap, Uri fileUri) {
            super();
            mContextRef = new WeakReference<Context>(context);
            mFilename = filename;
            mBitmap = bitmap;
            mFileUri = fileUri;
        }

        @Override
        protected Void doInBackground(String... params) {
            saveFileIfNeededBlocking();
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            // If file is pending for access, notify to read the fileUri.
            synchronized (sLatchMapLock) {
                if (sUriLatchMap.containsKey(mFileUri)) {
                    CountDownLatch latch = sUriLatchMap.get(mFileUri);
                    latch.countDown();
                }
                sFilesInSerialization.remove(mFileUri);
                sUriLatchMap.remove(mFileUri);
            }
            new FileCleanupTask(mContextRef).executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
        }

        private void saveFileBlocking(File img) {
            FileOutputStream fOut = null;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
                AtomicFile atomicFile = new AtomicFile(img);
                try {
                    fOut = atomicFile.startWrite();
                    mBitmap.compress(Bitmap.CompressFormat.PNG, 100, fOut);
                    fOut.close();
                    atomicFile.finishWrite(fOut);
                } catch (IOException e) {
                    Log.e(TAG, "Fail to save file", e);
                    atomicFile.failWrite(fOut);
                }
            } else {
                try {
                    fOut = new FileOutputStream(img);
                    mBitmap.compress(Bitmap.CompressFormat.PNG, 100, fOut);
                    fOut.close();
                } catch (IOException e) {
                    Log.e(TAG, "Fail to save file", e);
                }
            }
        }

        private void saveFileIfNeededBlocking() {
            if (mContextRef.get() == null) return;
            File path = new File(mContextRef.get().getFilesDir(), FILE_SUB_DIR);
            synchronized (sFileCleanupLock) {
                if (!path.exists() && !path.mkdir()) return;
                File img = new File(path, mFilename + FILE_EXTENSION);
                if (!img.exists()) saveFileBlocking(img);
                img.setLastModified(System.currentTimeMillis());
            }
        }
    }

    /**
     * Request a {@link Uri} used to access the bitmap through the file provider.
     * @param context The {@link Context} used to generate the uri, save the bitmap and grant the
     *                read permission.
     * @param bitmap The {@link Bitmap} to be saved and access through the file provider.
     * @param name The name of the bitmap.
     * @param version The version number of the bitmap. Note: This plus the name decides the
     *                 filename of the bitmap. If it matches with existing file, bitmap will skip
     *                 saving.
     * @return The uri to access the bitmap.
     */
    @UiThread
    public static Uri generateUri(Context context, Bitmap bitmap, String name, int version) {
        String filename = name + "_" + Integer.toString(version);
        Uri uri = generateUri(context, filename);
        boolean shouldSaveImage;
        synchronized (sLatchMapLock) {
            shouldSaveImage = sFilesInSerialization.add(uri);
        }
        if (shouldSaveImage) {
            FileSaveTask fileSaveTask = new FileSaveTask(context, filename, bitmap, uri);
            fileSaveTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
        return uri;
    }

    private static Uri generateUri(Context context, String filename) {
        String fileName = FILE_SUB_DIR_NAME + filename + FILE_EXTENSION;
        return new Uri.Builder()
                .scheme(CONTENT_SCHEME)
                .authority(context.getPackageName() + AUTHORITY_SUFFIX)
                .path(fileName)
                .build();
    }

    /**
     * Grant the read permission to a list of {@link Uri} sent through a {@link Intent}.
     * @param intent The sending Intent which holds a list of Uri.
     * @param uris A list of Uri generated by generateUri(Context, Bitmap, String, int,
     *             List<String>).
     * @param context The context requests to grant the permission.
     */
    public static void grantReadPermission(Intent intent, List<Uri> uris, Context context) {
        if (uris == null || uris.size() == 0) return;
        ContentResolver resolver = context.getContentResolver();
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        ClipData clipData = ClipData.newUri(resolver, CLIP_DATA_LABEL, uris.get(0));
        for (int i = 1; i < uris.size(); i++) {
            clipData.addItem(new ClipData.Item(uris.get(i)));
        }
        intent.setClipData(clipData);
    }

    @Override
    @WorkerThread
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!blockUntilFileReady(uri)) {
            throw new FileNotFoundException("File open is interrupted");
        }
        return super.openFile(uri, mode);
    }

    @Override
    @WorkerThread
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        if (!blockUntilFileReady(uri)) return null;
        return super.query(uri, projection, selection, selectionArgs, sortOrder);
    }

    private boolean blockUntilFileReady(Uri fileUri) {
        CountDownLatch latch;
        synchronized (sLatchMapLock) {
            if (!sFilesInSerialization.contains(fileUri)) return true;
            if (sUriLatchMap.containsKey(fileUri)) {
                latch = sUriLatchMap.get(fileUri);
            } else {
                latch = new CountDownLatch(1);
                sUriLatchMap.put(fileUri, latch);
            }
        }
        try {
            latch.await();
        } catch (InterruptedException e) {
            Log.e(TAG, "Interrupt waiting for file: " + fileUri.toString());
            return false;
        }
        return true;
    }
}
