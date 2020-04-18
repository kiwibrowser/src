/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ipc.invalidation.external.client;

import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.external.client.types.Invalidation;
import com.google.ipc.invalidation.external.client.types.ObjectId;

/**
 * Listener Interface that must be implemented by  clients to receive object invalidations,
 * registration status events, and error events.
 * <p>
 * After the application publishes an invalidation (oid, version) to ,  guarantees to send
 * at least one of the following events to listeners that have registered for oid:
 * <ol>
 *   <li> Invalidate(oid, version)
 *   <li> Invalidate(oid, laterVersion) where laterVersion >= version
 *   <li> InvalidateUnknownVersion(oid)
 * </ol>
 * <p>
 * Each invalidation must be acknowledged by the application. Each includes an AckHandle that
 * the application must use to call {@link InvalidationClient#acknowledge} after it is done handling
 * that event.
 * <p>
 * Please see http://go/-api for additional information on the  API and semantics.
 * <p>
 * Please see {@link com.google.ipc.invalidation.external.client.contrib.SimpleListener} for a
 * base class that implements some events on behalf of the application.
 * <p>
 */
public interface InvalidationListener {
  /** Possible registration states for an object. */
  public enum RegistrationState {
    REGISTERED,
    UNREGISTERED
  }

  /**
   * Called in response to the {@code InvalidationClient.start} call. Indicates that the
   * InvalidationClient is now ready for use, i.e., calls such as register/unregister can be
   * performed on that object.
   * <p>
   * The application MUST NOT issue calls such as register/unregister on the InvalidationClient
   * before receiving this event.
   *
   * @param client the {@link InvalidationClient} invoking the listener
   */
  void ready(InvalidationClient client);

  /**
   * Indicates that an object has been updated to a particular version.
   * <ul>
   *   <li> When it receives this call, the application MUST hit its backend to fetch the
   *   updated state of the object, unless it already has has a version at least as recent as that
   *   of the invalidation.
   *
   *   <li>  MAY choose to drop older versions of invalidations, as long as it calls
   *   {@link #invalidate} with a later version of the same object, or calls
   *   {@link #invalidateUnknownVersion}.
   *
   *   <li>  MAY reorder or duplicate invalidations.
   *
   *   <li>  MAY drop a published payload without notice.
   *
   *   <li> The application MUST acknowledge this event by calling
   *   {@link InvalidationClient#acknowledge} with the provided {@code ackHandle}, otherwise the
   *   event will be redelivered.
   * </ul>
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param ackHandle event acknowledgement handle
   */
  void invalidate(InvalidationClient client, Invalidation invalidation, AckHandle ackHandle);

  /**
   * Indicates that an object has been updated, but the version number and payload are unknown.
   *
   * <ul>
   *   <li> When it receives this call, the application MUST hit its backend to fetch the updated
   *   state of the object, regardless of what version it has in its cache.
   *
   *   <li> The application MUST acknowledge this event by calling
   *   {@link InvalidationClient#acknowledge} with the provided {@code ackHandle}, otherwise the
   *   event will be redelivered.
   * </ul>
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param ackHandle event acknowledgement handle
   */
  void invalidateUnknownVersion(InvalidationClient client, ObjectId objectId,
      AckHandle ackHandle);

  /**
   * Indicates that the application should consider all objects to have changed. This event is sent
   * extremely rarely.
   *
   * <ul>
   *   <li> The application MUST hit its backend to fetch the updated state of all objects,
   *   regardless of what version it has in its cache.
   *
   *   <li> The application MUST acknowledge this event by calling
   *   {@link InvalidationClient#acknowledge} with the provided {@code ackHandle}, otherwise the
   *   event will be redelivered.
   * </ul>
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param ackHandle event acknowledgement handle
   */
  void invalidateAll(InvalidationClient client, AckHandle ackHandle);

  /**
   * Indicates that the registration state of an object has changed.
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param objectId the id of the object whose state changed
   * @param regState the new state
   */
  void informRegistrationStatus(InvalidationClient client, ObjectId objectId,
      RegistrationState regState);

  /**
   * Indicates that an object registration or unregistration operation may have failed.
   * <p>
   * For transient failures, the application MAY retry the registration later. If it chooses to do
   * so, it MUST use exponential backoff or other sensible backoff policy..
   * <p>
   * For permanent failures, the application MUST NOT automatically retry without fixing the
   * situation (e.g., by presenting a dialog box to the user).
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param objectId the id of the object whose state changed
   * @param isTransient whether the error is transient or permanent
   * @param errorMessage extra information about the message
   */
  void informRegistrationFailure(InvalidationClient client, ObjectId objectId,
      boolean isTransient, String errorMessage);

  /**
   * Indicates that all registrations for the client are in an unknown state (e.g.,  may have
   * dropped registrations.)
   *
   * The application MUST call {@link InvalidationClient#register} for all objects that it wishes
   * to be registered for.
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param ignored clients can ignore this argument.
   * @param ignored2 clients can ignore this argument.
   */
  void reissueRegistrations(InvalidationClient client, byte[] ignored, int ignored2);

  /**
   * Informs the listener about errors that have occurred in the backend.
   *
   * If the error reason is AUTH_FAILURE, the application may notify the user.
   * Otherwise, the application should log the error info for debugging purposes but take no
   * other action.
   *
   * @param client the {@link InvalidationClient} invoking the listener
   * @param errorInfo information about the error
   */
  void informError(InvalidationClient client, ErrorInfo errorInfo);
}
