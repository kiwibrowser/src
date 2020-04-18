// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.crash;

import android.os.Build;
import android.util.Patterns;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.components.minidump_uploader.CrashFileManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Extracts the recent logcat output from an Android device, elides PII sensitive info from it,
 * prepends the logcat data to the caller-provided minidump file, and initiates upload for the crash
 * report.
 *
 * Elided information includes: Emails, IP address, MAC address, URL/domains as well as Javascript
 * console messages.
 */
public class LogcatExtractionRunnable implements Runnable {
    private static final String TAG = "LogcatExtraction";
    private static final long HALF_SECOND = 500;

    protected static final int LOGCAT_SIZE = 256; // Number of lines.

    protected static final String EMAIL_ELISION = "XXX@EMAIL.ELIDED";

    @VisibleForTesting
    protected static final String URL_ELISION = "HTTP://WEBADDRESS.ELIDED";

    private static final String GOOD_IRI_CHAR = "a-zA-Z0-9\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF";

    private static final Pattern IP_ADDRESS = Pattern.compile(
            "((25[0-5]|2[0-4][0-9]|[0-1][0-9]{2}|[1-9][0-9]|[1-9])\\.(25[0-5]|2[0-4]"
            + "[0-9]|[0-1][0-9]{2}|[1-9][0-9]|[1-9]|0)\\.(25[0-5]|2[0-4][0-9]|[0-1]"
            + "[0-9]{2}|[1-9][0-9]|[1-9]|0)\\.(25[0-5]|2[0-4][0-9]|[0-1][0-9]{2}"
            + "|[1-9][0-9]|[0-9]))");

    private static final String IRI =
            "[" + GOOD_IRI_CHAR + "]([" + GOOD_IRI_CHAR + "\\-]{0,61}[" + GOOD_IRI_CHAR + "]){0,1}";

    private static final String GOOD_GTLD_CHAR = "a-zA-Z\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF";
    private static final String GTLD = "[" + GOOD_GTLD_CHAR + "]{2,63}";
    private static final String HOST_NAME = "(" + IRI + "\\.)+" + GTLD;

    private static final Pattern DOMAIN_NAME =
            Pattern.compile("(" + HOST_NAME + "|" + IP_ADDRESS + ")");

    private static final Pattern LIKELY_EXCEPTION_LOG =
            Pattern.compile("\\sat\\sorg\\.chromium\\.[^ ]+.");

    private static final Pattern WEB_URL =
            Pattern.compile("(?:\\b|^)((?:(http|https|Http|Https|rtsp|Rtsp):"
                    + "\\/\\/(?:(?:[a-zA-Z0-9\\$\\-\\_\\.\\+\\!\\*\\'\\(\\)"
                    + "\\,\\;\\?\\&\\=]|(?:\\%[a-fA-F0-9]{2})){1,64}(?:\\:(?:[a-zA-Z0-9\\$\\-\\_"
                    + "\\.\\+\\!\\*\\'\\(\\)\\,\\;\\?\\&\\=]|(?:\\%[a-fA-F0-9]{2})){1,25})?\\@)?)?"
                    + "(?:" + DOMAIN_NAME + ")"
                    + "(?:\\:\\d{1,5})?)"
                    + "(\\/(?:(?:[" + GOOD_IRI_CHAR + "\\;\\/\\?\\:\\@\\&\\=\\#\\~"
                    + "\\-\\.\\+\\!\\*\\'\\(\\)\\,\\_])|(?:\\%[a-fA-F0-9]{2}))*)?"
                    + "(?:\\b|$)");

    @VisibleForTesting
    protected static final String BEGIN_MICRODUMP = "-----BEGIN BREAKPAD MICRODUMP-----";
    @VisibleForTesting
    protected static final String END_MICRODUMP = "-----END BREAKPAD MICRODUMP-----";
    @VisibleForTesting
    protected static final String SNIPPED_MICRODUMP =
            "-----SNIPPED OUT BREAKPAD MICRODUMP FOR THIS CRASH-----";

    @VisibleForTesting
    protected static final String IP_ELISION = "1.2.3.4";

    @VisibleForTesting
    protected static final String MAC_ELISION = "01:23:45:67:89:AB";

    @VisibleForTesting
    protected static final String CONSOLE_ELISION = "[ELIDED:CONSOLE(0)] ELIDED CONSOLE MESSAGE";

    private static final Pattern MAC_ADDRESS =
            Pattern.compile("([0-9a-fA-F]{2}[-:]+){5}[0-9a-fA-F]{2}");

    private static final Pattern CONSOLE_MSG = Pattern.compile("\\[\\w*:CONSOLE.*\\].*");

    private static final String[] CHROME_NAMESPACE = new String[] {"org.chromium.", "com.google."};

    private static final String[] SYSTEM_NAMESPACE = new String[] {"android.accessibilityservice",
            "android.accounts", "android.animation", "android.annotation", "android.app",
            "android.appwidget", "android.bluetooth", "android.content", "android.database",
            "android.databinding", "android.drm", "android.gesture", "android.graphics",
            "android.hardware", "android.inputmethodservice", "android.location", "android.media",
            "android.mtp", "android.net", "android.nfc", "android.opengl", "android.os",
            "android.preference", "android.print", "android.printservice", "android.provider",
            "android.renderscript", "android.sax", "android.security", "android.service",
            "android.speech", "android.support", "android.system", "android.telecom",
            "android.telephony", "android.test", "android.text", "android.transition",
            "android.util", "android.view", "android.webkit", "android.widget", "com.android.",
            "dalvik.", "java.", "javax.", "org.apache.", "org.json.", "org.w3c.dom.", "org.xml.",
            "org.xmlpull."};

    private final File mMinidumpFile;

    /**
     * @param minidump The minidump file that needs logcat output to be attached.
     */
    public LogcatExtractionRunnable(File minidump) {
        mMinidumpFile = minidump;
    }

    @Override
    public void run() {
        uploadMinidumpWithLogcat(false);
    }

    /**
     * @param uploadNow If this flag is set to true, we will upload the minidump immediately,
     * otherwise the upload is controlled by the job scheduler.
     */
    public void uploadMinidumpWithLogcat(boolean uploadNow) {
        Log.i(TAG, "Trying to extract logcat for minidump %s.", mMinidumpFile.getName());
        File fileToUpload = attachLogcatToMinidump();

        // Regardless of success, initiate the upload. That way, even if there are errors augmenting
        // the minidump with logcat data, the service can still upload the unaugmented minidump.
        try {
            if (uploadNow) {
                MinidumpUploadService.tryUploadCrashDumpNow(fileToUpload);
            } else if (MinidumpUploadService.shouldUseJobSchedulerForUploads()) {
                MinidumpUploadService.scheduleUploadJob();
            } else {
                MinidumpUploadService.tryUploadCrashDump(fileToUpload);
            }
        } catch (SecurityException e) {
            // For KitKat and below, there was a framework bug which causes us to not be able to
            // find our own crash uploading service. Ignore a SecurityException here on older
            // OS versions since the crash will eventually get uploaded on next start.
            // crbug/542533
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                Log.w(TAG, e.toString());
                if (!uploadNow) throw e;
            }
        }
    }

    @VisibleForTesting
    public File attachLogcatToMinidump() {
        CrashFileManager fileManager =
                new CrashFileManager(ContextUtils.getApplicationContext().getCacheDir());
        File fileToUpload = mMinidumpFile;
        try {
            List<String> logcat = getElidedLogcat();
            fileToUpload = new MinidumpLogcatPrepender(fileManager, mMinidumpFile, logcat).run();
            Log.i(TAG, "Succeeded extracting logcat to %s.", fileToUpload.getName());
        } catch (IOException | InterruptedException e) {
            Log.w(TAG, e.toString());
        }
        return fileToUpload;
    }

    private List<String> getElidedLogcat() throws IOException, InterruptedException {
        List<String> rawLogcat = getLogcat();
        return Collections.unmodifiableList(elideLogcat(rawLogcat));
    }

    @VisibleForTesting
    protected List<String> getLogcat() throws IOException, InterruptedException {
        // Grab the last lines of the logcat output, with a generous buffer to compensate for any
        // microdumps that might be in the logcat output, since microdumps are stripped in the
        // extraction code. Note that the repeated check of the process exit value is to account for
        // the fact that the process might not finish immediately.  And, it's not appropriate to
        // call p.waitFor(), because this call will block *forever* if the process's output buffer
        // fills up.
        Process p = Runtime.getRuntime().exec("logcat -d");
        BufferedReader reader = new BufferedReader(new InputStreamReader(p.getInputStream()));
        LinkedList<String> rawLogcat = new LinkedList<>();
        Integer exitValue = null;
        try {
            while (exitValue == null) {
                String logLn;
                while ((logLn = reader.readLine()) != null) {
                    rawLogcat.add(logLn);
                    if (rawLogcat.size() > LOGCAT_SIZE * 4) {
                        rawLogcat.removeFirst();
                    }
                }

                try {
                    exitValue = p.exitValue();
                } catch (IllegalThreadStateException itse) {
                    Thread.sleep(HALF_SECOND);
                }
            }
        } finally {
            reader.close();
        }

        if (exitValue != 0) {
            String msg = "Logcat failed: " + exitValue;
            Log.w(TAG, msg);
            throw new IOException(msg);
        }

        return trimLogcat(rawLogcat, LOGCAT_SIZE);
    }

    /**
     * Extracts microdump-free logcat for more informative crash reports. Returns the most recent
     * lines that are likely to be relevant to the crash, which are either the lines leading up to a
     * microdump if a microdump is present, or just the final lines of the logcat if no microdump is
     * present.
     *
     * @param rawLogcat The last lines of the raw logcat file, with sufficient history to allow a
     *     sufficient history even after trimming.
     * @param maxLines The maximum number of lines logcat extracts from minidump.
     *
     * @return Logcat up to specified length as a list of strings.
     */
    @VisibleForTesting
    protected static List<String> trimLogcat(List<String> rawLogcat, int maxLines) {
        // Trim off the last microdump, and anything after it.
        for (int i = rawLogcat.size() - 1; i >= 0; i--) {
            if (rawLogcat.get(i).contains(BEGIN_MICRODUMP)) {
                rawLogcat = rawLogcat.subList(0, i);
                rawLogcat.add(SNIPPED_MICRODUMP);
                break;
            }
        }

        // Trim down the remainder to only contain the most recent lines. Thus, if the original
        // input contained a microdump, the result contains the most recent lines before the
        // microdump, which are most likely to be relevant to the crash.  If there is no microdump
        // in the raw logcat, then just hope that the last lines in the dump are relevant.
        if (rawLogcat.size() > maxLines) {
            rawLogcat = rawLogcat.subList(rawLogcat.size() - maxLines, rawLogcat.size());
        }
        return rawLogcat;
    }

    @VisibleForTesting
    protected static List<String> elideLogcat(List<String> rawLogcat) {
        List<String> elided = new ArrayList<String>(rawLogcat.size());
        for (String ln : rawLogcat) {
            ln = elideEmail(ln);
            ln = elideUrl(ln);
            ln = elideIp(ln);
            ln = elideMac(ln);
            ln = elideConsole(ln);
            elided.add(ln);
        }
        return elided;
    }

    /**
     * Elides any emails in the specified {@link String} with
     * {@link #EMAIL_ELISION}.
     *
     * @param original String potentially containing emails.
     * @return String with elided emails.
     */
    @VisibleForTesting
    protected static String elideEmail(String original) {
        return Patterns.EMAIL_ADDRESS.matcher(original).replaceAll(EMAIL_ELISION);
    }

    /**
     * Elides any URLs in the specified {@link String} with
     * {@link #URL_ELISION}.
     *
     * @param original String potentially containing URLs.
     * @return String with elided URLs.
     */
    @VisibleForTesting
    protected static String elideUrl(String original) {
        // Url-matching is fussy. If something looks like an exception message, just return.
        if (LIKELY_EXCEPTION_LOG.matcher(original).find()) return original;
        StringBuilder buffer = new StringBuilder(original);
        Matcher matcher = WEB_URL.matcher(buffer);
        int start = 0;
        while (matcher.find(start)) {
            start = matcher.start();
            int end = matcher.end();
            String url = buffer.substring(start, end);
            if (!likelyToBeChromeNamespace(url) && !likelyToBeSystemNamespace(url)) {
                buffer.replace(start, end, URL_ELISION);
                end = start + URL_ELISION.length();
                matcher = WEB_URL.matcher(buffer);
            }
            start = end;
        }
        return buffer.toString();
    }

    public static boolean likelyToBeChromeNamespace(String url) {
        for (String ns : CHROME_NAMESPACE) {
            if (url.startsWith(ns)) {
                return true;
            }
        }
        return false;
    }

    public static boolean likelyToBeSystemNamespace(String url) {
        for (String ns : SYSTEM_NAMESPACE) {
            if (url.startsWith(ns)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Elides any IP addresses in the specified {@link String} with
     * {@link #IP_ELISION}.
     *
     * @param original String potentially containing IPs.
     * @return String with elided IPs.
     */
    @VisibleForTesting
    protected static String elideIp(String original) {
        return Patterns.IP_ADDRESS.matcher(original).replaceAll(IP_ELISION);
    }

    /**
     * Elides any MAC addresses in the specified {@link String} with
     * {@link #MAC_ELISION}.
     *
     * @param original String potentially containing MACs.
     * @return String with elided MACs.
     */
    @VisibleForTesting
    protected static String elideMac(String original) {
        return MAC_ADDRESS.matcher(original).replaceAll(MAC_ELISION);
    }

    /**
     * Elides any console messages in the specified {@link String} with
     * {@link #CONSOLE_ELISION}.
     *
     * @param original String potentially containing console messages.
     * @return String with elided console messages.
     */
    @VisibleForTesting
    protected static String elideConsole(String original) {
        return CONSOLE_MSG.matcher(original).replaceAll(CONSOLE_ELISION);
    }
}
