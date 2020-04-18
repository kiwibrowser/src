// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var SourceEntry = (function() {
  'use strict';

  /**
   * A SourceEntry gathers all log entries with the same source.
   *
   * @constructor
   */
  function SourceEntry(logEntry, maxPreviousSourceId) {
    this.maxPreviousSourceId_ = maxPreviousSourceId;
    this.entries_ = [];
    this.description_ = '';

    // Set to true on most net errors.
    this.isError_ = false;

    // If the first entry is a BEGIN_PHASE, set to false.
    // Set to true when an END_PHASE matching the first entry is encountered.
    this.isInactive_ = true;

    if (logEntry.phase == EventPhase.PHASE_BEGIN)
      this.isInactive_ = false;

    this.update(logEntry);
  }

  SourceEntry.prototype = {
    update: function(logEntry) {
      // Only the last event should have the same type first event,
      if (!this.isInactive_ && logEntry.phase == EventPhase.PHASE_END &&
          logEntry.type == this.entries_[0].type) {
        this.isInactive_ = true;
      }

      // If we have a net error code, update |this.isError_| if appropriate.
      if (logEntry.params) {
        var netErrorCode = logEntry.params.net_error;
        // Skip both cases where netErrorCode is undefined, and cases where it
        // is 0, indicating no actual error occurred.
        if (netErrorCode) {
          // Ignore error code caused by not finding an entry in the cache.
          if (logEntry.type != EventType.HTTP_CACHE_OPEN_ENTRY ||
              netErrorCode != NetError.ERR_FAILED) {
            this.isError_ = true;
          }
        }
      }

      var prevStartEntry = this.getStartEntry_();
      this.entries_.push(logEntry);
      var curStartEntry = this.getStartEntry_();

      // If we just got the first entry for this source.
      if (prevStartEntry != curStartEntry)
        this.updateDescription_();
    },

    updateDescription_: function() {
      var e = this.getStartEntry_();
      this.description_ = '';
      if (!e)
        return;

      if (e.source.type == EventSourceType.NONE) {
        // NONE is what we use for global events that aren't actually grouped
        // by a "source ID", so we will just stringize the event's type.
        this.description_ = EventTypeNames[e.type];
        return;
      }

      if (e.params == undefined) {
        return;
      }

      switch (e.source.type) {
        case EventSourceType.URL_REQUEST:
        // TODO(ricea): Remove SOCKET_STREAM after M41 is released.
        case EventSourceType.SOCKET_STREAM:
        case EventSourceType.HTTP_STREAM_JOB:
        case EventSourceType.HTTP_STREAM_JOB_CONTROLLER:
        case EventSourceType.BIDIRECTIONAL_STREAM:
          this.description_ = e.params.url;
          break;
        // TODO(davidben): Remove CONNECT_JOB after M57 is released.
        case EventSourceType.CONNECT_JOB:
        case EventSourceType.TRANSPORT_CONNECT_JOB:
        case EventSourceType.SSL_CONNECT_JOB:
        case EventSourceType.SOCKS_CONNECT_JOB:
        case EventSourceType.HTTP_PROXY_CONNECT_JOB:
        case EventSourceType.WEB_SOCKET_TRANSPORT_CONNECT_JOB:
          this.description_ = e.params.group_name;
          break;
        case EventSourceType.HOST_RESOLVER_IMPL_JOB:
        case EventSourceType.HOST_RESOLVER_IMPL_PROC_TASK:
          this.description_ = e.params.host;
          break;
        case EventSourceType.DISK_CACHE_ENTRY:
        case EventSourceType.MEMORY_CACHE_ENTRY:
          this.description_ = e.params.key;
          break;
        case EventSourceType.QUIC_SESSION:
          if (e.params.host != undefined)
            this.description_ = e.params.host;
          break;
        case EventSourceType.HTTP2_SESSION:
          if (e.params.host)
            this.description_ = e.params.host + ' (' + e.params.proxy + ')';
          break;
        case EventSourceType.HTTP_PIPELINED_CONNECTION:
          if (e.params.host_and_port)
            this.description_ = e.params.host_and_port;
          break;
        case EventSourceType.SOCKET:
        case EventSourceType.PROXY_CLIENT_SOCKET:
          // Use description of parent source, if any.
          if (e.params.source_dependency != undefined) {
            var parentId = e.params.source_dependency.id;
            this.description_ =
                SourceTracker.getInstance().getDescription(parentId);
          }
          break;
        case EventSourceType.UDP_SOCKET:
          if (e.params.address != undefined) {
            this.description_ = e.params.address;
            // If the parent of |this| is a HOST_RESOLVER_IMPL_JOB, use
            // '<DNS Server IP> [<host we're resolving>]'.
            if (this.entries_[0].type == EventType.SOCKET_ALIVE &&
                this.entries_[0].params &&
                this.entries_[0].params.source_dependency != undefined) {
              var parentId = this.entries_[0].params.source_dependency.id;
              var parent = SourceTracker.getInstance().getSourceEntry(parentId);
              if (parent &&
                  parent.getSourceType() ==
                      EventSourceType.HOST_RESOLVER_IMPL_JOB &&
                  parent.getDescription().length > 0) {
                this.description_ += ' [' + parent.getDescription() + ']';
              }
            }
          }
          break;
        case EventSourceType.ASYNC_HOST_RESOLVER_REQUEST:
        case EventSourceType.DNS_TRANSACTION:
          this.description_ = e.params.hostname;
          break;
        case EventSourceType.DOWNLOAD:
          switch (e.type) {
            case EventType.DOWNLOAD_FILE_RENAMED:
              this.description_ = e.params.new_filename;
              break;
            case EventType.DOWNLOAD_FILE_OPENED:
              this.description_ = e.params.file_name;
              break;
            case EventType.DOWNLOAD_ITEM_ACTIVE:
              this.description_ = e.params.file_name;
              break;
          }
          break;
      }

      if (this.description_ == undefined)
        this.description_ = '';
    },

    /**
     * Returns a description for this source log stream, which will be displayed
     * in the list view. Most often this is a URL that identifies the request,
     * or a hostname for a connect job, etc...
     */
    getDescription: function() {
      return this.description_;
    },

    /**
     * Returns the starting entry for this source. Conceptually this is the
     * first entry that was logged to this source. However, we skip over the
     * TYPE_REQUEST_ALIVE entries without parameters which wrap
     * TYPE_URL_REQUEST_START_JOB entries.  (TYPE_REQUEST_ALIVE may or may not
     * have parameters depending on what version of Chromium they were
     * generated from.)
     */
    getStartEntry_: function() {
      if (this.entries_.length < 1)
        return undefined;
      if (this.entries_[0].source.type == EventSourceType.FILESTREAM) {
        var e = this.findLogEntryByType_(EventType.FILE_STREAM_OPEN);
        if (e != undefined)
          return e;
      }
      if (this.entries_[0].source.type == EventSourceType.DOWNLOAD) {
        // If any rename occurred, use the last name
        e = this.findLastLogEntryStartByType_(EventType.DOWNLOAD_FILE_RENAMED);
        if (e != undefined)
          return e;
        // Otherwise, if the file was opened, use that name
        e = this.findLogEntryByType_(EventType.DOWNLOAD_FILE_OPENED);
        if (e != undefined)
          return e;
        // History items are never opened, so use the activation info
        e = this.findLogEntryByType_(EventType.DOWNLOAD_ITEM_ACTIVE);
        if (e != undefined)
          return e;
      }
      if (this.entries_.length >= 2) {
        // Needed for compatability with log dumps prior to M26.
        // TODO(mmenke):  Remove this.
        if (this.entries_[0].type == EventType.SOCKET_POOL_CONNECT_JOB &&
            this.entries_[0].params == undefined) {
          return this.entries_[1];
        }
        if (this.entries_[1].type == EventType.UDP_CONNECT)
          return this.entries_[1];
        if (this.entries_[0].type == EventType.REQUEST_ALIVE &&
            this.entries_[0].params == undefined) {
          var startIndex = 1;
          // Skip over delegate events for URL_REQUESTs.
          for (; startIndex + 1 < this.entries_.length; ++startIndex) {
            var type = this.entries_[startIndex].type;
            if (type != EventType.URL_REQUEST_DELEGATE &&
                type != EventType.DELEGATE_INFO) {
              break;
            }
          }
          return this.entries_[startIndex];
        }
        if (this.entries_[1].type == EventType.IPV6_PROBE_RUNNING)
          return this.entries_[1];
      }
      return this.entries_[0];
    },

    /**
     * Returns the first entry with the specified type, or undefined if not
     * found.
     */
    findLogEntryByType_: function(type) {
      for (var i = 0; i < this.entries_.length; ++i) {
        if (this.entries_[i].type == type) {
          return this.entries_[i];
        }
      }
      return undefined;
    },

    /**
     * Returns the beginning of the last entry with the specified type, or
     * undefined if not found.
     */
    findLastLogEntryStartByType_: function(type) {
      for (var i = this.entries_.length - 1; i >= 0; --i) {
        if (this.entries_[i].type == type) {
          if (this.entries_[i].phase != EventPhase.PHASE_END)
            return this.entries_[i];
        }
      }
      return undefined;
    },

    getLogEntries: function() {
      return this.entries_;
    },

    getSourceTypeString: function() {
      return EventSourceTypeNames[this.entries_[0].source.type];
    },

    getSourceType: function() {
      return this.entries_[0].source.type;
    },

    getSourceId: function() {
      return this.entries_[0].source.id;
    },

    /**
     * Returns the largest source ID seen before this object was received.
     * Used only for sorting SourceEntries without a source by source ID.
     */
    getMaxPreviousEntrySourceId: function() {
      return this.maxPreviousSourceId_;
    },

    isInactive: function() {
      return this.isInactive_;
    },

    isError: function() {
      return this.isError_;
    },

    /**
     * Returns time ticks of first event.
     */
    getStartTicks: function() {
      return this.entries_[0].time;
    },

    /**
     * Returns time of last event if inactive.  Returns current time otherwise.
     * Returned time is a "time ticks" value.
     */
    getEndTicks: function() {
      if (!this.isInactive_)
        return timeutil.getCurrentTimeTicks();
      return this.entries_[this.entries_.length - 1].time;
    },

    /**
     * Returns the time between the first and last events with a matching
     * source ID.  If source is still active, uses the current time for the
     * last event.
     */
    getDuration: function() {
      var startTime = this.getStartTicks();
      var endTime = this.getEndTicks();
      return endTime - startTime;
    },

    /**
     * Prints descriptive text about |entries_| to a new node added to the end
     * of |parent|.
     */
    printAsText: function(parent) {
      var tablePrinter = this.createTablePrinter();

      // Format the table for fixed-width text.
      tablePrinter.toText(0, parent);
    },

    /**
     * Creates a table printer for the SourceEntry.
     */
    createTablePrinter: function() {
      return createLogEntryTablePrinter(
          this.entries_,
          SourceTracker.getInstance().getUseRelativeTimes() ?
              timeutil.getBaseTime() :
              0,
          Constants.clientInfo.numericDate);
    },
  };

  return SourceEntry;
})();
