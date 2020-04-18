// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.os.Bundle;

import org.chromium.base.Log;

import java.util.List;

/**
 * {@link PepperPluginManager} collects meta data about plugins from preloaded android apps
 * that reply to PEPPERPLUGIN intent query.
 */
public class PepperPluginManager {

    private static final String TAG = "cr.PepperPluginManager";

    /**
     * Service Action: A plugin wishes to be loaded in the ContentView must
     * provide {@link android.content.IntentFilter IntentFilter} that accepts
     * this action in its AndroidManifest.xml.
     */
    public static final String PEPPER_PLUGIN_ACTION = "org.chromium.intent.PEPPERPLUGIN";
    public static final String PEPPER_PLUGIN_ROOT = "/system/lib/pepperplugin/";

    // A plugin will specify the following fields in its AndroidManifest.xml.
    private static final String FILENAME = "filename";
    private static final String MIMETYPE = "mimetype";
    private static final String NAME = "name";
    private static final String DESCRIPTION = "description";
    private static final String VERSION = "version";

    private static String getPluginDescription(Bundle metaData) {
        // Find the name of the plugin's shared library.
        String filename = metaData.getString(FILENAME);
        if (filename == null || filename.isEmpty()) {
            return null;
        }
        // Find the mimetype of the plugin. Flash is handled in getFlashPath.
        String mimetype = metaData.getString(MIMETYPE);
        if (mimetype == null || mimetype.isEmpty()) {
            return null;
        }
        // Assemble the plugin info, according to the format described in
        // pepper_plugin_list.cc.
        // (eg. path<#name><#description><#version>;mimetype)
        StringBuilder plugin = new StringBuilder(PEPPER_PLUGIN_ROOT);
        plugin.append(filename);

        // Find the (optional) name/description/version of the plugin.
        String name = metaData.getString(NAME);
        String description = metaData.getString(DESCRIPTION);
        String version = metaData.getString(VERSION);

        if (name != null && !name.isEmpty()) {
            plugin.append("#");
            plugin.append(name);
            if (description != null && !description.isEmpty()) {
                plugin.append("#");
                plugin.append(description);
                if (version != null && !version.isEmpty()) {
                    plugin.append("#");
                    plugin.append(version);
                }
            }
        }
        plugin.append(';');
        plugin.append(mimetype);

        return plugin.toString();
    }

    /**
     * Collects information about installed plugins and returns a plugin description
     * string, which will be appended to for command line to load plugins.
     *
     * @param context Android context
     * @return        Description string for plugins
     */
    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("WrongConstant")
    public static String getPlugins(final Context context) {
        StringBuilder ret = new StringBuilder();
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> plugins = pm.queryIntentServices(
                new Intent(PEPPER_PLUGIN_ACTION),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);
        for (ResolveInfo info : plugins) {
            // Retrieve the plugin's service information.
            ServiceInfo serviceInfo = info.serviceInfo;
            if (serviceInfo == null || serviceInfo.metaData == null
                    || serviceInfo.packageName == null) {
                Log.e(TAG, "Can't get service information from %s", info);
                continue;
            }

            // Retrieve the plugin's package information.
            PackageInfo pkgInfo;
            try {
                pkgInfo = pm.getPackageInfo(serviceInfo.packageName, 0);
            } catch (NameNotFoundException e) {
                Log.e(TAG, "Can't find plugin: %s", serviceInfo.packageName);
                continue;
            }
            if (pkgInfo == null
                    || (pkgInfo.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0) {
                continue;
            }
            Log.i(TAG, "The given plugin package is preloaded: %s", serviceInfo.packageName);

            String plugin = getPluginDescription(serviceInfo.metaData);
            if (plugin == null) {
                continue;
            }
            if (ret.length() > 0) {
                ret.append(',');
            }
            ret.append(plugin);
        }
        return ret.toString();
    }
}
