/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

/**
 * This class is a wrapper for a number of nested classes. They define exceptions that may be thrown
 * by the <b>jmake</b> code. These exceptions are caught in the <code>Main.mainExternal</code> method,
 * or they may be caught and handled by an application invoking <b>jmake</b> programmatically
 * through <code>Main.mainProgrammatic</code> method.
 *
 * @see Main#mainExternal(String[])
 * @see Main#mainProgrammatic(String[])
 *
 * @author  Misha Dmitriev
 *  17 January 2003
 */
public class PublicExceptions {

    /**
     * This exception is thrown whenever there is any problem initializing or calling the compiler, or
     * when the compiler itself reports compilation errors. Depending on the nature of the problem, an
     * instance of this class is initialized with, and then returns, an original exception signalling
     * the problem, or the compiler application exit code.
     */
    public static class CompilerInteractionException extends Exception {

        private static final long serialVersionUID = 1L;
        private Exception originalException;
        private int compilerExitCode;

        /**
         * Initialize an instance of this exception with an error message and either an original
         * exception or a compiler exit code.
         */
        CompilerInteractionException(String msg, Exception originalException, int compilerExitCode) {
            super(msg);
            this.originalException = originalException;
            this.compilerExitCode = compilerExitCode;
        }

        /**
         * Get the original exception that caused this exception. <code>null</code> is returned if there
         * were no original exception. In that case, there was a compilation error and the compiler has
         * returned with the exit code that can be obtained using <code>getCompilerExitCode</code> method.
         *
         * @see #getCompilerExitCode()
         */
        public Exception getOriginalException() {
            return originalException;
        }

        /**
         * Get the compiler exit code. If <code>0</code> is returned, this indicates that an exception was
         * thrown during compiler initialization, or inside the compiler itself. Use <code>getOriginalExcepion</code>
         * method to obtain that exception.
         *
         * @see #getOriginalException
         */
        public int getCompilerExitCode() {
            return compilerExitCode;
        }
    }

    /** Exception signalling a problem with reading project database file. */
    public static class PDBCorruptedException extends Exception {

        private static final long serialVersionUID = 1L;

        PDBCorruptedException(String msg) {
            super(msg);
        }
    }

    /** Exception signalling a problem when parsing a class file */
    public static class ClassFileParseException extends Exception {

        private static final long serialVersionUID = 1L;

        ClassFileParseException(String msg) {
            super(msg);
        }
    }

    /** Exception signalling that <b>jmake</b> was not requested to do any real work. */
    public static class NoActionRequestedException extends Exception {

        private static final long serialVersionUID = 1L;
    }

    /** Exception signalling that an invalid command line option has been passed to jmake. */
    public static class InvalidCmdOptionException extends Exception {

        private static final long serialVersionUID = 1L;

        InvalidCmdOptionException(String msg) {
            super(msg);
        }
    }

    /**
     * Exception signalling a problem (typically an I/O exception) when parsing a command line file
     * passed to <b>jmake</b>.
     */
    public static class CommandFileReadException extends Exception {

        private static final long serialVersionUID = 1L;

        CommandFileReadException(String msg) {
            super(msg);
        }
    }

    /**
     * Exception signalling that for some class the deduced name (that is, name based on the source file name and
     * directory) and the real name (the one read from the class file) are different.
     */
    public static class ClassNameMismatchException extends Exception {

        private static final long serialVersionUID = 1L;

        ClassNameMismatchException(String msg) {
            super(msg);
        }
    }

    /** Exception thrown if any specified source file has an invalid extension (not <code>.java</code> or <code>.jar</code>). */
    public static class InvalidSourceFileExtensionException extends Exception {

        private static final long serialVersionUID = 1L;

        InvalidSourceFileExtensionException(String msg) {
            super(msg);
        }
    }

    /** Exception thrown if a dependence of a class in a <code>JAR</code> file on a class with a <code>.java</code> source is detected. */
    public static class JarDependsOnSourceException extends Exception {

        private static final long serialVersionUID = 1L;

        JarDependsOnSourceException(String msg) {
            super(msg);
        }
    }

    /** Exception thrown if more than one entry for the same class is detected in the project */
    public static class DoubleEntryException extends Exception {

        private static final long serialVersionUID = 1L;

        DoubleEntryException(String msg) {
            super(msg);
        }
    }

    /** Exception thrown if an internal problem that should never happen is detected. */
    public static class InternalException extends Exception {

        private static final long serialVersionUID = 1L;

        InternalException(String msg) {
            super(msg);
        }
    }
}
