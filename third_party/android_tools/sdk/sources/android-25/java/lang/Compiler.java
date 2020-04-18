/*
 * Copyright (C) 2014 The Android Open Source Project
 * Copyright (c) 1995, 2008, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package java.lang;

/**
 * Does nothing on Android.
 */
public final class Compiler  {
    /**
     * Prevent this class from being instantiated.
     */
    private Compiler() {}               // don't make instances

    /**
     * Compiles the specified class using the JIT compiler and indicates if
     * compilation has been successful. Does nothing and returns false on
     * Android.
     *
     * @param classToCompile
     *            java.lang.Class the class to JIT compile
     * @return {@code true} if the compilation has been successful;
     *         {@code false} if it has failed or if there is no JIT compiler
     *         available.
     */
    public static boolean compileClass(Class<?> classToCompile) {
        return false;
    }

    /**
     * Compiles all classes whose name matches the specified name using the JIT
     * compiler and indicates if compilation has been successful. Does nothing
     * and returns false on Android.
     *
     * @param nameRoot
     *            the string to match class names with.
     * @return {@code true} if the compilation has been successful;
     *         {@code false} if it has failed or if there is no JIT compiler
     *         available.
     */
    public static boolean compileClasses(String nameRoot) {
        return false;
    }

    /**
     * Executes an operation according to the specified command object. This
     * method is the low-level interface to the JIT compiler. It may return any
     * object or {@code null} if no JIT compiler is available. Returns null
     * on Android, whether or not the system has a JIT.
     *
     * @param cmd
     *            the command object for the JIT compiler.
     * @return the result of executing command or {@code null}.
     */
    public static Object command(Object cmd) {
        return null;
    }

    /**
     * Enables the JIT compiler. Does nothing on Android.
     */
    public static void enable() {

    }

    /**
     * Disables the JIT compiler. Does nothing on Android.
     */
    public static void disable() {

    }
}
