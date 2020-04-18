// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "tools/battor_agent/battor_agent.h"

#include <algorithm>
#include <iomanip>
#include <vector>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "tools/battor_agent/battor_connection_impl.h"
#include "tools/battor_agent/battor_sample_converter.h"

using base::StringPrintf;
using std::vector;

namespace battor {

namespace {

// The maximum number of times to retry a command.
const uint8_t kMaxCommandAttempts = 10;

// The maximum number of times to retry a sample frame.
const uint8_t kMaxFrameAttempts = 10;

// The amount of time we need to wait after recording a clock sync marker in
// order to ensure that the sample we synced to doesn't get thrown out.
const uint8_t kStopTracingClockSyncDelayMilliseconds = 100;

// The number of seconds to wait before retrying a command.
const uint16_t kCommandRetryDelaySeconds = 2;

// The number of milliseconds to wait before retrying a sample frame.
const uint16_t kFrameRetryDelayMilliseconds = 100;

// The number of seconds allowed for a control message before timing out.
const uint8_t kBattOrControlMessageTimeoutSeconds = 2;

// Returns true if the specified vector of bytes decodes to a message that is an
// ack for the specified control message type.
bool IsAckOfControlCommand(BattOrMessageType message_type,
                           BattOrControlMessageType control_message_type,
                           const vector<char>& msg) {
  if (message_type != BATTOR_MESSAGE_TYPE_CONTROL_ACK)
    return false;

  if (msg.size() != sizeof(BattOrControlMessageAck))
    return false;

  const BattOrControlMessageAck* ack =
      reinterpret_cast<const BattOrControlMessageAck*>(msg.data());

  if (ack->type != control_message_type)
    return false;

  return true;
}

// Attempts to decode the specified vector of bytes decodes to a valid EEPROM.
// Returns the new EEPROM, or nullptr if unsuccessful.
std::unique_ptr<BattOrEEPROM> ParseEEPROM(BattOrMessageType message_type,
                                          const vector<char>& msg) {
  if (message_type != BATTOR_MESSAGE_TYPE_CONTROL_ACK)
    return nullptr;

  if (msg.size() != sizeof(BattOrEEPROM))
    return nullptr;

  std::unique_ptr<BattOrEEPROM> eeprom(new BattOrEEPROM());
  memcpy(eeprom.get(), msg.data(), sizeof(BattOrEEPROM));
  return eeprom;
}
}  // namespace

BattOrResults::BattOrResults() = default;

BattOrResults::BattOrResults(std::string details,
                             std::vector<float> power_samples_W,
                             uint32_t sample_rate)
    : details_(std::move(details)),
      power_samples_W_(std::move(power_samples_W)),
      sample_rate_(sample_rate) {}

BattOrResults::BattOrResults(const BattOrResults&) = default;

BattOrResults::~BattOrResults() = default;

BattOrAgent::BattOrAgent(
    const std::string& path,
    Listener* listener,
    scoped_refptr<base::SingleThreadTaskRunner> ui_thread_task_runner)
    : connection_(new BattOrConnectionImpl(path, this, ui_thread_task_runner)),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      listener_(listener),
      last_action_(Action::INVALID),
      command_(Command::INVALID),
      next_sequence_number_(0),
      num_command_attempts_(0),
      num_frame_attempts_(0) {
  // We don't care what sequence the constructor is called on - we only care
  // that all of the other method invocations happen on the same sequence.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

BattOrAgent::~BattOrAgent() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void BattOrAgent::StartTracing() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  connection_->LogSerial("Starting command StartTracing.");

  // When tracing is restarted, all previous clock sync markers are invalid.
  clock_sync_markers_.clear();
  last_clock_sync_time_ = base::TimeTicks();

  command_ = Command::START_TRACING;

  if (connection_->IsOpen()) {
    PerformAction(GetFirstAction(Command::START_TRACING));
  } else {
    PerformAction(Action::REQUEST_CONNECTION);
  }
}

void BattOrAgent::StopTracing() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  connection_->LogSerial("Starting command StopTracing.");

  command_ = Command::STOP_TRACING;

  if (connection_->IsOpen()) {
    PerformAction(GetFirstAction(Command::STOP_TRACING));
  } else {
    PerformAction(Action::REQUEST_CONNECTION);
  }
}

void BattOrAgent::RecordClockSyncMarker(const std::string& marker) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  connection_->LogSerial("Starting command RecordClockSyncMarker.");

  command_ = Command::RECORD_CLOCK_SYNC_MARKER;
  pending_clock_sync_marker_ = marker;

  if (connection_->IsOpen()) {
    PerformAction(GetFirstAction(Command::RECORD_CLOCK_SYNC_MARKER));
  } else {
    PerformAction(Action::REQUEST_CONNECTION);
  }
}

void BattOrAgent::GetFirmwareGitHash() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  connection_->LogSerial("Starting command GetFirmwareGitHash.");

  command_ = Command::GET_FIRMWARE_GIT_HASH;

  if (connection_->IsOpen()) {
    PerformAction(GetFirstAction(Command::GET_FIRMWARE_GIT_HASH));
  } else {
    PerformAction(Action::REQUEST_CONNECTION);
  }
}

void BattOrAgent::BeginConnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  connection_->Open();
}

void BattOrAgent::OnConnectionOpened(bool success) {
  if (!success) {
    CompleteCommand(BATTOR_ERROR_CONNECTION_FAILED);
    return;
  }

  PerformAction(Action::POST_CONNECT_FLUSH);
}

void BattOrAgent::OnConnectionFlushed(bool success) {
  if (!success) {
    CompleteCommand(BATTOR_ERROR_CONNECTION_FAILED);
    return;
  }

  if (last_action_ == Action::POST_CONNECT_FLUSH) {
    PerformAction(GetFirstAction(command_));
  } else if (last_action_ == Action::POST_READ_ERROR_FLUSH) {
    base::TimeDelta request_samples_delay =
        base::TimeDelta::FromMilliseconds(kFrameRetryDelayMilliseconds);
    PerformDelayedAction(Action::SEND_SAMPLES_REQUEST, request_samples_delay);
  } else {
    NOTREACHED();
  }
}

void BattOrAgent::OnBytesSent(bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!success) {
    CompleteCommand(BATTOR_ERROR_SEND_ERROR);
    return;
  }

  switch (last_action_) {
    case Action::SEND_INIT:
      PerformAction(Action::READ_INIT_ACK);
      return;
    case Action::SEND_SET_GAIN:
      PerformAction(Action::READ_SET_GAIN_ACK);
      return;
    case Action::SEND_START_TRACING:
      PerformAction(Action::READ_START_TRACING_ACK);
      return;
    case Action::SEND_EEPROM_REQUEST:
      PerformAction(Action::READ_EEPROM);
      return;
    case Action::SEND_SAMPLES_REQUEST:
      if (next_sequence_number_ == 0)
        PerformAction(Action::READ_CALIBRATION_FRAME);
      else
        PerformAction(Action::READ_DATA_FRAME);
      return;
    case Action::SEND_CURRENT_SAMPLE_REQUEST:
      PerformAction(Action::READ_CURRENT_SAMPLE);
      return;
    case Action::SEND_GIT_HASH_REQUEST:
      PerformAction(Action::READ_GIT_HASH);
      return;
    default:
      NOTREACHED();
      return;
  }
}

void BattOrAgent::OnMessageRead(bool success,
                                BattOrMessageType type,
                                std::unique_ptr<vector<char>> bytes) {
  timeout_callback_.Cancel();

  if (!success) {
    switch (last_action_) {
      case Action::READ_GIT_HASH:
      case Action::READ_INIT_ACK:
      case Action::READ_SET_GAIN_ACK:
      case Action::READ_START_TRACING_ACK:
      case Action::READ_EEPROM:
        RetryCommand();
        return;

      case Action::READ_CALIBRATION_FRAME:
      case Action::READ_DATA_FRAME:
        RetryFrame();
        return;

      case Action::READ_CURRENT_SAMPLE:
        CompleteCommand(BATTOR_ERROR_RECEIVE_ERROR);
        return;

      default:
        NOTREACHED();
        return;
    }
  }

  switch (last_action_) {
    case Action::READ_INIT_ACK:
      if (!IsAckOfControlCommand(type, BATTOR_CONTROL_MESSAGE_TYPE_INIT,
                                 *bytes)) {
        RetryCommand();
        return;
      }

      switch (command_) {
        case Command::START_TRACING:
          PerformAction(Action::SEND_SET_GAIN);
          return;
        default:
          NOTREACHED();
          return;
      }

    case Action::READ_SET_GAIN_ACK:
      if (!IsAckOfControlCommand(type, BATTOR_CONTROL_MESSAGE_TYPE_SET_GAIN,
                                 *bytes)) {
        RetryCommand();
        return;
      }

      PerformAction(Action::SEND_START_TRACING);
      return;

    case Action::READ_START_TRACING_ACK:
      if (!IsAckOfControlCommand(
              type, BATTOR_CONTROL_MESSAGE_TYPE_START_SAMPLING_SD, *bytes)) {
        RetryCommand();
        return;
      }

      CompleteCommand(BATTOR_ERROR_NONE);
      return;

    case Action::READ_EEPROM: {
      battor_eeprom_ = ParseEEPROM(type, *bytes);
      if (!battor_eeprom_) {
        RetryCommand();
        return;
      }

      // Make sure that we don't request samples until a safe amount of time has
      // elapsed since recording the last clock sync marker: we need to ensure
      // that the sample we synced to doesn't get thrown out.
      base::TimeTicks min_request_samples_time =
          last_clock_sync_time_ + base::TimeDelta::FromMilliseconds(
                                      kStopTracingClockSyncDelayMilliseconds);
      base::TimeDelta request_samples_delay =
          std::max(min_request_samples_time - tick_clock_->NowTicks(),
                   base::TimeDelta());

      num_frame_attempts_ = 1;
      PerformDelayedAction(Action::SEND_SAMPLES_REQUEST, request_samples_delay);
      return;
    }
    case Action::READ_CALIBRATION_FRAME: {
      BattOrFrameHeader frame_header;
      if (!ParseSampleFrame(type, *bytes, next_sequence_number_, &frame_header,
                            &calibration_frame_)) {
        RetryFrame();
        return;
      }

      // Make sure that the calibration frame has actual samples in it.
      if (calibration_frame_.empty()) {
        CompleteCommand(BATTOR_ERROR_FILE_NOT_FOUND);
        return;
      }

      next_sequence_number_++;
      num_frame_attempts_ = 1;
      PerformAction(Action::SEND_SAMPLES_REQUEST);
      return;
    }

    case Action::READ_DATA_FRAME: {
      BattOrFrameHeader frame_header;
      vector<RawBattOrSample> frame;
      if (!ParseSampleFrame(type, *bytes, next_sequence_number_, &frame_header,
                            &frame)) {
        RetryFrame();
        return;
      }

      // Check for the empty frame the BattOr uses to indicate it's done
      // streaming samples.
      if (frame.empty()) {
        CompleteCommand(BATTOR_ERROR_NONE);
        return;
      }

      samples_.insert(samples_.end(), frame.begin(), frame.end());

      next_sequence_number_++;
      num_frame_attempts_ = 1;
      PerformAction(Action::SEND_SAMPLES_REQUEST);
      return;
    }

    case Action::READ_CURRENT_SAMPLE:
      if (type != BATTOR_MESSAGE_TYPE_CONTROL_ACK ||
          bytes->size() != sizeof(uint32_t)) {
        CompleteCommand(BATTOR_ERROR_UNEXPECTED_MESSAGE);
        return;
      }

      uint32_t sample_num;
      memcpy(&sample_num, bytes->data(), sizeof(uint32_t));
      clock_sync_markers_[sample_num] = pending_clock_sync_marker_;
      last_clock_sync_time_ = tick_clock_->NowTicks();
      CompleteCommand(BATTOR_ERROR_NONE);
      return;

    case Action::READ_GIT_HASH:
      if (type != BATTOR_MESSAGE_TYPE_CONTROL_ACK) {
        RetryCommand();
        return;
      }

      firmware_git_hash_ = std::string(bytes->begin(), bytes->end());
      CompleteCommand(BATTOR_ERROR_NONE);
      return;

    default:
      NOTREACHED();
      return;
  }
}

void BattOrAgent::PerformAction(Action action) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  last_action_ = action;

  switch (action) {
    case Action::REQUEST_CONNECTION:
      BeginConnect();
      return;
    case Action::POST_CONNECT_FLUSH:
    case Action::POST_READ_ERROR_FLUSH:
      connection_->Flush();
      return;
    // The following actions are required for StartTracing:
    case Action::SEND_INIT:
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_INIT, 0, 0);
      return;
    case Action::READ_INIT_ACK:
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL_ACK);
      return;
    case Action::SEND_SET_GAIN:
      // Set the BattOr's gain. Setting the gain tells the BattOr the range of
      // power measurements that we expect to see.
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_SET_GAIN, BATTOR_GAIN_LOW,
                         0);
      return;
    case Action::READ_SET_GAIN_ACK:
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL_ACK);
      return;
    case Action::SEND_START_TRACING:
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_START_SAMPLING_SD, 0, 0);
      return;
    case Action::READ_START_TRACING_ACK:
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL_ACK);
      return;
    // The following actions are required for StopTracing:
    case Action::SEND_EEPROM_REQUEST:
      // Read the BattOr's EEPROM to get calibration information that's required
      // to convert the raw samples to accurate ones.
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_READ_EEPROM,
                         sizeof(BattOrEEPROM), 0);
      return;
    case Action::READ_EEPROM:
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL_ACK);
      return;
    case Action::SEND_SAMPLES_REQUEST:
      // Send a request to the BattOr to tell it to start streaming the samples
      // that it's stored on its SD card over the serial connection.
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_READ_SD_UART, 0,
                         next_sequence_number_);
      return;
    case Action::READ_CALIBRATION_FRAME:
      // Data frames are numbered starting at zero and counting up by one each
      // data frame. We keep track of the next frame sequence number we expect
      // to see to ensure we don't miss any data.
      next_sequence_number_ = 0;

      // Clear stored samples from prior attempts to read sample frames.
      samples_.clear();
      calibration_frame_.clear();
      FALLTHROUGH;
    case Action::READ_DATA_FRAME:
      // The first frame sent back from the BattOr contains voltage and current
      // data that excludes whatever device is being measured from the
      // circuit. We use this first frame to establish a baseline voltage and
      // current.
      //
      // All further frames contain real (but uncalibrated) voltage and current
      // data.
      SetActionTimeout(kBattOrControlMessageTimeoutSeconds);
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_SAMPLES);
      return;

    // The following actions are required for RecordClockSyncMarker:
    case Action::SEND_CURRENT_SAMPLE_REQUEST:
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_READ_SAMPLE_COUNT, 0, 0);
      return;
    case Action::READ_CURRENT_SAMPLE:
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL_ACK);
      return;

    case Action::SEND_GIT_HASH_REQUEST:
      SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_GET_FIRMWARE_GIT_HASH, 0,
                         0);
      return;

    case Action::READ_GIT_HASH:
      connection_->ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL_ACK);
      return;

    case Action::INVALID:
      NOTREACHED();
      return;
  }
}

void BattOrAgent::PerformDelayedAction(Action action, base::TimeDelta delay) {
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&BattOrAgent::PerformAction, AsWeakPtr(), action),
      delay);
}

void BattOrAgent::OnActionTimeout() {
  switch (last_action_) {
    case Action::READ_INIT_ACK:
    case Action::READ_SET_GAIN_ACK:
    case Action::READ_START_TRACING_ACK:
    case Action::READ_EEPROM:
    case Action::READ_CALIBRATION_FRAME:
    case Action::READ_DATA_FRAME:
    case Action::READ_GIT_HASH:
      connection_->CancelReadMessage();
      return;

    default:
      CompleteCommand(BATTOR_ERROR_TIMEOUT);
      timeout_callback_.Cancel();
  }
}

void BattOrAgent::SendControlMessage(BattOrControlMessageType type,
                                     uint16_t param1,
                                     uint16_t param2) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  SetActionTimeout(kBattOrControlMessageTimeoutSeconds);

  BattOrControlMessage msg{type, param1, param2};
  connection_->SendBytes(BATTOR_MESSAGE_TYPE_CONTROL, &msg, sizeof(msg));
}

// Returns true if the specified vector of bytes decodes to a valid BattOr
// samples frame. The frame header and samples are returned via the frame_header
// and samples paramaters.
bool BattOrAgent::ParseSampleFrame(BattOrMessageType type,
                                   const vector<char>& msg,
                                   uint32_t expected_sequence_number,
                                   BattOrFrameHeader* frame_header,
                                   vector<RawBattOrSample>* samples) {
  if (type != BATTOR_MESSAGE_TYPE_SAMPLES) {
    connection_->LogSerial(
        StringPrintf("ParseSampleFrame failed due to unexpected message type "
                     "number (wanted BATTOR_MESSAGE_TYPE_SAMPLES, but got %d).",
                     type));
    return false;
  }

  // Each frame should contain a header and an integer number of BattOr samples.
  if ((msg.size() - sizeof(BattOrFrameHeader)) % sizeof(RawBattOrSample) != 0) {
    connection_->LogSerial(
        "ParseSampleFrame failed due to containing a noninteger number of "
        "BattOr samples.");
    return false;
  }

  // The first bytes in the frame contain the frame header.
  const char* frame_ptr = reinterpret_cast<const char*>(msg.data());
  memcpy(frame_header, frame_ptr, sizeof(BattOrFrameHeader));
  frame_ptr += sizeof(BattOrFrameHeader);

  if (frame_header->sequence_number != expected_sequence_number) {
    connection_->LogSerial(
        StringPrintf("ParseSampleFrame failed due to unexpected sequence "
                     "number (wanted %d, but got %d).",
                     expected_sequence_number, frame_header->sequence_number));
    return false;
  }

  size_t remaining_bytes = msg.size() - sizeof(BattOrFrameHeader);
  if (remaining_bytes != frame_header->length) {
    connection_->LogSerial(StringPrintf(
        "ParseSampleFrame failed due to to a mismatch between the length of "
        "the frame as stated in the frame header and the actual length of the "
        "frame (frame header %d, actual length %zu).",
        frame_header->length, remaining_bytes));
    return false;
  }

  samples->resize(remaining_bytes / sizeof(RawBattOrSample));
  memcpy(samples->data(), frame_ptr, remaining_bytes);

  return true;
}

void BattOrAgent::RetryCommand() {
  if (++num_command_attempts_ >= kMaxCommandAttempts) {
    connection_->LogSerial(StringPrintf(
        "Exhausted command retry attempts (would have been attempt %d of %d).",
        num_command_attempts_ + 1, kMaxCommandAttempts));
    CompleteCommand(BATTOR_ERROR_TOO_MANY_COMMAND_RETRIES);
    return;
  }

  connection_->LogSerial(StringPrintf("Retrying command (attempt %d of %d).",
                                      num_command_attempts_ + 1,
                                      kMaxCommandAttempts));

  // Restart the serial connection to guarantee that the connection gets flushed
  // before retrying the command.
  connection_->Close();

  // Failed to read response to message, retry current command.
  base::Callback<void()> next_command;
  switch (command_) {
    case Command::START_TRACING:
      next_command = base::Bind(&BattOrAgent::StartTracing, AsWeakPtr());
      break;
    case Command::STOP_TRACING:
      next_command = base::Bind(&BattOrAgent::StopTracing, AsWeakPtr());
      break;
    case Command::GET_FIRMWARE_GIT_HASH:
      next_command = base::Bind(&BattOrAgent::GetFirmwareGitHash, AsWeakPtr());
      break;
    default:
      NOTREACHED();
  }

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, next_command,
      base::TimeDelta::FromSeconds(kCommandRetryDelaySeconds));
}

void BattOrAgent::RetryFrame() {
  if (++num_frame_attempts_ > kMaxFrameAttempts) {
    connection_->LogSerial(StringPrintf(
        "Exhausted frame retry attempts (would have been attempt %d of %d).",
        num_frame_attempts_, kMaxFrameAttempts));
    CompleteCommand(BATTOR_ERROR_TOO_MANY_FRAME_RETRIES);
    return;
  }

  connection_->LogSerial(StringPrintf("Retrying frame (attempt %d of %d).",
                                      num_frame_attempts_,
                                      kMaxFrameAttempts));

  PerformAction(Action::POST_READ_ERROR_FLUSH);
}

void BattOrAgent::CompleteCommand(BattOrError error) {
  connection_->LogSerial(
      StringPrintf("Completing command with error code: %d.", error));

  switch (command_) {
    case Command::START_TRACING:
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&Listener::OnStartTracingComplete,
                                base::Unretained(listener_), error));
      break;
    case Command::STOP_TRACING:
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::Bind(&Listener::OnStopTracingComplete,
                     base::Unretained(listener_), SamplesToResults(), error));
      break;
    case Command::RECORD_CLOCK_SYNC_MARKER:
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&Listener::OnRecordClockSyncMarkerComplete,
                                base::Unretained(listener_), error));
      break;
    case Command::GET_FIRMWARE_GIT_HASH:
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::Bind(&Listener::OnGetFirmwareGitHashComplete,
                     base::Unretained(listener_), firmware_git_hash_, error));
      break;
    case Command::INVALID:
      NOTREACHED();
      return;
  }

  last_action_ = Action::INVALID;
  command_ = Command::INVALID;
  pending_clock_sync_marker_.clear();
  battor_eeprom_.reset();
  calibration_frame_.clear();
  samples_.clear();
  next_sequence_number_ = 0;
  num_command_attempts_ = 0;
}

BattOrResults BattOrAgent::SamplesToResults() {
  if (calibration_frame_.empty() || samples_.empty() || !battor_eeprom_)
    return BattOrResults();

  BattOrSampleConverter converter(*battor_eeprom_, calibration_frame_);

  std::stringstream trace_stream;
  trace_stream << std::fixed;

  // Create a header that indicates the BattOr's parameters for these samples.
  BattOrSample min_sample = converter.MinSample();
  BattOrSample max_sample = converter.MaxSample();
  trace_stream << "# BattOr" << std::endl
               << std::setprecision(1) << "# voltage_range ["
               << min_sample.voltage_mV << ", " << max_sample.voltage_mV
               << "] mV" << std::endl
               << "# current_range [" << min_sample.current_mA << ", "
               << max_sample.current_mA << "] mA" << std::endl
               << "# sample_rate " << battor_eeprom_->sd_sample_rate << " Hz"
               << ", gain " << battor_eeprom_->low_gain << "x" << std::endl;

  // Create a string representation of the BattOr samples.
  for (size_t i = 0; i < samples_.size(); i++) {
    BattOrSample sample = converter.ToSample(samples_[i], i);
    trace_stream << std::setprecision(2) << sample.time_ms << " "
                 << std::setprecision(1) << sample.current_mA << " "
                 << sample.voltage_mV;

    // If there's a clock sync marker for the current sample, print it.
    auto clock_sync_marker = clock_sync_markers_.find(
        static_cast<uint32_t>(calibration_frame_.size() + i));
    if (clock_sync_marker != clock_sync_markers_.end())
      trace_stream << " <" << clock_sync_marker->second << ">";

    trace_stream << std::endl;
  }

  for (auto it = clock_sync_markers_.begin(); it != clock_sync_markers_.end();
       ++it) {
    size_t total_sample_count = calibration_frame_.size() + samples_.size();
    if (it->first >= total_sample_count) {
      connection_->LogSerial(StringPrintf(
          "Clock sync occurred at a sample not included in the result (clock "
          "sync sample index: %d, total sample count: %zu).",
          it->first, total_sample_count));
    }
  }

  // Convert to a vector of power in watts.
  std::vector<float> samples(samples_.size());
  for (size_t i = 0; i < samples_.size(); i++)
    samples[i] = converter.ToWatts(samples_[i]);

  return BattOrResults(trace_stream.str(), samples,
                       battor_eeprom_->sd_sample_rate);
}

void BattOrAgent::SetActionTimeout(uint16_t timeout_seconds) {
  timeout_callback_.Reset(
      base::Bind(&BattOrAgent::OnActionTimeout, AsWeakPtr()));
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, timeout_callback_.callback(),
      base::TimeDelta::FromSeconds(timeout_seconds));
}

BattOrAgent::Action BattOrAgent::GetFirstAction(BattOrAgent::Command command) {
  switch (command_) {
    case Command::START_TRACING:
      return Action::SEND_INIT;
    case Command::STOP_TRACING:
      return Action::SEND_EEPROM_REQUEST;
    case Command::RECORD_CLOCK_SYNC_MARKER:
      return Action::SEND_CURRENT_SAMPLE_REQUEST;
    case Command::GET_FIRMWARE_GIT_HASH:
      return Action::SEND_GIT_HASH_REQUEST;
    case Command::INVALID:
      NOTREACHED();
  }

  return Action::INVALID;
}

}  // namespace battor
