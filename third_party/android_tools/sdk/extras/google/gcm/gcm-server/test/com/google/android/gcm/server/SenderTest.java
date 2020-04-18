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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyListOf;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.runners.MockitoJUnitRunner;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RunWith(MockitoJUnitRunner.class)
public class SenderTest {

  private final String regId = "15;16";
  private final String collapseKey = "collapseKey";
  private final boolean delayWhileIdle = true;
  private final int retries = 42;
  private final int ttl = 108;
  private final String authKey = "4815162342";
  private final JSONParser jsonParser = new JSONParser();
  
  private final Message message =
      new Message.Builder()
          .collapseKey(collapseKey)
          .delayWhileIdle(delayWhileIdle)
          .timeToLive(ttl)
          .addData("k1", "v1")
          .addData("k2", "v2")
          .addData("k3", "v3")
          .build();

  // creates a Mockito Spy so we can stub internal methods
  @Spy private Sender sender = new Sender(authKey);

  @Mock private HttpURLConnection mockedConn;
  private final ByteArrayOutputStream outputStream = 
      new ByteArrayOutputStream();
  private Result result;

  @Before
  public void setFixtures() {
    result = new Result.Builder().build();
  }

  @Test(expected = IllegalArgumentException.class)
  public void testConstructor_null() {
    new Sender(null);
  }

  @Test
  public void testSend_noRetryOk() throws Exception {
    doNotSleep();
    doReturn(result).when(sender).sendNoRetry(message, regId);
    sender.send(message, regId, 0);
  }

  @Test(expected = IOException.class)
  public void testSend_noRetryFail() throws Exception {
    doNotSleep();
    doReturn(null).when(sender).sendNoRetry(message, regId);
    sender.send(message, regId, 0);
  }

  @Test(expected = IOException.class)
  public void testSend_noRetryException() throws Exception {
    doThrow(new IOException()).when(sender).sendNoRetry(message, regId);
    sender.send(message, regId, 0);
  }

  @Test
  public void testSend_retryOk() throws Exception {
    doNothing().when(sender).sleep(anyInt());
    doReturn(null) // fails 1st time
        .doReturn(null) // fails 2nd time
        .doReturn(result) // succeeds 3rd time
        .when(sender).sendNoRetry(message, regId);
    sender.send(message, regId, 2);
    verify(sender, times(3)).sendNoRetry(message, regId);
  }

  @Test(expected = IOException.class)
  public void testSend_retryFails() throws Exception {
    doNothing().when(sender).sleep(anyInt());
    doReturn(null) // fails 1st time
        .doReturn(null) // fails 2nd time
        .doReturn(null) // fails 3rd time
        .when(sender).sendNoRetry(message, regId);
    sender.send(message, regId, 2);
    verify(sender, times(3)).sendNoRetry(message, regId);
  }

  @Test
  public void testSend_retryExponentialBackoff() throws Exception {
    ArgumentCaptor<Long> capturedSleep = ArgumentCaptor.forClass(Long.class);
    int total = retries + 1; // fist attempt + retries
    doNothing().when(sender).sleep(anyInt());
    doReturn(null).when(sender).sendNoRetry(message, regId);
    try {
      sender.send(message, regId, retries);
      fail("Should have thrown IOEXception");
    } catch (IOException e) {
      String message = e.getMessage();
      assertTrue("invalid message:" + message, message.contains("" + total));
    }
    verify(sender, times(total)).sendNoRetry(message, regId);
    verify(sender, times(retries)).sleep(capturedSleep.capture());
    long backoffRange = Sender.BACKOFF_INITIAL_DELAY;
    for (long value : capturedSleep.getAllValues()) {
      assertTrue(value >= backoffRange / 2);
      assertTrue(value <= backoffRange * 3 / 2);
      if (2 * backoffRange < Sender.MAX_BACKOFF_DELAY) {
        backoffRange *= 2;
      }
    }
  }

  @Test
  public void testSendNoRetry_ok() throws Exception {
    setResponseExpectations(200, "id=4815162342");
    Result result = sender.sendNoRetry(message, regId);
    assertNotNull(result);
    assertEquals("4815162342", result.getMessageId());
    assertNull(result.getCanonicalRegistrationId());
    assertNull(result.getErrorCodeName());
    assertRequestBody();
    verify(mockedConn).disconnect();
  }

  @Test
  public void testSendNoRetry_ok_canonical() throws Exception {
    setResponseExpectations(200, "id=4815162342\nregistration_id=108");
    Result result = sender.sendNoRetry(message, regId);
    assertNotNull(result);
    assertEquals("4815162342", result.getMessageId());
    assertEquals("108", result.getCanonicalRegistrationId());
    assertNull(result.getErrorCodeName());
    assertRequestBody();
    verify(mockedConn).disconnect();
  }

  @Test
  public void testSendNoRetry_unauthorized() throws Exception {
    setResponseExpectations(401, "");
    try {
      sender.sendNoRetry(message, regId);
      fail("Should have thrown InvalidRequestException");
    } catch (InvalidRequestException e) {
      assertEquals(401, e.getHttpStatusCode());
    }
    assertRequestBody();
  }

  @Test
  public void testSendNoRetry_error() throws Exception {
    setResponseExpectations(200, "Error=D'OH!");
    Result result = sender.sendNoRetry(message, regId);
    assertNull(result.getMessageId());
    assertNull(result.getCanonicalRegistrationId());
    assertEquals("D'OH!", result.getErrorCodeName());
    assertRequestBody();
    verify(mockedConn).disconnect();
  }

  @Test
  public void testSendNoRetry_serviceUnavailable() throws Exception {
    setResponseExpectations(503, "");
    Result result = sender.sendNoRetry(message, regId);
    assertNull(result);
    assertRequestBody();
  }

  @Test(expected = IOException.class)
  public void testSendNoRetry_emptyBody() throws Exception {
    setResponseExpectations(200, "");
    sender.sendNoRetry(message, regId);
    assertRequestBody();
    verify(mockedConn).disconnect();
  }

  @Test(expected = IOException.class)
  public void testSendNoRetry_noToken() throws Exception {
    setResponseExpectations(200, "no token");
    sender.sendNoRetry(message, regId);
    assertRequestBody();
    verify(mockedConn).disconnect();
  }

  @Test(expected = IOException.class)
  public void testSendNoRetry_invalidToken() throws Exception {
    setResponseExpectations(200, "bad=token");
    sender.sendNoRetry(message, regId);
    verify(mockedConn).disconnect();
  }

  @Test(expected = IOException.class)
  public void testSendNoRetry_emptyToken() throws Exception {
    setResponseExpectations(200, "token=");
    sender.sendNoRetry(message, regId);
    verify(mockedConn).disconnect();
  }

  @Test
  public void testSendNoRetry_invalidHttpStatusCode() throws Exception {
    setResponseExpectations(108, "id=4815162342");
    try {
      sender.sendNoRetry(message, regId);
    } catch (InvalidRequestException e) {
      assertEquals(108, e.getHttpStatusCode());
    }
  }

  @Test(expected = IllegalArgumentException.class)
  public void testSendNoRetry_noRegistrationId() throws Exception {
    sender.sendNoRetry(new Message.Builder().build(), (String) null);
  }

  @Test()
  public void testSend_json_allAttemptsFail() throws Exception {
    doNothing().when(sender).sleep(anyInt());
    // mock sendNoRetry
    Result unaivalableResult =
        new Result.Builder().errorCode("Unavailable").build();
    // for the intermediate request, only the multicast id matters
    MulticastResult mockedResult = new MulticastResult.Builder(0, 0, 0, 42)
      .addResult(unaivalableResult).build();
    List<String> regIds = Arrays.asList("108");
    doReturn(mockedResult).when(sender).sendNoRetry(message, regIds);
    MulticastResult actualResult = sender.send(message, regIds, 2);
    assertNotNull(actualResult);
    assertEquals(1, actualResult.getTotal());
    assertEquals(0, actualResult.getSuccess());
    assertEquals(1, actualResult.getFailure());
    assertEquals(0, actualResult.getCanonicalIds());
    assertEquals(42, actualResult.getMulticastId());
    assertEquals(1, actualResult.getResults().size());
    assertResult(actualResult.getResults().get(0), null, "Unavailable", null);
    verify(sender, times(3)).sendNoRetry(message, regIds);
  }

  @Test()
  public void testSend_json_secondAttemptOk() throws Exception {
    doNothing().when(sender).sleep(anyInt());
    // mock sendNoRetry
    Result unaivalableResult =
        new Result.Builder().errorCode("Unavailable").build();
    Result okResult =
        new Result.Builder().messageId("42").build();
    // for the intermediate request, only the multicast id matters
    MulticastResult mockedResult1 = new MulticastResult.Builder(0, 0, 0, 100)
        .addResult(unaivalableResult).build();
    MulticastResult mockedResult2 = new MulticastResult.Builder(0, 0, 0, 200)
        .addResult(okResult).build();
    List<String> regIds = Arrays.asList("108");
    doReturn(mockedResult1) // fist time it fails
        .doReturn(mockedResult2) // second time it succeeds
        .when(sender).sendNoRetry(message, regIds);
    MulticastResult actualResult = sender.send(message, regIds, 10);
    assertNotNull(actualResult);
    assertEquals(1, actualResult.getTotal());
    assertEquals(1, actualResult.getSuccess());
    assertEquals(0, actualResult.getFailure());
    assertEquals(0, actualResult.getCanonicalIds());
    assertEquals(100, actualResult.getMulticastId());
    assertEquals(1, actualResult.getResults().size());
    assertResult(actualResult.getResults().get(0), "42", null, null);
    List<Long> retryMulticastIds = actualResult.getRetryMulticastIds();
    assertEquals(1, retryMulticastIds.size());
    assertEquals(200, retryMulticastIds.get(0).longValue());
    verify(sender, times(2)).sendNoRetry(message, regIds);
  }

  @Test()
  public void testSend_json_ok() throws Exception {
    doNothing().when(sender).sleep(anyInt());
    /*
     * The following scenario is mocked below:
     *
     * input: 4, 8, 15, 16, 23, 42
     *
     * 1st call (multicast_id:100): 4,16:ok 8,15,23:unavailable, 42:error,
     * 2nd call (multicast_id:200): 8,15: unavailable, 23:ok
     * 3rd call (multicast_id:300): 8:error, 15:unavailable
     * 4th call (multicast_id:400): 15:unavailable
     *
     * output: total:6, success:3, error: 3, canonicals: 0, multicast_id: 100
     *         results: ok, error, unavailable, ok, ok, error
     */
    Result unaivalableResult =
        new Result.Builder().errorCode("Unavailable").build();
    Result errorResult =
        new Result.Builder().errorCode("D'OH!").build();
    Result okResultMsg4 =
        new Result.Builder().messageId("msg4").build();
    Result okResultMsg16 =
        new Result.Builder().messageId("msg16").build();
    Result okResultMsg23 =
        new Result.Builder().messageId("msg23").build();
    MulticastResult result1stCall = new MulticastResult.Builder(0, 0, 0, 100)
        .addResult(okResultMsg4)
        .addResult(unaivalableResult)
        .addResult(unaivalableResult)
        .addResult(okResultMsg16)
        .addResult(unaivalableResult)
        .addResult(errorResult)
        .build();
    doReturn(result1stCall).when(sender).sendNoRetry(message,
        Arrays.asList("4", "8", "15", "16", "23", "42"));
    MulticastResult result2ndCall = new MulticastResult.Builder(0, 0, 0, 200)
      .addResult(unaivalableResult)
      .addResult(unaivalableResult)
      .addResult(okResultMsg23)
      .build();
    doReturn(result2ndCall).when(sender).sendNoRetry(message,
        Arrays.asList("8", "15", "23"));
    MulticastResult result3rdCall = new MulticastResult.Builder(0, 0, 0, 300)
      .addResult(errorResult)
      .addResult(unaivalableResult)
      .build();
    doReturn(result3rdCall).when(sender).sendNoRetry(message,
        Arrays.asList("8", "15"));
    MulticastResult result4thCall = new MulticastResult.Builder(0, 0, 0, 400)
      .addResult(unaivalableResult)
      .build();
    doReturn(result4thCall).when(sender).sendNoRetry(message,
        Arrays.asList("15"));

    // call it
    MulticastResult actualResult = sender.send(message,
        Arrays.asList("4", "8", "15", "16", "23", "42"), 3);

    // assert results
    assertNotNull(actualResult);
    assertEquals(6, actualResult.getTotal());
    assertEquals(3, actualResult.getSuccess());
    assertEquals(3, actualResult.getFailure());
    assertEquals(0, actualResult.getCanonicalIds());
    assertEquals(100, actualResult.getMulticastId());
    List<Result> actualResults = actualResult.getResults();
    assertEquals(6, actualResults.size());
    assertResult(actualResults.get(0), "msg4", null, null); // 4
    assertResult(actualResults.get(1), null, "D'OH!", null); // 8
    assertResult(actualResults.get(2), null, "Unavailable", null); // 15
    assertResult(actualResults.get(3), "msg16", null, null); // 16
    assertResult(actualResults.get(4), "msg23", null, null); // 23
    assertResult(actualResults.get(5), null, "D'OH!", null); // 42
    List<Long> retryMulticastIds = actualResult.getRetryMulticastIds();
    assertEquals(3, retryMulticastIds.size());
    assertEquals(200, retryMulticastIds.get(0).longValue());
    assertEquals(300, retryMulticastIds.get(1).longValue());
    assertEquals(400, retryMulticastIds.get(2).longValue());
    verify(sender, times(4)).sendNoRetry(eq(message), anyListOf(String.class));
  }

  @Test(expected = IllegalArgumentException.class)
  public void testSendNoRetry_json_nullRegIds() throws Exception {
    sender.sendNoRetry(message, (List<String>) null);
  }

  @Test(expected = IllegalArgumentException.class)
  public void testSendNoRetry_json_emptyRegIds() throws Exception {
    sender.sendNoRetry(message, Collections.<String>emptyList());
  }

  @Test
  public void testSendNoRetry_json_badRequest() throws Exception {
    setResponseExpectations(42, "bad json");
    try {
      sender.sendNoRetry(message, Arrays.asList("108"));
    } catch (InvalidRequestException e) {
      assertEquals(42, e.getHttpStatusCode());
      assertEquals("bad json", e.getDescription());
      assertRequestJsonBody("108");
    }
  }

  @Test()
  public void testSendNoRetry_json_ok() throws Exception {
    String json = replaceQuotes("\n"
        + "{"
        + "  'multicast_id': 108,"
        + "  'success': 2,"
        + "  'failure': 1,"
        + "  'canonical_ids': 1,"
        + "  'results': ["
        + "    {'message_id': '16'}, "
        + "    {'error': 'DOH!'}, "
        + "    {'message_id': '23', 'registration_id': '42'}"
        + "  ]"
        + "}");
    setResponseExpectations(200, json);
    MulticastResult multicastResult = sender.sendNoRetry(message,
        Arrays.asList("4", "8", "15"));
    assertNotNull(multicastResult);
    assertEquals(3, multicastResult.getTotal());
    assertEquals(2, multicastResult.getSuccess());
    assertEquals(1, multicastResult.getFailure());
    assertEquals(1, multicastResult.getCanonicalIds());
    assertEquals(108, multicastResult.getMulticastId());
    List<Result> results = multicastResult.getResults();
    assertNotNull(results);
    assertEquals(3, results.size());
    assertResult(results.get(0), "16", null, null);
    assertResult(results.get(1), null, "DOH!", null);
    assertResult(results.get(2), "23", null, "42");
    assertRequestJsonBody("4", "8", "15");
  }

  // replace ' by ", otherwise JSON strins would need to escape double-quotes
  private String replaceQuotes(String json) {
    return json.replaceAll("'", "\"");
  }

  private void assertResult(Result result, String messageId, String error,
      String canonicalRegistrationId) {
    assertEquals(messageId, result.getMessageId());
    assertEquals(error, result.getErrorCodeName());
    assertEquals(canonicalRegistrationId, result.getCanonicalRegistrationId());
  }

  private void assertRequestJsonBody(String...expectedRegIds) throws Exception {
    ArgumentCaptor<String> capturedBody = ArgumentCaptor.forClass(String.class);
    verify(sender).post(eq(Constants.GCM_SEND_ENDPOINT), eq("application/json"),
        capturedBody.capture());
    // parse body
    String body = capturedBody.getValue();
    JSONObject json = (JSONObject) jsonParser.parse(body);
    assertEquals(ttl, ((Long) json.get("time_to_live")).intValue());
    assertEquals(collapseKey, json.get("collapse_key"));
    assertEquals(delayWhileIdle, json.get("delay_while_idle"));
    @SuppressWarnings("unchecked")
    Map<String, Object> payload = (Map<String, Object>) json.get("data");
    assertNotNull("no payload", payload);
    assertEquals("wrong payload size", 3, payload.size());
    assertEquals("v1", payload.get("k1"));
    assertEquals("v2", payload.get("k2"));
    assertEquals("v3", payload.get("k3"));
    JSONArray actualRegIds = (JSONArray) json.get("registration_ids");
    assertEquals("Wrong number of regIds",
        expectedRegIds.length, actualRegIds.size());
    for (int i = 0; i < expectedRegIds.length; i++) {
      String expectedRegId = expectedRegIds[i];
      String actualRegId = (String) actualRegIds.get(i);
      assertEquals("invalid regId at index " + i, expectedRegId, actualRegId);
    }
  }

  @Test
  public void testNewKeyValues() {
    Map<String, String> x = Sender.newKeyValues("key", "value");
    assertEquals(1, x.size());
    assertEquals("value", x.get("key"));
  }

  @Test(expected = IllegalArgumentException.class)
  public void testNewKeyValues_nullKey() {
    Sender.newKeyValues(null, "value");
  }

  @Test(expected = IllegalArgumentException.class)
  public void testNewKeyValues_nullValue() {
    Sender.newKeyValues("key", null);
  }

  @Test
  public void testNewBody() {
    StringBuilder body = Sender.newBody("name", "value");
    assertEquals("name=value", body.toString());
  }

  @Test(expected = IllegalArgumentException.class)
  public void testNewBody_nullKey() {
    Sender.newBody(null, "value");
  }

  @Test(expected = IllegalArgumentException.class)
  public void testNewBody_nullValue() {
    Sender.newBody("key", null);
  }

  @Test
  public void testAddParameter() {
    StringBuilder body = new StringBuilder("P=NP");
    Sender.addParameter(body, "name", "value");
    assertEquals("P=NP&name=value", body.toString());
  }

  @Test(expected = IllegalArgumentException.class)
  public void testAddParameter_nullBody() {
    Sender.addParameter(null, "key", "value");
  }

  @Test(expected = IllegalArgumentException.class)
  public void testAddParameter_nullKey() {
    StringBuilder body = new StringBuilder();
    Sender.addParameter(body, null, "value");
  }

  @Test(expected = IllegalArgumentException.class)
  public void testAddParameter_nullValue() {
    StringBuilder body = new StringBuilder();
    Sender.addParameter(body, "key", null);
  }

  @Test
  public void testGetString_oneLine() throws Exception {
    String expected = "108";
    InputStream stream = new ByteArrayInputStream(expected.getBytes());
    String actual = Sender.getString(stream);
    assertEquals(expected, actual);
  }

  @Test
  public void testGetString_stripsLastLine() throws Exception {
    InputStream stream = new ByteArrayInputStream("108\n".getBytes());
    String stripped = Sender.getString(stream);
    assertEquals("108", stripped);
  }

  @Test
  public void testGetString_multipleLines() throws Exception {
    String expected = "4\n8\n15\n\n16\n23\n42";
    InputStream stream = new ByteArrayInputStream(expected.getBytes());
    String actual = Sender.getString(stream);
    assertEquals(expected, actual);
  }

 @Test(expected = IllegalArgumentException.class)
  public void testGetString_nullValue() throws Exception {
    Sender.getString(null);
  }

  @Test(expected = IllegalArgumentException.class)
  public void testPost_noUrl() throws Exception {
    sender.post(null, "whatever");
  }

  @Test(expected = IllegalArgumentException.class)
  public void testPost_noBody() throws Exception {
    sender.post(Constants.GCM_SEND_ENDPOINT, null);
  }

  @Test
  public void testPost() throws Exception {
    String requestBody = "req";
    String responseBody = "resp";
    setResponseExpectations(200, responseBody);
    HttpURLConnection response =
        sender.post(Constants.GCM_SEND_ENDPOINT, requestBody);
    assertEquals(requestBody, new String(outputStream.toByteArray()));
    verify(mockedConn).setRequestMethod("POST");
    verify(mockedConn).setFixedLengthStreamingMode(requestBody.length());
    verify(mockedConn).setRequestProperty("Content-Type",
        "application/x-www-form-urlencoded;charset=UTF-8");
    verify(mockedConn).setRequestProperty("Authorization", "key=" + authKey);
    assertEquals(200, response.getResponseCode());
  }

  @Test
  public void testPost_customType() throws Exception {
    String requestBody = "req";
    String responseBody = "resp";
    setResponseExpectations(200, responseBody);
    HttpURLConnection response =
        sender.post(Constants.GCM_SEND_ENDPOINT, "stuff", requestBody);
    assertEquals(requestBody, new String(outputStream.toByteArray()));
    verify(mockedConn).setRequestMethod("POST");
    verify(mockedConn).setFixedLengthStreamingMode(requestBody.length());
    verify(mockedConn).setRequestProperty("Content-Type", "stuff");
    verify(mockedConn).setRequestProperty("Authorization", "key=" + authKey);
    assertEquals(200, response.getResponseCode());
  }

  /**
   * Sets the expectations of the HTTP connection.
   */
  private void setResponseExpectations(int statusCode, String response) 
      throws IOException {
    when(mockedConn.getResponseCode()).thenReturn(statusCode);
    InputStream inputStream = new ByteArrayInputStream(response.getBytes());
    if (statusCode == 200) {
      when(mockedConn.getInputStream()).thenReturn(inputStream);
    } else {
      when(mockedConn.getErrorStream()).thenReturn(inputStream);
    }
    when(mockedConn.getOutputStream()).thenReturn(outputStream);
    doReturn(mockedConn).when(sender)
        .getConnection(Constants.GCM_SEND_ENDPOINT);
  }

  private void doNotSleep() {
    doThrow(new AssertionError("Thou should not sleep!")).when(sender)
        .sleep(anyInt());
  }

  private void assertRequestBody() throws Exception {
    ArgumentCaptor<String> capturedBody = ArgumentCaptor.forClass(String.class);
    verify(sender).post(eq(Constants.GCM_SEND_ENDPOINT),
        capturedBody.capture());
    // parse body
    String body = capturedBody.getValue();
    Map<String, String> params = new HashMap<String, String>();
    for(String param : body.split("&")) {
      String[] split = param.split("=");
      params.put(split[0], split[1]);
    }
    // check parameters
    assertEquals(7, params.size());
    assertParameter(params, "registration_id", regId);
    assertParameter(params, "collapse_key", collapseKey);
    assertParameter(params, "delay_while_idle", delayWhileIdle ? "1" : "0");
    assertParameter(params, "time_to_live", "" + ttl);
    assertParameter(params, "data.k1", "v1");
    assertParameter(params, "data.k2", "v2");
    assertParameter(params, "data.k3", "v3");
  }

  static void assertParameter(Map<String, String> params, String name,
      String expectedValue) {
    assertEquals("invalid value for request parameter parameter " + name,
        params.get(name), expectedValue);
  }

}
