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
package com.google.android.gcm.server;

import static com.google.android.gcm.server.Constants.GCM_SEND_ENDPOINT;
import static com.google.android.gcm.server.Constants.JSON_CANONICAL_IDS;
import static com.google.android.gcm.server.Constants.JSON_ERROR;
import static com.google.android.gcm.server.Constants.JSON_FAILURE;
import static com.google.android.gcm.server.Constants.JSON_MESSAGE_ID;
import static com.google.android.gcm.server.Constants.JSON_MULTICAST_ID;
import static com.google.android.gcm.server.Constants.JSON_PAYLOAD;
import static com.google.android.gcm.server.Constants.JSON_REGISTRATION_IDS;
import static com.google.android.gcm.server.Constants.JSON_RESULTS;
import static com.google.android.gcm.server.Constants.JSON_SUCCESS;
import static com.google.android.gcm.server.Constants.PARAM_COLLAPSE_KEY;
import static com.google.android.gcm.server.Constants.PARAM_DELAY_WHILE_IDLE;
import static com.google.android.gcm.server.Constants.PARAM_PAYLOAD_PREFIX;
import static com.google.android.gcm.server.Constants.PARAM_REGISTRATION_ID;
import static com.google.android.gcm.server.Constants.PARAM_TIME_TO_LIVE;
import static com.google.android.gcm.server.Constants.TOKEN_CANONICAL_REG_ID;
import static com.google.android.gcm.server.Constants.TOKEN_ERROR;
import static com.google.android.gcm.server.Constants.TOKEN_MESSAGE_ID;

import com.google.android.gcm.server.Result.Builder;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Helper class to send messages to the GCM service using an API Key.
 */
public class Sender {

  protected static final String UTF8 = "UTF-8";

  /**
   * Initial delay before first retry, without jitter.
   */
  protected static final int BACKOFF_INITIAL_DELAY = 1000;
  /**
   * Maximum delay before a retry.
   */
  protected static final int MAX_BACKOFF_DELAY = 1024000;

  protected final Random random = new Random();
  protected final Logger logger = Logger.getLogger(getClass().getName());

  private final String key;

  /**
   * Default constructor.
   *
   * @param key API key obtained through the Google API Console.
   */
  public Sender(String key) {
    this.key = nonNull(key);
  }

  /**
   * Sends a message to one device, retrying in case of unavailability.
   *
   * <p>
   * <strong>Note: </strong> this method uses exponential back-off to retry in
   * case of service unavailability and hence could block the calling thread
   * for many seconds.
   *
   * @param message message to be sent, including the device's registration id.
   * @param registrationId device where the message will be sent.
   * @param retries number of retries in case of service unavailability errors.
   *
   * @return result of the request (see its javadoc for more details)
   *
   * @throws IllegalArgumentException if registrationId is {@literal null}.
   * @throws InvalidRequestException if GCM didn't returned a 200 or 503 status.
   * @throws IOException if message could not be sent.
   */
  public Result send(Message message, String registrationId, int retries)
      throws IOException {
    int attempt = 0;
    Result result = null;
    int backoff = BACKOFF_INITIAL_DELAY;
    boolean tryAgain;
    do {
      attempt++;
      if (logger.isLoggable(Level.FINE)) {
        logger.fine("Attempt #" + attempt + " to send message " +
            message + " to regIds " + registrationId);
      }
      result = sendNoRetry(message, registrationId);
      tryAgain = result == null && attempt <= retries;
      if (tryAgain) {
        int sleepTime = backoff / 2 + random.nextInt(backoff);
        sleep(sleepTime);
        if (2 * backoff < MAX_BACKOFF_DELAY) {
          backoff *= 2;
        }
      }
    } while (tryAgain);
    if (result == null) {
      throw new IOException("Could not send message after " + attempt +
          " attempts");
    }
    return result;
  }

  /**
   * Sends a message without retrying in case of service unavailability. See
   * {@link #send(Message, String, int)} for more info.
   *
   * @return result of the post, or {@literal null} if the GCM service was
   *         unavailable.
   *
   * @throws InvalidRequestException if GCM didn't returned a 200 or 503 status.
   * @throws IllegalArgumentException if registrationId is {@literal null}.
   */
  public Result sendNoRetry(Message message, String registrationId)
      throws IOException {
    StringBuilder body = newBody(PARAM_REGISTRATION_ID, registrationId);
    Boolean delayWhileIdle = message.isDelayWhileIdle();
    if (delayWhileIdle != null) {
      addParameter(body, PARAM_DELAY_WHILE_IDLE, delayWhileIdle ? "1" : "0");
    }
    String collapseKey = message.getCollapseKey();
    if (collapseKey != null) {
      addParameter(body, PARAM_COLLAPSE_KEY, collapseKey);
    }
    Integer timeToLive = message.getTimeToLive();
    if (timeToLive != null) {
      addParameter(body, PARAM_TIME_TO_LIVE, Integer.toString(timeToLive));
    }
    for (Entry<String, String> entry : message.getData().entrySet()) {
      String key = PARAM_PAYLOAD_PREFIX + entry.getKey();
      String value = entry.getValue();
      addParameter(body, key, URLEncoder.encode(value, UTF8));
    }
    String requestBody = body.toString();
    logger.finest("Request body: " + requestBody);
    HttpURLConnection conn = post(GCM_SEND_ENDPOINT, requestBody);
    int status = conn.getResponseCode();
    if (status == 503) {
      logger.fine("GCM service is unavailable");
      return null;
    }
    if (status != 200) {
      throw new InvalidRequestException(status);
    }
    try {
      BufferedReader reader =
          new BufferedReader(new InputStreamReader(conn.getInputStream()));
      try {
        String line = reader.readLine();

        if (line == null || line.equals("")) {
          throw new IOException("Received empty response from GCM service.");
        }
        String[] responseParts = split(line);
        String token = responseParts[0];
        String value = responseParts[1];
        if (token.equals(TOKEN_MESSAGE_ID)) {
          Builder builder = new Result.Builder().messageId(value);
          // check for canonical registration id
          line = reader.readLine();
          if (line != null) {
            responseParts = split(line);
            token = responseParts[0];
            value = responseParts[1];
            if (token.equals(TOKEN_CANONICAL_REG_ID)) {
              builder.canonicalRegistrationId(value);
            } else {
              logger.warning("Received invalid second line from GCM: " + line);
            }
          }

          Result result = builder.build();
          if (logger.isLoggable(Level.FINE)) {
            logger.fine("Message created succesfully (" + result + ")");
          }
          return result;
        } else if (token.equals(TOKEN_ERROR)) {
          return new Result.Builder().errorCode(value).build();
        } else {
          throw new IOException("Received invalid response from GCM: " + line);
        }
      } finally {
        reader.close();
      }
    } finally {
      conn.disconnect();
    }
  }

  /**
   * Sends a message to many devices, retrying in case of unavailability.
   *
   * <p>
   * <strong>Note: </strong> this method uses exponential back-off to retry in
   * case of service unavailability and hence could block the calling thread
   * for many seconds.
   *
   * @param message message to be sent.
   * @param regIds registration id of the devices that will receive
   *        the message.
   * @param retries number of retries in case of service unavailability errors.
   *
   * @return combined result of all requests made.
   *
   * @throws IllegalArgumentException if registrationIds is {@literal null} or
   *         empty.
   * @throws InvalidRequestException if GCM didn't returned a 200 or 503 status.
   * @throws IOException if message could not be sent.
   */
  public MulticastResult send(Message message, List<String> regIds, int retries)
      throws IOException {
    int attempt = 0;
    MulticastResult multicastResult = null;
    int backoff = BACKOFF_INITIAL_DELAY;
    // Map of results by registration id, it will be updated after each attempt
    // to send the messages
    Map<String, Result> results = new HashMap<String, Result>();
    List<String> unsentRegIds = new ArrayList<String>(regIds);
    boolean tryAgain;
    List<Long> multicastIds = new ArrayList<Long>();
    do {
      attempt++;
      if (logger.isLoggable(Level.FINE)) {
        logger.fine("Attempt #" + attempt + " to send message " +
            message + " to regIds " + unsentRegIds);
      }
      multicastResult = sendNoRetry(message, unsentRegIds);
      long multicastId = multicastResult.getMulticastId();
      logger.fine("multicast_id on attempt # " + attempt + ": " +
          multicastId);
      multicastIds.add(multicastId);
      unsentRegIds = updateStatus(unsentRegIds, results, multicastResult);
      tryAgain = !unsentRegIds.isEmpty() && attempt <= retries;
      if (tryAgain) {
        int sleepTime = backoff / 2 + random.nextInt(backoff);
        sleep(sleepTime);
        if (2 * backoff < MAX_BACKOFF_DELAY) {
          backoff *= 2;
        }
      }
    } while (tryAgain);
    // calculate summary
    int success = 0, failure = 0 , canonicalIds = 0;
    for (Result result : results.values()) {
      if (result.getMessageId() != null) {
        success++;
        if (result.getCanonicalRegistrationId() != null) {
          canonicalIds++;
        }
      } else {
        failure++;
      }
    }
    // build a new object with the overall result
    long multicastId = multicastIds.remove(0);
    MulticastResult.Builder builder = new MulticastResult.Builder(success,
        failure, canonicalIds, multicastId).retryMulticastIds(multicastIds);
    // add results, in the same order as the input
    for (String regId : regIds) {
      Result result = results.get(regId);
      builder.addResult(result);
    }
    return builder.build();
  }

  /**
   * Updates the status of the messages sent to devices and the list of devices
   * that should be retried.
   *
   * @param unsentRegIds list of devices that are still pending an update.
   * @param allResults map of status that will be updated.
   * @param multicastResult result of the last multicast sent.
   *
   * @return updated version of devices that should be retried.
   */
  private List<String> updateStatus(List<String> unsentRegIds,
      Map<String, Result> allResults, MulticastResult multicastResult) {
    List<Result> results = multicastResult.getResults();
    if (results.size() != unsentRegIds.size()) {
      // should never happen, unless there is a flaw in the algorithm
      throw new RuntimeException("Internal error: sizes do not match. " +
          "currentResults: " + results + "; unsentRegIds: " + unsentRegIds);
    }
    List<String> newUnsentRegIds = new ArrayList<String>();
    for (int i = 0; i < unsentRegIds.size(); i++) {
      String regId = unsentRegIds.get(i);
      Result result = results.get(i);
      allResults.put(regId, result);
      String error = result.getErrorCodeName();
      if (error != null && error.equals(Constants.ERROR_UNAVAILABLE)) {
        newUnsentRegIds.add(regId);
      }
    }
    return newUnsentRegIds;
  }

  /**
   * Sends a message without retrying in case of service unavailability. See
   * {@link #send(Message, List, int)} for more info.
   *
   * @return {@literal true} if the message was sent successfully,
   *         {@literal false} if it failed but could be retried.
   *
   * @throws IllegalArgumentException if registrationIds is {@literal null} or
   *         empty.
   * @throws InvalidRequestException if GCM didn't returned a 200 status.
   * @throws IOException if message could not be sent or received.
   */
  public MulticastResult sendNoRetry(Message message,
      List<String> registrationIds) throws IOException {
    if (nonNull(registrationIds).isEmpty()) {
      throw new IllegalArgumentException("registrationIds cannot be empty");
    }
    Map<Object, Object> jsonRequest = new HashMap<Object, Object>();
    setJsonField(jsonRequest, PARAM_TIME_TO_LIVE, message.getTimeToLive());
    setJsonField(jsonRequest, PARAM_COLLAPSE_KEY, message.getCollapseKey());
    setJsonField(jsonRequest, PARAM_DELAY_WHILE_IDLE,
        message.isDelayWhileIdle());
    jsonRequest.put(JSON_REGISTRATION_IDS, registrationIds);
    Map<String, String> payload = message.getData();
    if (!payload.isEmpty()) {
      jsonRequest.put(JSON_PAYLOAD, payload);
    }
    String requestBody = JSONValue.toJSONString(jsonRequest);
    logger.finest("JSON request: " + requestBody);
    HttpURLConnection conn =
        post(GCM_SEND_ENDPOINT, "application/json", requestBody);
    int status = conn.getResponseCode();
    String responseBody;
    if (status != 200) {
      responseBody = getString(conn.getErrorStream());
      logger.finest("JSON error response: " + responseBody);
      throw new InvalidRequestException(status, responseBody);
    }
    responseBody = getString(conn.getInputStream());
    logger.finest("JSON response: " + responseBody);
    JSONParser parser = new JSONParser();
    JSONObject jsonResponse;
    try {
      jsonResponse = (JSONObject) parser.parse(responseBody);
      int success = getNumber(jsonResponse, JSON_SUCCESS).intValue();
      int failure = getNumber(jsonResponse, JSON_FAILURE).intValue();
      int canonicalIds = getNumber(jsonResponse, JSON_CANONICAL_IDS).intValue();
      long multicastId = getNumber(jsonResponse, JSON_MULTICAST_ID).longValue();
      MulticastResult.Builder builder = new MulticastResult.Builder(success,
          failure, canonicalIds, multicastId);
      @SuppressWarnings("unchecked")
      List<Map<String, Object>> results =
          (List<Map<String, Object>>) jsonResponse.get(JSON_RESULTS);
      if (results != null) {
        for (Map<String, Object> jsonResult : results) {
          String messageId = (String) jsonResult.get(JSON_MESSAGE_ID);
          String canonicalRegId =
              (String) jsonResult.get(TOKEN_CANONICAL_REG_ID);
          String error = (String) jsonResult.get(JSON_ERROR);
          Result result = new Result.Builder()
              .messageId(messageId)
              .canonicalRegistrationId(canonicalRegId)
              .errorCode(error)
              .build();
          builder.addResult(result);
        }
      }
      MulticastResult multicastResult = builder.build();
      return multicastResult;
    } catch (ParseException e) {
      throw newIoException(responseBody, e);
    } catch (CustomParserException e) {
      throw newIoException(responseBody, e);
    }
  }

  private IOException newIoException(String responseBody, Exception e) {
    // log exception, as IOException constructor that takes a message and cause
    // is only available on Java 6
    String msg = "Error parsing JSON response (" + responseBody + ")";
    logger.log(Level.WARNING, msg, e);
    return new IOException(msg + ":" + e);
  }

  /**
   * Sets a JSON field, but only if the value is not {@literal null}.
   */
  private void setJsonField(Map<Object, Object> json, String field,
      Object value) {
    if (value != null) {
      json.put(field, value);
    }
  }

  private Number getNumber(Map<?, ?> json, String field) {
    Object value = json.get(field);
    if (value == null) {
      throw new CustomParserException("Missing field: " + field);
    }
    if (!(value instanceof Number)) {
      throw new CustomParserException("Field " + field +
          " does not contain a number: " + value);
    }
    return (Number) value;
  }

  class CustomParserException extends RuntimeException {
    CustomParserException(String message) {
      super(message);
    }
  }

  private String[] split(String line) throws IOException {
    String[] split = line.split("=", 2);
    if (split.length != 2) {
      throw new IOException("Received invalid response line from GCM: " + line);
    }
    return split;
  }

  /**
   * Make an HTTP post to a given URL.
   *
   * @return HTTP response.
   */
  protected HttpURLConnection post(String url, String body)
      throws IOException {
    return post(url, "application/x-www-form-urlencoded;charset=UTF-8", body);
  }

  protected HttpURLConnection post(String url, String contentType, String body)
      throws IOException {
    if (url == null || body == null) {
      throw new IllegalArgumentException("arguments cannot be null");
    }
    if (!url.startsWith("https://")) {
      logger.warning("URL does not use https: " + url);
    }
    logger.fine("Sending POST to " + url);
    logger.finest("POST body: " + body);
    byte[] bytes = body.getBytes();
    HttpURLConnection conn = getConnection(url);
    conn.setDoOutput(true);
    conn.setUseCaches(false);
    conn.setFixedLengthStreamingMode(bytes.length);
    conn.setRequestMethod("POST");
    conn.setRequestProperty("Content-Type", contentType);
    conn.setRequestProperty("Authorization", "key=" + key);
    OutputStream out = conn.getOutputStream();
    out.write(bytes);
    out.close();
    return conn;
  }

  /**
   * Creates a map with just one key-value pair.
   */
  protected static final Map<String, String> newKeyValues(String key,
      String value) {
    Map<String, String> keyValues = new HashMap<String, String>(1);
    keyValues.put(nonNull(key), nonNull(value));
    return keyValues;
  }

  /**
   * Creates a {@link StringBuilder} to be used as the body of an HTTP POST.
   *
   * @param name initial parameter for the POST.
   * @param value initial value for that parameter.
   * @return StringBuilder to be used an HTTP POST body.
   */
  protected static StringBuilder newBody(String name, String value) {
    return new StringBuilder(nonNull(name)).append('=').append(nonNull(value));
  }

  /**
   * Adds a new parameter to the HTTP POST body.
   *
   * @param body HTTP POST body
   * @param name parameter's name
   * @param value parameter's value
   */
  protected static void addParameter(StringBuilder body, String name,
      String value) {
    nonNull(body).append('&')
        .append(nonNull(name)).append('=').append(nonNull(value));
  }

  /**
   * Gets an {@link HttpURLConnection} given an URL.
   */
  protected HttpURLConnection getConnection(String url) throws IOException {
    HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
    return conn;
  }

  /**
   * Convenience method to convert an InputStream to a String.
   *
   * <p>
   * If the stream ends in a newline character, it will be stripped.
   */
  protected static String getString(InputStream stream) throws IOException {
    BufferedReader reader =
        new BufferedReader(new InputStreamReader(nonNull(stream)));
    StringBuilder content = new StringBuilder();
    String newLine;
    do {
      newLine = reader.readLine();
      if (newLine != null) {
        content.append(newLine).append('\n');
      }
    } while (newLine != null);
    if (content.length() > 0) {
      // strip last newline
      content.setLength(content.length() - 1);
    }
    return content.toString();
  }

  static <T> T nonNull(T argument) {
    if (argument == null) {
      throw new IllegalArgumentException("argument cannot be null");
    }
    return argument;
  }

  void sleep(long millis) {
    try {
      Thread.sleep(millis);
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

}
