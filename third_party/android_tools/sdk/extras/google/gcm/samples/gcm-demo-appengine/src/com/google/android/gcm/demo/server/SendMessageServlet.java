/*
 * Copyright 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.gcm.demo.server;

import com.google.android.gcm.server.Constants;
import com.google.android.gcm.server.Message;
import com.google.android.gcm.server.MulticastResult;
import com.google.android.gcm.server.Result;
import com.google.android.gcm.server.Sender;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Servlet that sends a message to a device.
 * <p>
 * This servlet is invoked by AppEngine's Push Queue mechanism.
 */
@SuppressWarnings("serial")
public class SendMessageServlet extends BaseServlet {

  private static final String HEADER_QUEUE_COUNT = "X-AppEngine-TaskRetryCount";
  private static final String HEADER_QUEUE_NAME = "X-AppEngine-QueueName";
  private static final int MAX_RETRY = 3;

  static final String PARAMETER_DEVICE = "device";
  static final String PARAMETER_MULTICAST = "multicastKey";

  private Sender sender;

  @Override
  public void init(ServletConfig config) throws ServletException {
    super.init(config);
    sender = newSender(config);
  }

  /**
   * Creates the {@link Sender} based on the servlet settings.
   */
  protected Sender newSender(ServletConfig config) {
    String key = (String) config.getServletContext()
        .getAttribute(ApiKeyInitializer.ATTRIBUTE_ACCESS_KEY);
    return new Sender(key);
  }

  /**
   * Indicates to App Engine that this task should be retried.
   */
  private void retryTask(HttpServletResponse resp) {
    resp.setStatus(500);
  }

  /**
   * Indicates to App Engine that this task is done.
   */
  private void taskDone(HttpServletResponse resp) {
    resp.setStatus(200);
  }

  /**
   * Processes the request to add a new message.
   */
  @Override
  protected void doPost(HttpServletRequest req, HttpServletResponse resp)
      throws IOException {
    if (req.getHeader(HEADER_QUEUE_NAME) == null) {
      throw new IOException("Missing header " + HEADER_QUEUE_NAME);
    }
    String retryCountHeader = req.getHeader(HEADER_QUEUE_COUNT);
    logger.fine("retry count: " + retryCountHeader);
    if (retryCountHeader != null) {
      int retryCount = Integer.parseInt(retryCountHeader);
      if (retryCount > MAX_RETRY) {
          logger.severe("Too many retries, dropping task");
          taskDone(resp);
          return;
      }
    }
    String regId = req.getParameter(PARAMETER_DEVICE);
    if (regId != null) {
      sendSingleMessage(regId, resp);
      return;
    }
    String multicastKey = req.getParameter(PARAMETER_MULTICAST);
    if (multicastKey != null) {
      sendMulticastMessage(multicastKey, resp);
      return;
    }
    logger.severe("Invalid request!");
    taskDone(resp);
    return;
  }

  private void sendSingleMessage(String regId, HttpServletResponse resp) {
    logger.info("Sending message to device " + regId);
    Message message = new Message.Builder().build();
    Result result;
    try {
      result = sender.sendNoRetry(message, regId);
    } catch (IOException e) {
      logger.log(Level.SEVERE, "Exception posting " + message, e);
      taskDone(resp);
      return;
    }
    if (result == null) {
      retryTask(resp);
      return;
    }
    if (result.getMessageId() != null) {
      logger.info("Succesfully sent message to device " + regId);
      String canonicalRegId = result.getCanonicalRegistrationId();
      if (canonicalRegId != null) {
        // same device has more than on registration id: update it
        logger.finest("canonicalRegId " + canonicalRegId);
        Datastore.updateRegistration(regId, canonicalRegId);
      }
    } else {
      String error = result.getErrorCodeName();
      if (error.equals(Constants.ERROR_NOT_REGISTERED)) {
        // application has been removed from device - unregister it
        Datastore.unregister(regId);
      } else {
        logger.severe("Error sending message to device " + regId
            + ": " + error);
      }
    }
  }

  private void sendMulticastMessage(String multicastKey,
      HttpServletResponse resp) {
    // Recover registration ids from datastore
    List<String> regIds = Datastore.getMulticast(multicastKey);
    Message message = new Message.Builder().build();
    MulticastResult multicastResult;
    try {
      multicastResult = sender.sendNoRetry(message, regIds);
    } catch (IOException e) {
      logger.log(Level.SEVERE, "Exception posting " + message, e);
      multicastDone(resp, multicastKey);
      return;
    }
    boolean allDone = true;
    // check if any registration id must be updated
    if (multicastResult.getCanonicalIds() != 0) {
      List<Result> results = multicastResult.getResults();
      for (int i = 0; i < results.size(); i++) {
        String canonicalRegId = results.get(i).getCanonicalRegistrationId();
        if (canonicalRegId != null) {
          String regId = regIds.get(i);
          Datastore.updateRegistration(regId, canonicalRegId);
        }
      }
    }
    if (multicastResult.getFailure() != 0) {
      // there were failures, check if any could be retried
      List<Result> results = multicastResult.getResults();
      List<String> retriableRegIds = new ArrayList<String>();
      for (int i = 0; i < results.size(); i++) {
        String error = results.get(i).getErrorCodeName();
        if (error != null) {
          String regId = regIds.get(i);
          logger.warning("Got error (" + error + ") for regId " + regId);
          if (error.equals(Constants.ERROR_NOT_REGISTERED)) {
            // application has been removed from device - unregister it
            Datastore.unregister(regId);
          }
          if (error.equals(Constants.ERROR_UNAVAILABLE)) {
            retriableRegIds.add(regId);
          }
        }
      }
      if (!retriableRegIds.isEmpty()) {
        // update task
        Datastore.updateMulticast(multicastKey, retriableRegIds);
        allDone = false;
        retryTask(resp);
      }
    }
    if (allDone) {
      multicastDone(resp, multicastKey);
    } else {
      retryTask(resp);
    }
  }

  private void multicastDone(HttpServletResponse resp, String encodedKey) {
    Datastore.deleteMulticast(encodedKey);
    taskDone(resp);
  }

}
