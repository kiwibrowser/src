// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.media.router.cast.CastMessageHandler.RequestRecord;
import org.chromium.chrome.browser.media.router.cast.JSONTestUtils.JSONObjectLike;
import org.chromium.chrome.browser.media.router.cast.JSONTestUtils.JSONStringLike;

import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Robolectric tests for CastSession.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastMessageHandlerTest {
    private static final String TAG = "MediaRouter";

    private static final String SESSION_ID = "SESSION_ID";
    private static final String INVALID_SESSION_ID = "INVALID_SESSION_ID";
    private static final String CLIENT_ID1 = "00000000000000001";
    private static final String CLIENT_ID2 = "00000000000000002";
    private static final String INVALID_CLIENT_ID = "xxxxxxxxxxxxxxxxx";
    private static final String NAMESPACE1 = "namespace1";
    private static final String NAMESPACE2 = "namespace2";
    private static final String MEDIA_NAMESPACE = CastSessionUtil.MEDIA_NAMESPACE;
    private static final int SEQUENCE_NUMBER1 = 1;
    private static final int SEQUENCE_NUMBER2 = 2;
    private static final int REQUEST_ID1 = 1;
    private static final int REQUEST_ID2 = 2;
    private static final int INVALID_SEQUENCE_NUMBER =
            CastMessageHandler.INVALID_SEQUENCE_NUMBER;
    private CastMediaRouteProvider mRouteProvider;
    private CastSession mSession;
    private CastMessageHandler mMessageHandler;
    private int mNumStopApplicationCalled = 0;

    private interface CheckedRunnable { void run() throws Exception; }

    @Before
    public void setUp() {
        ShadowLog.stream = System.out;
        mRouteProvider = mock(CastMediaRouteProvider.class);
        mSession = mock(CastSession.class);
        doReturn(SESSION_ID).when(mSession).getSessionId();
        doReturn(true).when(mSession).sendStringCastMessage(
                anyString(), anyString(), anyString(), anyInt());
        mMessageHandler = spy(new CastMessageHandler(mRouteProvider));
        mMessageHandler.onSessionCreated(mSession);
        final Set<String> clientIds = new HashSet<String>();
        clientIds.add(CLIENT_ID1);
        clientIds.add(CLIENT_ID2);
        doAnswer(new Answer() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                return clientIds;
            }
        })
                .when(mRouteProvider)
                .getClients();
        doNothing().when(mRouteProvider).onMessage(anyString(), anyString());
        doNothing().when(mRouteProvider).onMessageSentResult(anyBoolean(), anyInt());
    }

    void setUpForAppMessageTest() throws JSONException {
        Set<String> namespaces = new HashSet<String>();
        namespaces.add(NAMESPACE1);
        doReturn(namespaces).when(mSession).getNamespaces();
        doReturn(true).when(mMessageHandler)
                .sendJsonCastMessage(any(JSONObject.class), anyString(), anyString(), anyInt());
    }


    @Test
    @Feature({"MediaRouter"})
    public void testOnSessionCreated() {
        Map<String, ClientRecord> clientRecords = new HashMap<String, ClientRecord>();
        clientRecords.put(CLIENT_ID1, new ClientRecord(
                "DONTCARE", CLIENT_ID1, "DONTCARE", "DONTCARE", "DONTCARE", 0));
        clientRecords.put(CLIENT_ID2, new ClientRecord(
                "DONTCARE", CLIENT_ID2, "DONTCARE", "DONTCARE", "DONTCARE", 0));
        clientRecords.get(CLIENT_ID1).isConnected = true;
        doReturn(clientRecords).when(mRouteProvider).getClientRecords();
        // The call in setUp() actually only sets the session. This call will notify the clients
        // that have sent "client_connect"
        mMessageHandler.onSessionCreated(mSession);
        verify(mSession).onClientConnected(CLIENT_ID1);
        verify(mSession, never()).onClientConnected(CLIENT_ID2);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testHandleSessionMessageOfV2MessageType() throws JSONException {
        doReturn(true).when(mMessageHandler).handleCastV2Message(any(JSONObject.class));

        JSONObject message = new JSONObject();
        message.put("type", "v2_message");
        assertTrue(mMessageHandler.handleSessionMessage(message));
        verify(mMessageHandler).handleCastV2Message(argThat(new JSONObjectLike(message)));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testHandleSessionMessageOfAppMessageType() throws JSONException {
        doReturn(true).when(mMessageHandler).handleAppMessage(any(JSONObject.class));

        JSONObject message = new JSONObject();
        message.put("type", "app_message");
        assertTrue(mMessageHandler.handleSessionMessage(message));
        verify(mMessageHandler).handleAppMessage(argThat(new JSONObjectLike(message)));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testHandleSessionMessageOfUnsupportedType() throws JSONException {
        doReturn(true).when(mMessageHandler).handleCastV2Message(any(JSONObject.class));

        JSONObject message = new JSONObject();
        message.put("type", "unsupported");
        assertFalse(mMessageHandler.handleSessionMessage(message));
        verify(mMessageHandler, never()).handleCastV2Message(any(JSONObject.class));
        verify(mMessageHandler, never()).handleAppMessage(any(JSONObject.class));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageWithWrongTypeInnerMessage() throws JSONException {
        JSONObject innerMessage = new JSONObject()
                .put("type", "STOP");
        final JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        // Replace the inner JSON message with string.
        message.put("message", "wrong type inner message");
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleCastV2Message(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).handleStopMessage(anyString(), anyInt());
        verify(mSession, never()).handleVolumeMessage(any(JSONObject.class), anyString(), anyInt());
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageOfStopType() throws JSONException {
        JSONObject innerMessage = new JSONObject()
                .put("type", "STOP");
        JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        assertTrue(mMessageHandler.handleCastV2Message(message));
        verify(mMessageHandler).handleStopMessage(eq(CLIENT_ID1), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageofSetVolumeTypeShouldWait() throws JSONException {
        doReturn(new CastSession.HandleVolumeMessageResult(true, true))
                .when(mSession).handleVolumeMessage(any(JSONObject.class), anyString(), anyInt());
        JSONObject innerMessage = new JSONObject()
                .put("type", "SET_VOLUME")
                .put("volume", new JSONObject()
                     .put("level", (double) 1)
                     .put("muted", false));
        JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        assertTrue(mMessageHandler.handleCastV2Message(message));
        JSONObject volumeMessage = innerMessage.getJSONObject("volume");
        verify(mSession).handleVolumeMessage(
                argThat(new JSONObjectLike(innerMessage.getJSONObject("volume"))),
                eq(CLIENT_ID1), eq(SEQUENCE_NUMBER1));
        assertEquals(mMessageHandler.getVolumeRequestsForTest().size(), 1);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageofSetVolumeTypeShouldNotWait() throws JSONException {
        doReturn(new CastSession.HandleVolumeMessageResult(true, false))
                .when(mSession).handleVolumeMessage(any(JSONObject.class), anyString(), anyInt());
        JSONObject innerMessage = new JSONObject()
                .put("type", "SET_VOLUME")
                .put("volume", new JSONObject()
                     .put("level", (double) 1)
                     .put("muted", false));
        JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        assertTrue(mMessageHandler.handleCastV2Message(message));
        JSONObject volumeMessage = innerMessage.getJSONObject("volume");
        verify(mSession).handleVolumeMessage(
                argThat(new JSONObjectLike(innerMessage.getJSONObject("volume"))),
                eq(CLIENT_ID1), eq(SEQUENCE_NUMBER1));
        assertEquals(mMessageHandler.getVolumeRequestsForTest().size(), 0);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageofSetVolumeTypeWithNullVolumeMessage() throws JSONException {
        JSONObject innerMessage = new JSONObject()
                .put("type", "SET_VOLUME");
        final JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleCastV2Message(message);
                }
            }, JSONException.class);
        verify(mSession, never()).handleVolumeMessage(any(JSONObject.class), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageofSetVolumeTypeWithWrongTypeVolumeMessage() throws JSONException {
        JSONObject innerMessage = new JSONObject()
                .put("type", "SET_VOLUME")
                .put("volume", "wrong type volume message");
        final JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleCastV2Message(message);
                }
            }, JSONException.class);
        verify(mSession, never()).handleVolumeMessage(any(JSONObject.class), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageOfMediaMessageType() throws JSONException {
        doReturn(true).when(mMessageHandler).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
        for (String messageType : CastMessageHandler.getMediaMessageTypesForTest()) {
            // TODO(zqzhang): SET_VOLUME and STOP should not reach here?
            if ("MEDIA_SET_VOLUME".equals(messageType) || "STOP_MEDIA".equals(messageType)) {
                continue;
            }
            JSONObject innerMessage = new JSONObject().put("type", messageType);
            JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
            assertTrue(mMessageHandler.handleCastV2Message(message));

            JSONObject expected = new JSONObject();
            if (CastMessageHandler.getMediaOverloadedMessageTypesForTest()
                    .containsKey(messageType)) {
                expected.put("type", CastMessageHandler.getMediaOverloadedMessageTypesForTest()
                        .get(messageType));
            } else {
                expected.put("type", messageType);
            }
            verify(mMessageHandler)
                    .sendJsonCastMessage(argThat(new JSONObjectLike(expected)),
                            eq(CastSessionUtil.MEDIA_NAMESPACE), eq(CLIENT_ID1),
                            eq(SEQUENCE_NUMBER1));
        }
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastV2MessageWithNullSequenceNumber() throws JSONException {
        JSONObject innerMessage = new JSONObject()
                .put("type", "STOP");
        JSONObject message = buildCastV2Message(CLIENT_ID1, innerMessage);
        message.remove("sequenceNumber");
        assertTrue(mMessageHandler.handleCastV2Message(message));
        verify(mMessageHandler).handleStopMessage(eq(CLIENT_ID1), eq(INVALID_SEQUENCE_NUMBER));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testHandleStopMessage() throws JSONException {
        assertEquals(0, mMessageHandler.getStopRequestsForTest().size());
        mMessageHandler.handleStopMessage(CLIENT_ID1, SEQUENCE_NUMBER1);
        assertEquals(1, mMessageHandler.getStopRequestsForTest().get(CLIENT_ID1).size(), 1);
        verify(mSession).stopApplication();
        mMessageHandler.handleStopMessage(CLIENT_ID1, SEQUENCE_NUMBER2);
        assertEquals(2, mMessageHandler.getStopRequestsForTest().get(CLIENT_ID1).size(), 2);
        verify(mSession, times(2)).stopApplication();
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithExistingNamespace() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        assertTrue(mMessageHandler.handleAppMessage(message));
        verify(mMessageHandler).sendJsonCastMessage(
                argThat(new JSONObjectLike(actualMessage)), eq(NAMESPACE1),
                eq(CLIENT_ID1), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithNonexistingNamespace() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE2, actualMessage);
        assertFalse(mMessageHandler.handleAppMessage(message));
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithoutSequenceNumber() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.remove("sequenceNumber");
        assertTrue(mMessageHandler.handleAppMessage(message));
        verify(mMessageHandler).sendJsonCastMessage(
                argThat(new JSONObjectLike(actualMessage)), eq(NAMESPACE1),
                eq(CLIENT_ID1), eq(INVALID_SEQUENCE_NUMBER));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithNullSessionId() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        final JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.getJSONObject("message").remove("sessionId");
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleAppMessage(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithWrongSessionId() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.getJSONObject("message").put("sessionId", INVALID_SESSION_ID);
        assertFalse(mMessageHandler.handleAppMessage(message));
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithNullActualMessage() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        final JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.getJSONObject("message").remove("message");
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleAppMessage(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithStringMessage() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.getJSONObject("message").put("message", "string message");
        assertTrue(mMessageHandler.handleAppMessage(message));
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
        verify(mSession).sendStringCastMessage(
                eq("string message"), eq(NAMESPACE1), eq(CLIENT_ID1), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithNullAppMessage() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        final JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.remove("message");
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleAppMessage(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithEmptyAppMessage() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        final JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.put("message", new JSONObject());
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleAppMessage(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithWrongTypeAppMessage() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        final JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.put("message", "wrong type app message");
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleAppMessage(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithNullClient() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        final JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.remove("clientId");
        expectException(new CheckedRunnable() {
                @Override
                public void run() throws Exception {
                    mMessageHandler.handleAppMessage(message);
                }
            }, JSONException.class);
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAppMessageWithNonexistingClient() throws JSONException {
        setUpForAppMessageTest();

        JSONObject actualMessage = buildActualAppMessage();
        JSONObject message = buildAppMessage(CLIENT_ID1, NAMESPACE1, actualMessage);
        message.put("clientId", INVALID_CLIENT_ID);
        assertFalse(mMessageHandler.handleAppMessage(message));
        verify(mMessageHandler, never()).sendJsonCastMessage(
                any(JSONObject.class), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSendJsonCastMessage() throws JSONException {
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        JSONObject message = buildJsonCastMessage("message");
        assertTrue(mMessageHandler.sendJsonCastMessage(
                message, NAMESPACE1, CLIENT_ID1, SEQUENCE_NUMBER1));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        verify(mSession).sendStringCastMessage(
                argThat(new JSONStringLike(message)), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSendJsonCastMessageWhenApiClientInvalid() throws JSONException {
        doReturn(true).when(mSession).isApiClientInvalid();

        JSONObject message = buildJsonCastMessage("message");
        assertFalse(mMessageHandler.sendJsonCastMessage(
                message, NAMESPACE1, CLIENT_ID1, SEQUENCE_NUMBER1));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        verify(mSession, never()).sendStringCastMessage(
                anyString(), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSendJsonCastMessageWithInvalidSequenceNumber() throws JSONException {
        JSONObject message = buildJsonCastMessage("message");
        assertTrue(mMessageHandler.sendJsonCastMessage(
                message, NAMESPACE1, CLIENT_ID1, INVALID_SEQUENCE_NUMBER));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        verify(mSession).sendStringCastMessage(
                argThat(new JSONStringLike(message)), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSendJsonCastMessageWithNullRequestId() throws JSONException {
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        JSONObject message = buildJsonCastMessage("message");
        message.remove("requestId");
        assertTrue(mMessageHandler.sendJsonCastMessage(
                message, NAMESPACE1, CLIENT_ID1, SEQUENCE_NUMBER1));
        assertTrue(message.has("requestId"));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        verify(mSession).sendStringCastMessage(
                argThat(new JSONStringLike(message)), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMessageReceivedWithExistingRequestId() throws JSONException {
        doNothing().when(mMessageHandler).onAppMessage(
                anyString(), anyString(), any(RequestRecord.class));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        RequestRecord request = new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1);
        mMessageHandler.getRequestsForTest().append(
                REQUEST_ID1, request);
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        JSONObject message = new JSONObject();
        message.put("requestId", REQUEST_ID1);
        mMessageHandler.onMessageReceived(NAMESPACE1, message.toString());
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        verify(mMessageHandler).onAppMessage(eq(message.toString()), eq(NAMESPACE1), eq(request));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMessageReceivedWithNonexistingRequestId() throws JSONException {
        doNothing().when(mMessageHandler).onAppMessage(
                anyString(), anyString(), any(RequestRecord.class));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        RequestRecord request = new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1);
        mMessageHandler.getRequestsForTest().append(
                REQUEST_ID1, request);
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        JSONObject message = new JSONObject();
        message.put("requestId", REQUEST_ID2);
        mMessageHandler.onMessageReceived(NAMESPACE1, message.toString());
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        verify(mMessageHandler).onAppMessage(
                eq(message.toString()), eq(NAMESPACE1), (RequestRecord) isNull());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMessageReceivedWithoutRequestId() throws JSONException {
        doNothing().when(mMessageHandler).onAppMessage(
                anyString(), anyString(), any(RequestRecord.class));
        assertEquals(mMessageHandler.getRequestsForTest().size(), 0);
        RequestRecord request = new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1);
        mMessageHandler.getRequestsForTest().append(
                REQUEST_ID1, request);
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        JSONObject message = new JSONObject();
        mMessageHandler.onMessageReceived(NAMESPACE1, message.toString());
        assertEquals(mMessageHandler.getRequestsForTest().size(), 1);
        verify(mMessageHandler).onAppMessage(
                eq(message.toString()), eq(NAMESPACE1), (RequestRecord) isNull());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMessageReceivedOfMediaNamespace() throws JSONException {
        doNothing().when(mMessageHandler).onMediaMessage(anyString(), any(RequestRecord.class));
        mMessageHandler.onMessageReceived(MEDIA_NAMESPACE, "anymessage");
        verify(mMessageHandler).onMediaMessage(eq("anymessage"), (RequestRecord) isNull());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMediaMessageOfMediaStatusTypeWithRequestRecord() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        doReturn(true).when(mMessageHandler).isMediaStatusMessage(anyString());
        RequestRecord request = new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1);
        mMessageHandler.onMediaMessage("anymessage", request);
        verify(mMessageHandler, never()).sendClientMessageTo(
                eq(CLIENT_ID1), eq("v2_message"), eq("anymessage"), eq(INVALID_SEQUENCE_NUMBER));
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID2), eq("v2_message"), eq("anymessage"), eq(INVALID_SEQUENCE_NUMBER));
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("v2_message"), eq("anymessage"), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMediaMessageOfMediaStatusTypeWithNullRequestRecord() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        doReturn(true).when(mMessageHandler).isMediaStatusMessage(anyString());
        mMessageHandler.onMediaMessage("anymessage", null);
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("v2_message"), eq("anymessage"), eq(INVALID_SEQUENCE_NUMBER));
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID2), eq("v2_message"), eq("anymessage"), eq(INVALID_SEQUENCE_NUMBER));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMediaMessageOfNonMediaStatusTypeWithRequestRecord() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        doReturn(false).when(mMessageHandler).isMediaStatusMessage(anyString());
        RequestRecord request = new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1);
        mMessageHandler.onMediaMessage("anymessage", request);
        verify(mMessageHandler, never()).sendClientMessageTo(
                anyString(), anyString(), anyString(), eq(INVALID_SEQUENCE_NUMBER));
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("v2_message"), eq("anymessage"), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnMediaMessageOfNonMediaStatusTypeWithNullRequestRecord() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        doReturn(false).when(mMessageHandler).isMediaStatusMessage(anyString());
        mMessageHandler.onMediaMessage("anymessage", null);
        verify(mMessageHandler, never()).sendClientMessageTo(
                anyString(), anyString(), anyString(), eq(INVALID_SEQUENCE_NUMBER));
        verify(mMessageHandler, never()).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnAppMessageWithRequestRecord() throws JSONException {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        RequestRecord request = new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1);
        mMessageHandler.onAppMessage("anyMessage", NAMESPACE1, request);
        JSONObject expected = new JSONObject();
        expected.put("sessionId", SESSION_ID);
        expected.put("namespaceName", NAMESPACE1);
        expected.put("message", "anyMessage");
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("app_message"),
                argThat(new JSONStringLike(expected)), eq(SEQUENCE_NUMBER1));
        verify(mMessageHandler, never()).broadcastClientMessage(anyString(), anyString());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnAppMessageWithNullRequestRecord() throws JSONException {
        doNothing().when(mMessageHandler).broadcastClientMessage(anyString(), anyString());
        mMessageHandler.onAppMessage("anyMessage", NAMESPACE1, null);
        JSONObject expected = new JSONObject();
        expected.put("sessionId", SESSION_ID);
        expected.put("namespaceName", NAMESPACE1);
        expected.put("message", "anyMessage");
        verify(mMessageHandler, never()).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        verify(mMessageHandler).broadcastClientMessage(
                eq("app_message"), argThat(new JSONStringLike(expected)));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnApplicationStopped() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        assertEquals(0, mMessageHandler.getStopRequestsForTest().size());
        mMessageHandler.getStopRequestsForTest().put(CLIENT_ID1, new ArrayDeque<Integer>());
        mMessageHandler.getStopRequestsForTest().get(CLIENT_ID1).add(SEQUENCE_NUMBER1);
        mMessageHandler.getStopRequestsForTest().get(CLIENT_ID1).add(SEQUENCE_NUMBER2);
        assertEquals(1, mMessageHandler.getStopRequestsForTest().size());
        assertEquals(2, mMessageHandler.getStopRequestsForTest().get(CLIENT_ID1).size());

        mMessageHandler.onApplicationStopped();
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("remove_session"), eq(SESSION_ID), eq(SEQUENCE_NUMBER1));
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("remove_session"), eq(SESSION_ID), eq(SEQUENCE_NUMBER2));
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID2), eq("remove_session"), eq(SESSION_ID), eq(INVALID_SEQUENCE_NUMBER));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnVolumeChanged() {
        doNothing().when(mMessageHandler).onVolumeChanged(anyString(), anyInt());
        assertEquals(0, mMessageHandler.getVolumeRequestsForTest().size());
        mMessageHandler.getVolumeRequestsForTest()
                .add(new RequestRecord(CLIENT_ID1, SEQUENCE_NUMBER1));
        assertEquals(1, mMessageHandler.getVolumeRequestsForTest().size());

        mMessageHandler.onVolumeChanged();
        verify(mMessageHandler).onVolumeChanged(CLIENT_ID1, SEQUENCE_NUMBER1);
        assertEquals(0, mMessageHandler.getVolumeRequestsForTest().size());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnVolumeChangedWithEmptyVolumeRequests() {
        mMessageHandler.onVolumeChanged();
        verify(mMessageHandler, never()).onVolumeChanged(eq(CLIENT_ID1), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnVolumeChangedForClient() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        mMessageHandler.onVolumeChanged(CLIENT_ID1, SEQUENCE_NUMBER1);
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("v2_message"), (String) isNull(), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnAppMessageSent() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        mMessageHandler.onAppMessageSent(CLIENT_ID1, SEQUENCE_NUMBER1);
        verify(mMessageHandler).sendClientMessageTo(
                eq(CLIENT_ID1), eq("app_message"), (String) isNull(), eq(SEQUENCE_NUMBER1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSendClientMessageTo() throws JSONException {
        String message = "discarded_message1";
        doReturn(message).when(mMessageHandler)
                .buildInternalMessage(anyString(), anyString(), anyString(), anyInt());
        mMessageHandler.sendClientMessageTo(
                CLIENT_ID1, "anytype", "discarded_message2", SEQUENCE_NUMBER1);

        verify(mRouteProvider).onMessage(
                eq(CLIENT_ID1), eq(message));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testBroadcastClientMessage() {
        doNothing().when(mMessageHandler).sendClientMessageTo(
                anyString(), anyString(), anyString(), anyInt());
        mMessageHandler.broadcastClientMessage("anytype", "anymessage");
        for (String clientId : mRouteProvider.getClients()) {
            verify(mMessageHandler).sendClientMessageTo(
                    eq(clientId), eq("anytype"), eq("anymessage"), eq(INVALID_SEQUENCE_NUMBER));
        }
    }

    @Test
    @Feature({"MediaRouter"})
    public void testBuildInternalMessageWithNullMessage() throws JSONException {
        String message = mMessageHandler.buildInternalMessage(
                "anytype", null, CLIENT_ID1, SEQUENCE_NUMBER1);
        JSONObject expected = new JSONObject();
        expected.put("type", "anytype");
        expected.put("sequenceNumber", SEQUENCE_NUMBER1);
        expected.put("timeoutMillis", 0);
        expected.put("clientId", CLIENT_ID1);
        expected.put("message", null);

        assertTrue("\nexpected: " + expected.toString() + ",\n  actual: " + message.toString(),
                new JSONObjectLike(expected).matches(new JSONObject(message)));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testBuildInternalMessageOfRemoveSessionType() throws JSONException {
        String message = mMessageHandler.buildInternalMessage(
                "remove_session", SESSION_ID, CLIENT_ID1, SEQUENCE_NUMBER1);
        JSONObject expected = new JSONObject();
        expected.put("type", "remove_session");
        expected.put("sequenceNumber", SEQUENCE_NUMBER1);
        expected.put("timeoutMillis", 0);
        expected.put("clientId", CLIENT_ID1);
        expected.put("message", SESSION_ID);

        assertTrue("\nexpected: " + expected.toString() + ",\n  actual: " + message.toString(),
                new JSONObjectLike(expected).matches(new JSONObject(message)));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testBuildInternalMessageOfDisconnectSessionType() throws JSONException {
        String message = mMessageHandler.buildInternalMessage(
                "disconnect_session", SESSION_ID, CLIENT_ID1, SEQUENCE_NUMBER1);
        JSONObject expected = new JSONObject();
        expected.put("type", "disconnect_session");
        expected.put("sequenceNumber", SEQUENCE_NUMBER1);
        expected.put("timeoutMillis", 0);
        expected.put("clientId", CLIENT_ID1);
        expected.put("message", SESSION_ID);

        assertTrue("\nexpected: " + expected.toString() + ",\n  actual: " + message.toString(),
                new JSONObjectLike(expected).matches(new JSONObject(message)));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testBuildInternalMessageWithInnerMessage() throws JSONException {
        JSONObject innerMessage = buildSessionMessage(SESSION_ID);
        String message = mMessageHandler.buildInternalMessage(
                "anytype", innerMessage.toString(), CLIENT_ID1, SEQUENCE_NUMBER1);
        JSONObject expected = new JSONObject();
        expected.put("type", "anytype");
        expected.put("sequenceNumber", SEQUENCE_NUMBER1);
        expected.put("timeoutMillis", 0);
        expected.put("clientId", CLIENT_ID1);
        expected.put("message", innerMessage);

        assertTrue("\nexpected: " + expected.toString() + ",\n  actual: " + message.toString(),
                new JSONObjectLike(expected).matches(new JSONObject(message)));
    }

    JSONObject buildCastV2Message(String clientId, JSONObject innerMessage) throws JSONException {
        JSONObject message = new JSONObject();
        message.put("type", "v2_message");
        message.put("message", innerMessage);
        message.put("sequenceNumber", SEQUENCE_NUMBER1);
        message.put("timeoutMillis", 0);
        message.put("clientId", clientId);

        return message;
    }

    JSONObject buildAppMessage(String clientId, String namespace, Object actualMessage)
            throws JSONException {
        JSONObject innerMessage = new JSONObject();
        innerMessage.put("sessionId", mSession.getSessionId());
        innerMessage.put("namespaceName", namespace);
        innerMessage.put("message", actualMessage);

        JSONObject message = new JSONObject();
        message.put("type", "app_message");
        message.put("message", innerMessage);
        message.put("sequenceNumber", SEQUENCE_NUMBER1);
        message.put("timeoutMillis", 0);
        message.put("clientId", clientId);

        return message;
    }

    JSONObject buildActualAppMessage() throws JSONException {
        JSONObject message = new JSONObject();
        message.put("type", "actual app message type");

        return message;
    }

    JSONObject buildSessionMessage(String sessionId) throws JSONException {
        JSONObject message = new JSONObject();
        message.put("sessionId", sessionId);

        return message;
    }

    void expectException(CheckedRunnable r, Class exceptionClass) {
        boolean caughtException = false;
        try {
            r.run();
        } catch (Exception e) {
            if (e.getClass() == exceptionClass) caughtException = true;
        }
        assertTrue(caughtException);
    }

    JSONObject buildJsonCastMessage(String message) throws JSONException {
        JSONObject jsonMessage = new JSONObject();
        jsonMessage.put("requestId", REQUEST_ID1);
        jsonMessage.put("message", message);
        return jsonMessage;
    }
}
