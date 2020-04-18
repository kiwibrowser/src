/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

/**
 * This class is used as a wrapper for a number of exceptions that are thrown by jmake. Its
 * only purpose is to help avoid using endless "throws" clauses in the code.
 *
 * @author  Misha Dmitriev
 *  12 November 2001
 */
public class PrivateException extends RuntimeException {

    private static final long serialVersionUID = 1L;
    private Throwable originalException;

    public PrivateException(Throwable e) {
        originalException = e;
    }

    public Throwable getOriginalException() {
        return originalException;
    }
}
