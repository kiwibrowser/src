package org.robolectric.internal.dependency;

import java.net.URL;

/**
 * Stub implementation of MavenDependencyResolver.
 *
 * This class is not used by Robolectric in Chromium. Compiling against a stub implementation
 * allows us to avoid needing to have some various Maven third-party libraries.
 */
public class MavenDependencyResolver implements DependencyResolver {
    public MavenDependencyResolver() {}

    public MavenDependencyResolver(String repositoryUrl, String repositoryId) {}

    @Override
    public URL getLocalArtifactUrl(DependencyJar dependency) {
        return null;
    }
}
