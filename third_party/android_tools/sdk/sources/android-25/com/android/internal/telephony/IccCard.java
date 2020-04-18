/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony;

import android.os.Handler;
import android.os.Message;

import com.android.internal.telephony.IccCardConstants.State;
import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccRecords;

/**
 * {@hide}
 * @Deprecated use UiccController.getUiccCard instead.
 *
 * Integrated Circuit Card (ICC) interface
 * An object of a class implementing this interface is used by external
 * apps (specifically PhoneApp) to perform icc card related functionality.
 *
 * Apps (those that have access to Phone object) can retrieve this object
 * by calling phone.getIccCard()
 *
 * This interface is implemented by IccCardProxy and the object PhoneApp
 * gets when it calls getIccCard is IccCardProxy.
 */
public interface IccCard {
    /**
     * @return combined Card and current App state
     */
    public State getState();

    /**
     * @return IccRecords object belonging to current UiccCardApplication
     */
    public IccRecords getIccRecords();

    /**
     * @return IccFileHandler object belonging to current UiccCardApplication
     */
    public IccFileHandler getIccFileHandler();

    /**
     * Notifies handler of any transition into IccCardConstants.State.ABSENT
     */
    public void registerForAbsent(Handler h, int what, Object obj);
    public void unregisterForAbsent(Handler h);

    /**
     * Notifies handler of any transition into IccCardConstants.State.NETWORK_LOCKED
     */
    public void registerForNetworkLocked(Handler h, int what, Object obj);
    public void unregisterForNetworkLocked(Handler h);

    /**
     * Notifies handler of any transition into IccCardConstants.State.isPinLocked()
     */
    public void registerForLocked(Handler h, int what, Object obj);
    public void unregisterForLocked(Handler h);

    /**
     * Supply the ICC PIN to the ICC
     *
     * When the operation is complete, onComplete will be sent to its
     * Handler.
     *
     * onComplete.obj will be an AsyncResult
     *
     * ((AsyncResult)onComplete.obj).exception == null on success
     * ((AsyncResult)onComplete.obj).exception != null on fail
     *
     * If the supplied PIN is incorrect:
     * ((AsyncResult)onComplete.obj).exception != null
     * && ((AsyncResult)onComplete.obj).exception
     *       instanceof com.android.internal.telephony.gsm.CommandException)
     * && ((CommandException)(((AsyncResult)onComplete.obj).exception))
     *          .getCommandError() == CommandException.Error.PASSWORD_INCORRECT
     */
    public void supplyPin (String pin, Message onComplete);

    /**
     * Supply the ICC PUK to the ICC
     */
    public void supplyPuk (String puk, String newPin, Message onComplete);

    /**
     * Supply the ICC PIN2 to the ICC
     */
    public void supplyPin2 (String pin2, Message onComplete);

    /**
     * Supply the ICC PUK2 to the ICC
     */
    public void supplyPuk2 (String puk2, String newPin2, Message onComplete);

    /**
     * Check whether fdn (fixed dialing number) service is available.
     * @return true if ICC fdn service available
     *         false if ICC fdn service not available
    */
    public boolean getIccFdnAvailable();

    /**
     * Supply Network depersonalization code to the RIL
     */
    public void supplyNetworkDepersonalization (String pin, Message onComplete);

    /**
     * Check whether ICC pin lock is enabled
     * This is a sync call which returns the cached pin enabled state
     *
     * @return true for ICC locked enabled
     *         false for ICC locked disabled
     */
    public boolean getIccLockEnabled();

    /**
     * Check whether ICC fdn (fixed dialing number) is enabled
     * This is a sync call which returns the cached pin enabled state
     *
     * @return true for ICC fdn enabled
     *         false for ICC fdn disabled
     */
    public boolean getIccFdnEnabled();

     /**
      * Set the ICC pin lock enabled or disabled
      * When the operation is complete, onComplete will be sent to its handler
      *
      * @param enabled "true" for locked "false" for unlocked.
      * @param password needed to change the ICC pin state, aka. Pin1
      * @param onComplete
      *        onComplete.obj will be an AsyncResult
      *        ((AsyncResult)onComplete.obj).exception == null on success
      *        ((AsyncResult)onComplete.obj).exception != null on fail
      */
     public void setIccLockEnabled (boolean enabled,
             String password, Message onComplete);

     /**
      * Set the ICC fdn enabled or disabled
      * When the operation is complete, onComplete will be sent to its handler
      *
      * @param enabled "true" for locked "false" for unlocked.
      * @param password needed to change the ICC fdn enable, aka Pin2
      * @param onComplete
      *        onComplete.obj will be an AsyncResult
      *        ((AsyncResult)onComplete.obj).exception == null on success
      *        ((AsyncResult)onComplete.obj).exception != null on fail
      */
     public void setIccFdnEnabled (boolean enabled,
             String password, Message onComplete);

     /**
      * Change the ICC password used in ICC pin lock
      * When the operation is complete, onComplete will be sent to its handler
      *
      * @param oldPassword is the old password
      * @param newPassword is the new password
      * @param onComplete
      *        onComplete.obj will be an AsyncResult
      *        ((AsyncResult)onComplete.obj).exception == null on success
      *        ((AsyncResult)onComplete.obj).exception != null on fail
      */
     public void changeIccLockPassword(String oldPassword, String newPassword,
             Message onComplete);

     /**
      * Change the ICC password used in ICC fdn enable
      * When the operation is complete, onComplete will be sent to its handler
      *
      * @param oldPassword is the old password
      * @param newPassword is the new password
      * @param onComplete
      *        onComplete.obj will be an AsyncResult
      *        ((AsyncResult)onComplete.obj).exception == null on success
      *        ((AsyncResult)onComplete.obj).exception != null on fail
      */
     public void changeIccFdnPassword(String oldPassword, String newPassword,
             Message onComplete);

    /**
     * Returns service provider name stored in ICC card.
     * If there is no service provider name associated or the record is not
     * yet available, null will be returned <p>
     *
     * Please use this value when display Service Provider Name in idle mode <p>
     *
     * Usage of this provider name in the UI is a common carrier requirement.
     *
     * Also available via Android property "gsm.sim.operator.alpha"
     *
     * @return Service Provider Name stored in ICC card
     *         null if no service provider name associated or the record is not
     *         yet available
     *
     */
    public String getServiceProviderName ();

    /**
     * Checks if an Application of specified type present on the card
     * @param type is AppType to look for
     */
    public boolean isApplicationOnIcc(IccCardApplicationStatus.AppType type);

    /**
     * @return true if a ICC card is present
     */
    public boolean hasIccCard();

    /**
     * @return true if ICC card is PIN2 blocked
     */
    public boolean getIccPin2Blocked();

    /**
     * @return true if ICC card is PUK2 blocked
     */
    public boolean getIccPuk2Blocked();
}
