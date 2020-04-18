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
  $Log: DOMTestSuite.java,v $
  Revision 1.4  2004/03/11 01:44:21  dom-ts-4
  Checkstyle fixes (bug 592)

  Revision 1.3  2002/02/03 04:22:35  dom-ts-4
  DOM4J and Batik support added.
  Rework of parser settings

  Revision 1.2  2001/07/23 04:52:20  dom-ts-4
  Initial test running using JUnit.

 */

package org.w3c.domts;

/**
 * Abstract base class for all test suites
 * (that is any collection of DOMTest's)
 *
 * @author Curt Arnold
 */
public abstract class DOMTestSuite
    extends DOMTest {
  /**
   * This constructor is used for suites that
   * assert one or more implementation attributes or
   * features.  setLibrary should be called before
   * the completion of the constructor in the derived class.
   */
  protected DOMTestSuite() {
  }

  /**
   * This constructor is used for suites that make no
   * additional requirements on the parser configuration.
   * @param factory may not be null
   */
  protected DOMTestSuite(DOMTestDocumentBuilderFactory factory) {
    super(factory);
  }

  /**
   * Adds a test to the test suite.  This method can
   * only be run after the test suite has been attached
   * to a test framework since each framework implements
   * test suites in different manners.
   */
  abstract public void build(DOMTestSink sink);
}
