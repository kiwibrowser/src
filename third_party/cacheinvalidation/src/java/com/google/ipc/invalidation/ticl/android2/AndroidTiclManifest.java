/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ipc.invalidation.ticl.android2;

import com.google.ipc.invalidation.util.Preconditions;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

import java.util.HashMap;
import java.util.Map;


/**
 * Interface to the {@code AndroidManifest.xml} that provides access to the configuration data
 * required by the Android Ticl.
 *
 */
public class AndroidTiclManifest {

  /**
   * Cache of {@link ApplicationMetadata} to avoid repeatedly scanning manifest. The key is the
   * package name for the context.
   */
  private static final Map<String, ApplicationMetadata> applicationMetadataCache =
      new HashMap<String, ApplicationMetadata>();

  /** Application metadata from the Android manifest. */
  private final ApplicationMetadata metadata;

  public AndroidTiclManifest(Context context) {
    metadata = createApplicationMetadata(Preconditions.checkNotNull(context));
  }

  /** Returns the name of the class implementing the Ticl service. */
  public String getTiclServiceClass() {
    return metadata.ticlServiceClass;
  }

  /** Returns the name of the class on which listener events will be invoked. */
  String getListenerClass() {
    return metadata.listenerClass;
  }

  /** Returns the name of the class implementing the invalidation listener intent service. */
  public String getListenerServiceClass() {
    return metadata.listenerServiceClass;
  }

  /**
   * Returns the name of the class implementing the background invalidation listener intent service.
   */
  String getBackgroundInvalidationListenerServiceClass() {
    return metadata.backgroundInvalidationListenerServiceClass;
  }

  
  public String getGcmUpstreamServiceClass() {
    return metadata.gcmUpstreamServiceClass;
  }

  /**
   * If it has not already been cached for the given {@code context}, creates and caches application
   * metadata from the manifest.
   */
  private static ApplicationMetadata createApplicationMetadata(Context context) {
    synchronized (applicationMetadataCache) {
      String packageName = context.getPackageName();
      ApplicationMetadata metadata = applicationMetadataCache.get(packageName);
      if (metadata == null) {
        metadata = new ApplicationMetadata(context);
        applicationMetadataCache.put(packageName, metadata);
      }
      return metadata;
    }
  }

  /** Application metadata for a specific context. */
  private static final class ApplicationMetadata {
    /**
     * Name of the {@code <application>} metadata element whose value gives the Java class that
     * implements the application {@code InvalidationListener}. Must be set if
     * {@link #LISTENER_SERVICE_NAME_KEY} is not set.
     */
    private static final String LISTENER_NAME_KEY = "ipc.invalidation.ticl.listener_class";

    /**
     * Name of the {@code <application>} metadata element whose value gives the Java class that
     * implements the Ticl service. Should only be set in tests.
     */
    private static final String TICL_SERVICE_NAME_KEY = "ipc.invalidation.ticl.service_class";

    /**
     * Name of the {@code <application>} metadata element whose value gives the Java class that
     * implements the application's invalidation listener intent service.
     */
    private static final String LISTENER_SERVICE_NAME_KEY =
        "ipc.invalidation.ticl.listener_service_class";

    /**
     * Name of the {@code <application>} metadata element whose value gives the Java class that
     * implements the application's background invalidation listener intent service.
     */
    private static final String BACKGROUND_INVALIDATION_LISTENER_SERVICE_NAME_KEY =
        "ipc.invalidation.ticl.background_invalidation_listener_service_class";

    /**
     * Name of the {@code <application>} metadata element whose value gives the Java class that
     * implements the application's gcm upstream sender intent service.
     */
    private static final String GCM_UPSTREAM_SERVICE_NAME_KEY =
        "ipc.invalidation.ticl.gcm_upstream_service_class";

    /** Default values returned if not overriden by the manifest file. */
    private static final Map<String, String> DEFAULTS = new HashMap<String, String>();
    static {
        DEFAULTS.put(TICL_SERVICE_NAME_KEY,
            "com.google.ipc.invalidation.ticl.android2.TiclService");
        DEFAULTS.put(LISTENER_NAME_KEY, "");
        DEFAULTS.put(LISTENER_SERVICE_NAME_KEY,
            "com.google.ipc.invalidation.ticl.android2.AndroidInvalidationListenerStub");
        DEFAULTS.put(BACKGROUND_INVALIDATION_LISTENER_SERVICE_NAME_KEY, null);
        DEFAULTS.put(GCM_UPSTREAM_SERVICE_NAME_KEY, null);
    }

    private final String ticlServiceClass;
    private final String listenerClass;
    private final String listenerServiceClass;
    private final String backgroundInvalidationListenerServiceClass;
    private final String gcmUpstreamServiceClass;

    ApplicationMetadata(Context context) {
      ApplicationInfo appInfo;
      try {
        // Read metadata from manifest.xml
        appInfo = context.getPackageManager()
            .getApplicationInfo(context.getPackageName(), PackageManager.GET_META_DATA);
      } catch (NameNotFoundException exception) {
        throw new RuntimeException("Cannot read own application info", exception);
      }
      ticlServiceClass = readApplicationMetadata(appInfo, TICL_SERVICE_NAME_KEY);
      listenerClass = readApplicationMetadata(appInfo, LISTENER_NAME_KEY);
      listenerServiceClass = readApplicationMetadata(appInfo, LISTENER_SERVICE_NAME_KEY);
      backgroundInvalidationListenerServiceClass =
          readApplicationMetadata(appInfo, BACKGROUND_INVALIDATION_LISTENER_SERVICE_NAME_KEY);
      gcmUpstreamServiceClass = readApplicationMetadata(appInfo, GCM_UPSTREAM_SERVICE_NAME_KEY);
    }

    /**
     * Returns the metadata-provided value for {@code key} in {@code appInfo} if one
     * exists, or the value from {@link #DEFAULTS} if one does not.
     */
    private static String readApplicationMetadata(ApplicationInfo appInfo, String key) {
      String value = null;
      if (appInfo.metaData != null) {
        value = appInfo.metaData.getString(key);
      }
      // Return the manifest value if present or the default value if not.
      return (value != null) ? value : DEFAULTS.get(key);
    }
  }
}
