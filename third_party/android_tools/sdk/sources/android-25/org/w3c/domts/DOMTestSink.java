/*
 * Copyright (c) 2001-2004 World Wide Web Consortium,
 * (Massachusetts Institute of Technology, Institut National de
 * Recherche en Informatique et en Automatique, Keio University). All
 * Rights Reserved. This program is distributed under the W3C's Software
 * Intellectual Property License. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 * See W3C License http://www.w3.org/Consortium/Legal/ for more details.
 */

/*
  $Log: DOMTestSink.java,v $
  Revision 1.2  2004/03/11 01:44:21  dom-ts-4
  Checkstyle fixes (bug 592)

  Revision 1.1  2001/07/23 04:52:20  dom-ts-4
  Initial test running using JUnit.

 */

package org.w3c.domts;

public interface DOMTestSink {
  public void addTest(Class test);
}
