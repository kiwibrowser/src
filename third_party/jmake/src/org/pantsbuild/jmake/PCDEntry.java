/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.File;

/**
 * An instance of this class represents an entry in the Project Class Directory.
 *
 * @author Misha Dmitriev
 *  29 March 2002
 */
public class PCDEntry {
    // Class versions compare results

    static final int CV_UNCHECKED = 0;
    static final int CV_COMPATIBLE = 1;
    static final int CV_INCOMPATIBLE = 2;
    static final int CV_DELETED = 3;
    static final int CV_NEW = 4;
    static final int CV_NEWER_FOUND_NEARER = 5;
    String className;           // Dots are replaced with slashes for convenience
    transient String classFileFullPath;
    String javaFileFullPath;
    long oldClassFileLastModified;
    transient long newClassFileLastModified;
    long oldClassFileFingerprint;
    transient long newClassFileFingerprint;
    ClassInfo oldClassInfo;
    transient ClassInfo newClassInfo;
    transient int checkResult;         // Reflects the result of class version comparison
    transient boolean checked;             // Mark entries for classes that have been checked and found existing.
    // It helps to detect double entries for the same class in the project file list,
    // and also not to confuse them with the case when a .java source for a class is moved.

    /** This constructor is called to initialize a record for a class that has just been added to the project. */
    public PCDEntry(String className,
            String javaFileFullPath,
            String classFileFullPath,
            long classFileLastModified,
            long classFileFingerprint,
            ClassInfo classInfo) {
        this.className = className;
        this.classFileFullPath = classFileFullPath;
        this.javaFileFullPath = javaFileFullPath;
        this.oldClassFileLastModified = this.newClassFileLastModified =
                classFileLastModified;
        this.oldClassFileFingerprint = this.newClassFileFingerprint =
                classFileFingerprint;
        this.newClassInfo = classInfo;
        checked = true;
    }

    /**
     * This constructor is called to initialize a record for a class that
     * exists at least in the previous version of the project.
     */
    public PCDEntry(String className,
            String javaFileFullPath,
            long classFileLastModified,
            long classFileFingerprint,
            ClassInfo classInfo) {
        this.className = className;
        this.javaFileFullPath = javaFileFullPath;
        this.oldClassFileLastModified = classFileLastModified;
        this.oldClassFileFingerprint = classFileFingerprint;
        this.oldClassInfo = classInfo;
    }

    // Debugging
    public String toString() {
        return "className = " + className +
                "; classFileFullPath = " + classFileFullPath +
                "; javaFileFullPath = " + javaFileFullPath;
    }

    /**
     * Returns the name of the class that corresponds to the file name, i.e. the public class
     */
    private String getExpectedClassName() {
        File path = new File(javaFileFullPath);
        int index = -1;
        do {
            index = className.indexOf('/', index + 1);
            path = path.getParentFile();
        } while (index != -1);
        String pathString = path.toString();
        if (!pathString.endsWith("/"))
            pathString += "/";
        // It is assumed that the javaFileFillPath ends with .java
        int javaPathWithoutSuffix = javaFileFullPath.length() - 5;
        return javaFileFullPath.substring(pathString.length(),
                                          javaPathWithoutSuffix);
    }

    /**
     * A class that neither has the same name as the java file, nor an inner class, is
     * package-private.
     */
    public boolean isPackagePrivateClass() {
        String expectedClassName = getExpectedClassName();

        return !(className.equals(expectedClassName)
                 || (className.startsWith(expectedClassName)
                     && className.charAt(expectedClassName.length()) == '$'));
    }
}
