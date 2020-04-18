/*
 * Copyright (c) 2004 World Wide Web Consortium,
 * (Massachusetts Institute of Technology, Institut National de
 * Recherche en Informatique et en Automatique, Keio University). All
 * Rights Reserved. This program is distributed under the W3C's Software
 * Intellectual Property License. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 * See W3C License http://www.w3.org/Consortium/Legal/ for more details.
 */
package org.w3c.domts;

import org.w3c.dom.DOMLocator;
import org.w3c.dom.Node;

/**
 * Implementation of DOMLocator
 *
 */
public class DOMLocatorImpl
    implements DOMLocator {
  private final int lineNumber;
  private final int columnNumber;
  private final int byteOffset;
  private final int utf16Offset;
  private final Node relatedNode;
  private final String uri;

  public DOMLocatorImpl(DOMLocator src) {
    this.lineNumber = src.getLineNumber();
    this.columnNumber = src.getColumnNumber();
    this.byteOffset = src.getByteOffset();
    this.utf16Offset = src.getUtf16Offset();
    this.relatedNode = src.getRelatedNode();
    this.uri = src.getUri();
  }

  /*
   * Line number
   * @see org.w3c.dom.DOMLocator#getLineNumber()
   */
  public int getLineNumber() {
    return lineNumber;
  }

  /*
   * Column number
   * @see org.w3c.dom.DOMLocator#getColumnNumber()
   */
  public int getColumnNumber() {
    return columnNumber;
  }

  /*
   * Byte offset
   * @see org.w3c.dom.DOMLocator#getByteOffset()
   */
  public int getByteOffset() {
    return byteOffset;
  }

  /* UTF-16 offset
   * @see org.w3c.dom.DOMLocator#getUtf16Offset()
   */
  public int getUtf16Offset() {
    return utf16Offset;
  }

  /* Related node
   * @see org.w3c.dom.DOMLocator#getRelatedNode()
   */
  public Node getRelatedNode() {
    return relatedNode;
  }

  /* URI
   * @see org.w3c.dom.DOMLocator#getUri()
   */
  public String getUri() {
    return uri;
  }

}
