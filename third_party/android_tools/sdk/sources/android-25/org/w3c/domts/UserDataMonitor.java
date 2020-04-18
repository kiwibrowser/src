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

import java.util.ArrayList;
import java.util.List;

import org.w3c.dom.Node;
import org.w3c.dom.UserDataHandler;

/**
 * This is a utility implementation of UserDataHandler that captures all
 * notifications
 */
public class UserDataMonitor
    implements UserDataHandler {
  private final List notifications = new ArrayList();

  /**
   * Public constructor
   *
   */
  public UserDataMonitor() {
  }

  /**
   * Implementation of UserDataHandler.handle. Creates a UserDataNotification
   * for later testing
   *
   * @param operation
   *            See org.w3c.dom.UserDataHandler
   * @param key
   *            See org.w3c.dom.UserDataHandler
   * @param data
   *            See org.w3c.dom.UserDataHandler
   * @param src
   *            See org.w3c.dom.UserDataHandler
   * @param dst
   *            See org.w3c.dom.UserDataHandler
   */
  public void handle(
      short operation,
      String key,
      Object data,
      Node src,
      Node dst) {
    notifications.add(
        new UserDataNotification(operation, key, data, src, dst));
  }

  /**
   * Gets list of notifications
   *
   * @return List of notifications, may not be null.
   */
  public final List getAllNotifications() {
    return new ArrayList(notifications);
  }

}
