// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import android.support.v7.media.MediaRouter;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.media.router.ChromeMediaRouter;
import org.chromium.chrome.browser.media.router.MediaRoute;
import org.chromium.chrome.browser.media.router.MediaRouteManager;
import org.chromium.chrome.browser.media.router.MediaRouteProvider;
import org.chromium.chrome.browser.media.router.MediaSink;
import org.chromium.chrome.browser.media.router.MediaSource;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * A {@link MediaRouteProvider} implementation for Cast devices and applications.
 */
public class CastMediaRouteProvider extends BaseMediaRouteProvider {
    private static final String TAG = "MediaRouter";

    private static final String AUTO_JOIN_PRESENTATION_ID = "auto-join";
    private static final String PRESENTATION_ID_SESSION_ID_PREFIX = "cast-session_";

    private final CastMessageHandler mMessageHandler;
    private ClientRecord mLastRemovedRouteRecord;
    private final Map<String, ClientRecord> mClientRecords = new HashMap<String, ClientRecord>();

    /**
     * @return Initialized {@link CastMediaRouteProvider} object.
     */
    public static CastMediaRouteProvider create(MediaRouteManager manager) {
        return new CastMediaRouteProvider(ChromeMediaRouter.getAndroidMediaRouter(), manager);
    }

    @Override
    public void onSessionStartFailed() {
        super.onSessionStartFailed();
        mClientRecords.clear();
    }

    @Override
    public void onSessionStarted(CastSession session) {
        super.onSessionStarted(session);
        mMessageHandler.onSessionCreated(mSession);
    }

    @Override
    public void onSessionEnded() {
        if (mSession == null) return;

        if (mClientRecords.isEmpty()) {
            for (String routeId : mRoutes.keySet()) mManager.onRouteClosed(routeId);
            mRoutes.clear();
        } else {
            mLastRemovedRouteRecord = mClientRecords.values().iterator().next();
            for (ClientRecord client : mClientRecords.values()) {
                mManager.onRouteClosed(client.routeId);

                mRoutes.remove(client.routeId);
            }
            mClientRecords.clear();
        }

        mSession = null;

        if (mAndroidMediaRouter != null) {
            mAndroidMediaRouter.selectRoute(mAndroidMediaRouter.getDefaultRoute());
        }
    }

    public void onMessageSentResult(boolean success, int callbackId) {
        mManager.onMessageSentResult(success, callbackId);
    }

    public void onMessage(String clientId, String message) {
        ClientRecord clientRecord = mClientRecords.get(clientId);
        if (clientRecord == null) return;

        if (!clientRecord.isConnected) {
            Log.d(TAG, "Queueing message to client %s: %s", clientId, message);
            clientRecord.pendingMessages.add(message);
            return;
        }

        Log.d(TAG, "Sending message to client %s: %s", clientId, message);
        mManager.onMessage(clientRecord.routeId, message);
    }

    public CastMessageHandler getMessageHandler() {
        return mMessageHandler;
    }

    public Set<String> getClients() {
        return mClientRecords.keySet();
    }

    public Map<String, ClientRecord> getClientRecords() {
        return mClientRecords;
    }

    @Override
    protected MediaSource getSourceFromId(String sourceId) {
        return CastMediaSource.from(sourceId);
    }

    @Override
    protected ChromeCastSessionManager.CastSessionLaunchRequest createSessionLaunchRequest(
            MediaSource source, MediaSink sink, String presentationId, String origin, int tabId,
            boolean isIncognito, int nativeRequestId) {
        return new CreateRouteRequest(source, sink, presentationId, origin, tabId, isIncognito,
                nativeRequestId, this, CreateRouteRequest.RequestedCastSessionType.CAST,
                mMessageHandler);
    }

    @Override
    public void onSessionStarting(
            ChromeCastSessionManager.CastSessionLaunchRequest sessionLaunchRequest) {
        CreateRouteRequest request = (CreateRouteRequest) sessionLaunchRequest;
        MediaSink sink = request.getSink();
        MediaSource source = request.getSource();

        MediaRoute route =
                new MediaRoute(sink.getId(), source.getSourceId(), request.getPresentationId());
        addRoute(route, request.getOrigin(), request.getTabId());
        mManager.onRouteCreated(route.id, route.sinkId, request.getNativeRequestId(), this, true);

        String clientId = ((CastMediaSource) source).getClientId();

        if (clientId != null) {
            ClientRecord clientRecord = mClientRecords.get(clientId);
            if (clientRecord != null) {
                sendReceiverAction(clientRecord.routeId, sink, clientId, "cast");
            }
        }
    }

    @Override
    public void joinRoute(String sourceId, String presentationId, String origin, int tabId,
            int nativeRequestId) {
        CastMediaSource source = CastMediaSource.from(sourceId);
        if (source == null || source.getClientId() == null) {
            mManager.onRouteRequestError("Unsupported presentation URL", nativeRequestId);
            return;
        }

        if (mSession == null) {
            mManager.onRouteRequestError("No presentation", nativeRequestId);
            return;
        }

        if (!canJoinExistingSession(presentationId, origin, tabId, source)) {
            mManager.onRouteRequestError("No matching route", nativeRequestId);
            return;
        }

        MediaRoute route = new MediaRoute(mSession.getSinkId(), sourceId, presentationId);
        addRoute(route, origin, tabId);
        mManager.onRouteCreated(route.id, route.sinkId, nativeRequestId, this, false);
    }

    @Override
    public void closeRoute(String routeId) {
        MediaRoute route = mRoutes.get(routeId);
        if (route == null) return;

        if (mSession == null) {
            mRoutes.remove(routeId);
            mManager.onRouteClosed(routeId);
            return;
        }

        ClientRecord client = getClientRecordByRouteId(routeId);
        if (client != null && mAndroidMediaRouter != null) {
            MediaSink sink = MediaSink.fromSinkId(mSession.getSinkId(), mAndroidMediaRouter);
            if (sink != null) sendReceiverAction(routeId, sink, client.clientId, "stop");
        }

        ChromeCastSessionManager.get().stopApplication();
    }

    @Override
    public void detachRoute(String routeId) {
        mRoutes.remove(routeId);

        removeClient(getClientRecordByRouteId(routeId));
    }

    @Override
    public void sendStringMessage(String routeId, String message, int nativeCallbackId) {
        Log.d(TAG, "Received message from client: %s", message);

        if (!mRoutes.containsKey(routeId)) {
            mManager.onMessageSentResult(false, nativeCallbackId);
            return;
        }

        boolean success = false;
        try {
            JSONObject jsonMessage = new JSONObject(message);

            String messageType = jsonMessage.getString("type");
            // TODO(zqzhang): Move the handling of "client_connect", "client_disconnect" and
            // "leave_session" from CastMRP to CastMessageHandler. Also, need to have a
            // ClientManager for client managing.
            if ("client_connect".equals(messageType)) {
                success = handleClientConnectMessage(jsonMessage);
            } else if ("client_disconnect".equals(messageType)) {
                success = handleClientDisconnectMessage(jsonMessage);
            } else if ("leave_session".equals(messageType)) {
                success = handleLeaveSessionMessage(jsonMessage);
            } else if (mSession != null) {
                success = mMessageHandler.handleSessionMessage(jsonMessage);
            }
        } catch (JSONException e) {
            Log.e(TAG, "JSONException while handling internal message: " + e);
            success = false;
        }

        mManager.onMessageSentResult(success, nativeCallbackId);
    }

    private boolean handleClientConnectMessage(JSONObject jsonMessage) throws JSONException {
        String clientId = jsonMessage.getString("clientId");
        if (clientId == null) return false;

        ClientRecord clientRecord = mClientRecords.get(clientId);
        if (clientRecord == null) return false;

        clientRecord.isConnected = true;
        if (mSession != null) mSession.onClientConnected(clientId);

        if (clientRecord.pendingMessages.size() == 0) return true;
        for (String message : clientRecord.pendingMessages) {
            Log.d(TAG, "Deqeueing message for client %s: %s", clientId, message);
            mManager.onMessage(clientRecord.routeId, message);
        }
        clientRecord.pendingMessages.clear();

        return true;
    }

    private boolean handleClientDisconnectMessage(JSONObject jsonMessage) throws JSONException {
        String clientId = jsonMessage.getString("clientId");
        if (clientId == null) return false;

        ClientRecord client = mClientRecords.get(clientId);
        if (client == null) return false;

        mRoutes.remove(client.routeId);
        removeClient(client);

        mManager.onRouteClosed(client.routeId);

        return true;
    }

    private boolean handleLeaveSessionMessage(JSONObject jsonMessage) throws JSONException {
        String clientId = jsonMessage.getString("clientId");
        if (clientId == null || mSession == null) return false;

        String sessionId = jsonMessage.getString("message");
        if (!mSession.getSessionId().equals(sessionId)) return false;

        ClientRecord leavingClient = mClientRecords.get(clientId);
        if (leavingClient == null) return false;

        int sequenceNumber = jsonMessage.optInt("sequenceNumber", -1);
        onMessage(clientId, buildInternalMessage("leave_session", sequenceNumber, clientId, null));

        // Send a "disconnect_session" message to all the clients that match with the leaving
        // client's auto join policy.
        for (ClientRecord client : mClientRecords.values()) {
            if ((CastMediaSource.AUTOJOIN_TAB_AND_ORIGIN_SCOPED.equals(leavingClient.autoJoinPolicy)
                        && isSameOrigin(client.origin, leavingClient.origin)
                        && client.tabId == leavingClient.tabId)
                    || (CastMediaSource.AUTOJOIN_ORIGIN_SCOPED.equals(leavingClient.autoJoinPolicy)
                               && isSameOrigin(client.origin, leavingClient.origin))) {
                onMessage(client.clientId,
                        buildInternalMessage("disconnect_session", -1, client.clientId, sessionId));
            }
        }

        return true;
    }

    private String buildInternalMessage(
            String type, int sequenceNumber, String clientId, String message) throws JSONException {
        JSONObject jsonMessage = new JSONObject();
        jsonMessage.put("type", type);
        jsonMessage.put("sequenceNumber", sequenceNumber);
        jsonMessage.put("timeoutMillis", 0);
        jsonMessage.put("clientId", clientId);
        jsonMessage.put("message", message);
        return jsonMessage.toString();
    }

    @VisibleForTesting
    CastMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        super(androidMediaRouter, manager);
        mMessageHandler = new CastMessageHandler(this);
    }

    private boolean canAutoJoin(CastMediaSource source, String origin, int tabId) {
        if (source.getAutoJoinPolicy().equals(CastMediaSource.AUTOJOIN_PAGE_SCOPED)) return false;

        CastMediaSource currentSource = CastMediaSource.from(mSession.getSourceId());
        if (!currentSource.getApplicationId().equals(source.getApplicationId())) return false;

        ClientRecord client = null;
        if (!mClientRecords.isEmpty()) {
            client = mClientRecords.values().iterator().next();
        } else if (mLastRemovedRouteRecord != null) {
            client = mLastRemovedRouteRecord;
            return isSameOrigin(origin, client.origin) && tabId == client.tabId;
        }
        if (client == null) return false;

        boolean sameOrigin = isSameOrigin(origin, client.origin);
        if (source.getAutoJoinPolicy().equals(CastMediaSource.AUTOJOIN_ORIGIN_SCOPED)) {
            return sameOrigin;
        } else if (source.getAutoJoinPolicy().equals(
                           CastMediaSource.AUTOJOIN_TAB_AND_ORIGIN_SCOPED)) {
            return sameOrigin && tabId == client.tabId;
        }

        return false;
    }

    private boolean canJoinExistingSession(
            String presentationId, String origin, int tabId, CastMediaSource source) {
        if (AUTO_JOIN_PRESENTATION_ID.equals(presentationId)) {
            return canAutoJoin(source, origin, tabId);
        } else if (presentationId.startsWith(PRESENTATION_ID_SESSION_ID_PREFIX)) {
            String sessionId = presentationId.substring(PRESENTATION_ID_SESSION_ID_PREFIX.length());
            if (mSession.getSessionId().equals(sessionId)) return true;
        } else {
            for (MediaRoute route : mRoutes.values()) {
                if (route.presentationId.equals(presentationId)) return true;
            }
        }
        return false;
    }

    @Nullable
    private ClientRecord getClientRecordByRouteId(String routeId) {
        for (ClientRecord record : mClientRecords.values()) {
            if (record.routeId.equals(routeId)) return record;
        }
        return null;
    }

    @VisibleForTesting
    void addRoute(MediaRoute route, String origin, int tabId) {
        mRoutes.put(route.id, route);

        CastMediaSource source = CastMediaSource.from(route.sourceId);
        final String clientId = source.getClientId();

        if (clientId == null || mClientRecords.get(clientId) != null) return;

        mClientRecords.put(clientId,
                new ClientRecord(
                        route.id,
                        clientId,
                        source.getApplicationId(),
                        source.getAutoJoinPolicy(),
                        origin,
                        tabId));
    }

    // TODO(zqzhang): Move this method to CastMessageHandler.
    private void sendReceiverAction(
            String routeId, MediaSink sink, String clientId, String action) {
        try {
            JSONObject jsonReceiver = new JSONObject();
            jsonReceiver.put("label", sink.getId());
            jsonReceiver.put("friendlyName", sink.getName());
            jsonReceiver.put("capabilities", CastSessionImpl.getCapabilities(sink.getDevice()));
            jsonReceiver.put("volume", null);
            jsonReceiver.put("isActiveInput", null);
            jsonReceiver.put("displayStatus", null);
            jsonReceiver.put("receiverType", "cast");

            JSONObject jsonReceiverAction = new JSONObject();
            jsonReceiverAction.put("receiver", jsonReceiver);
            jsonReceiverAction.put("action", action);

            JSONObject json = new JSONObject();
            json.put("type", "receiver_action");
            json.put("sequenceNumber", -1);
            json.put("timeoutMillis", 0);
            json.put("clientId", clientId);
            json.put("message", jsonReceiverAction);

            onMessage(clientId, json.toString());
        } catch (JSONException e) {
            Log.e(TAG, "Failed to send receiver action message", e);
        }
    }

    private void removeClient(@Nullable ClientRecord client) {
        if (client == null) return;

        mLastRemovedRouteRecord = client;
        mClientRecords.remove(client.clientId);
    }

    /**
     * Compares two origins. Empty origin strings correspond to unique origins in
     * url::Origin.
     *
     * @param originA A URL origin.
     * @param originB A URL origin.
     * @return True if originA and originB represent the same origin, false otherwise.
     */
    private static final boolean isSameOrigin(String originA, String originB) {
        if (originA == null || originA.isEmpty() || originB == null || originB.isEmpty())
            return false;
        return originA.equals(originB);
    }
}
