/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef LIBWDS_PUBLIC_PEER_H_
#define LIBWDS_PUBLIC_PEER_H_

#include <string>

#include "wds_export.h"

namespace wds {

enum ErrorType {
  /// Arrived message cannot be handled by the state machine at the time.
  UnexpectedMessageError,
  /// RTSP message cannot be created form the given input.
  MessageParseError,
  /// The connected peer became unresponsive.
  TimeoutError
};


/**
 * Peer interface.
 *
 * Peer is a base class for sink and source state machines.
 */
class WDS_EXPORT Peer {
 public:
  /**
   * Delegate interface.
   *
   * Implementation of Delegate interface is used by the state machine
   * to obtain necessary context from the client.
   */
  class Delegate {
   public:
    /**
     * The implementation should send the RTSP data over the network
     * @param data data to be send
     */
    virtual void SendRTSPData(const std::string& data) = 0;
    /**
     * Returns the local IP address
     * @return IP address
     */
    virtual std::string GetLocalIPAddress() const = 0;
    /**
     * The implementation should start a timer to be used by the state machine.
     * @param seconds the time interval in seconds
     * @return unique timer id within the session
     */
    virtual unsigned CreateTimer(int seconds) = 0;
    /**
     * The implementation should release timer by the given id.
     * @param timer_id id of the timer to be released.
     */
    virtual void ReleaseTimer(unsigned timer_id) = 0;

    /**
     * Returns the sequence number for the following RTSP request-response pair
     * @param initial_peer_cseq is provided for the WFD sink implementation at
     * the first method's call during the WFD session and it contains the
     * initial request sequence number obtained from the WFD source (the initial
     * sequence numbers of connected peers may not be identical).
     */
    virtual int GetNextCSeq(int* initial_peer_cseq = nullptr) const = 0;

   protected:
    virtual ~Delegate() {}
  };

  /**
   * Observer interface.
   *
   * This interface can be used by the client in order to get notifications
   * from the state machine.
   */
  class Observer {
   public:
    /**
     * This method is called by the state machine if an error has occurred.
     * State machine is not reset.
     *
     * @param error type of the error
     *
     * @see ErrorType
     */
    virtual void ErrorOccurred(ErrorType error) {}
    /**
     * This method is called by the state machine if the session has been
     * completed normally (with 'teardown' message).
     * State machine is not reset.
     */
    virtual void SessionCompleted() {}

   protected:
    virtual ~Observer() {}
  };

  virtual ~Peer() {}

  /**
   * Starts wds state machine (source or sink)
   */
  virtual void Start() = 0;

  /**
   * Reset wds state machine (source or sink) to the initial state.
   */
  virtual void Reset() = 0;

  /**
   * Whenever RTSP data is received, this method should be called, so that
   * the state machine could take action based on current state.
   */
  virtual void RTSPDataReceived(const std::string& data) = 0;

  // Following methods:
  // @see Teardown()
  // @see Play()
  // @see Pause()
  // send M5 wfd_trigger_method messages for Peers that implement
  // Source functionality or M7, M8 and M9 for Sink implementations

  /**
   * Sends RTSP teardown request.
   * @return true if request can be sent, false otherwise.
   */
  virtual bool Teardown() = 0;

  /**
   * Sends RTSP play request.
   * @return true if request can be sent, false otherwise.
   */
  virtual bool Play() = 0;

  /**
   * Sends RTSP pause request.
   * @return true if request can be sent, false otherwise.
   */
  virtual bool Pause() = 0;

  /**
   * This method should be called by the client to notify the state
   * machine about the timer events.
   * @param timer_id id of the timer
   * @return true if request can be sent, false otherwise.
   *
   * @see Delegate::CreateTimer()
   */
  virtual void OnTimerEvent(unsigned timer_id) = 0;
};

}

#endif // LIBWDS_PUBLIC_PEER_H_
