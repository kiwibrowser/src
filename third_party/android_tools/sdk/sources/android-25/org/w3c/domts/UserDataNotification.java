/*
 * Copyright (c) 2004 World Wide Web Consortium, (Massachusetts Institute of
 * Technology, Institut National de Recherche en Informatique et en
 * Automatique, Keio University). All Rights Reserved. This program is
 * distributed under the W3C's Software Intellectual Property License. This
 * program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See W3C License
 * http://www.w3.org/Consortium/Legal/ for more details.
 */

package org.w3c.domts;

import org.w3c.dom.Node;

/**
 * This class captures the parameters to one invocation of
 * UserDataHandler.handle.
 *
 */
public class UserDataNotification {
  private final short operation;
  private final String key;
  private final Object data;
  private final Node src;
  private final Node dst;

  /**
   * Public constructor
   *
   */
  public UserDataNotification(short operation,
                              String key,
                              Object data,
                              Node src,
                              Node dst) {
    this.operation = operation;
    this.key = key;
    this.data = data;
    this.src = src;
    this.dst = dst;
  }

  /**
   * Get value of operation parameter
   *
   * @return value of operation parameter
   */
  public final short getOperation() {
    return operation;
  }

  /**
   * Gets value of key parameter
   *
   * @return value of key parameter
   */
  public final String getKey() {
    return key;
  }

  /**
   * Gets value of data parameter
   *
   * @return value of data parameter
   */
  public final Object getData() {
    return data;
  }

  /**
   * Gets value of src parameter
   *
   * @return value of src parameter
   */
  public final Node getSrc() {
    return src;
  }

  /**
   * Gets value of dst parameter
   *
   * @return value of dst parameter
   */
  public final Node getDst() {
    return dst;
  }
}
