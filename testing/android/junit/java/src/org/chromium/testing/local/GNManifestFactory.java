// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.testing.local;

import org.robolectric.annotation.Config;
import org.robolectric.internal.ManifestFactory;
import org.robolectric.internal.ManifestIdentifier;
import org.robolectric.manifest.AndroidManifest;
import org.robolectric.res.Fs;
import org.robolectric.res.FsFile;
import org.robolectric.res.ResourcePath;

import java.io.File;
import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Class that manages passing Android manifest information to Robolectric.
 */
public class GNManifestFactory implements ManifestFactory {
    private static final String CHROMIUM_MANIFEST_PATH = "chromium.robolectric.manifest";
    private static final String CHROMIUM_RES_DIRECTORIES = "chromium.robolectric.resource.dirs";

    @Override
    public ManifestIdentifier identify(Config config) {
        if (config.resourceDir() != null
                && !config.resourceDir().equals(Config.DEFAULT_RES_FOLDER)) {
            throw new RuntimeException("Resource dirs should be generated automatically by GN. "
                    + "Make sure you specify the correct app package_name in the GN build file. "
                    + "Make sure you run the tests using the generated run_<test name> scripts.");
        }

        if (config.manifest() != null && !config.manifest().equals(Config.NONE)) {
            throw new RuntimeException("Specify manifest path in GN build file.");
        }

        return new ManifestIdentifier(null, null, null, config.packageName(), null);
    }

    @Override
    public AndroidManifest create(ManifestIdentifier manifestIdentifier) {
        String manifestPath = System.getProperty(CHROMIUM_MANIFEST_PATH);
        String resourceDirs = System.getProperty(CHROMIUM_RES_DIRECTORIES);

        final List<FsFile> resourceDirsList = new ArrayList<FsFile>();
        if (resourceDirs != null) {
            for (String resourceDir : resourceDirs.split(":")) {
                try {
                    resourceDirsList.add(Fs.fromURL(new File(resourceDir).toURI().toURL()));
                } catch (MalformedURLException e) {
                }
            }
        }

        FsFile manifestFile = null;
        if (manifestPath != null) {
            try {
                manifestFile = Fs.fromURL(new File(manifestPath).toURI().toURL());
            } catch (MalformedURLException e) {
            }
        }

        return new AndroidManifest(manifestFile, null, null, manifestIdentifier.getPackageName()) {
            @Override
            public List<ResourcePath> getIncludedResourcePaths() {
                List<ResourcePath> paths = super.getIncludedResourcePaths();
                for (FsFile resourceDir : resourceDirsList) {
                    paths.add(new ResourcePath(getRClass(), resourceDir, getAssetsDirectory()));
                }
                return paths;
            }
        };
    }
}
