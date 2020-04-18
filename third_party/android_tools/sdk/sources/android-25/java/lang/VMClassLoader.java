/*
 * Copyright (C) 2007 The Android Open Source Project
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

package java.lang;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.net.URLStreamHandler;
import java.util.ArrayList;
import java.util.List;
import libcore.io.ClassPathURLStreamHandler;

class VMClassLoader {

    private static final ClassPathURLStreamHandler[] bootClassPathUrlHandlers;
    static {
        bootClassPathUrlHandlers = createBootClassPathUrlHandlers();
    }

    /**
     * Creates an array of ClassPathURLStreamHandler objects for handling resource loading from
     * the boot classpath.
     */
    private static ClassPathURLStreamHandler[] createBootClassPathUrlHandlers() {
        String[] bootClassPathEntries = getBootClassPathEntries();
        ArrayList<String> zipFileUris = new ArrayList<String>(bootClassPathEntries.length);
        ArrayList<URLStreamHandler> urlStreamHandlers =
                new ArrayList<URLStreamHandler>(bootClassPathEntries.length);
        for (String bootClassPathEntry : bootClassPathEntries) {
            try {
                String entryUri = new File(bootClassPathEntry).toURI().toString();

                // We assume all entries are zip or jar files.
                URLStreamHandler urlStreamHandler =
                        new ClassPathURLStreamHandler(bootClassPathEntry);
                zipFileUris.add(entryUri);
                urlStreamHandlers.add(urlStreamHandler);
            } catch (IOException e) {
                // Skip it
                System.logE("Unable to open boot classpath entry: " + bootClassPathEntry, e);
            }
        }
        return urlStreamHandlers.toArray(new ClassPathURLStreamHandler[urlStreamHandlers.size()]);
    }

    /**
     * Get a resource from a file in the bootstrap class path.
     *
     * We assume that the bootclasspath can't change once the VM has started.
     * This assumption seems to be supported by the spec.
     */
    static URL getResource(String name) {
        for (ClassPathURLStreamHandler urlHandler : bootClassPathUrlHandlers) {
            URL url = urlHandler.getEntryUrlOrNull(name);
            if (url != null) {
                return url;
            }
        }
        return null;
    }

    /*
     * Get an enumeration with all matching resources.
     */
    static List<URL> getResources(String name) {
        ArrayList<URL> list = new ArrayList<URL>();
        for (ClassPathURLStreamHandler urlHandler : bootClassPathUrlHandlers) {
            URL url = urlHandler.getEntryUrlOrNull(name);
            if (url != null) {
                list.add(url);
            }
        }
        return list;
    }

    native static Class findLoadedClass(ClassLoader cl, String name);

    /**
     * Boot class path manipulation, for getResources().
     */
    native private static String[] getBootClassPathEntries();

}
