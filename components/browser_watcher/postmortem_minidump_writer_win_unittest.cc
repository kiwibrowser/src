// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/postmortem_minidump_writer.h"

#include <windows.h>  // NOLINT
#include <dbghelp.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/win/scoped_handle.h"
#include "components/browser_watcher/stability_data_names.h"
#include "components/browser_watcher/stability_report.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/crashpad/crashpad/snapshot/minidump/process_snapshot_minidump.h"
#include "third_party/crashpad/crashpad/util/file/file_reader.h"
#include "third_party/crashpad/crashpad/util/misc/uuid.h"

namespace browser_watcher {

using crashpad::UUID;

const char kProductName[] = "some-product";
const char kExpectedProductName[] = "some-product_Postmortem";
const char kVersion[] = "51.0.2704.106";
const char kChannel[] = "some-channel";
const char kPlatform[] = "some-platform";

class WritePostmortemDumpTest : public testing::Test {
 public:
  void SetUp() override {
    testing::Test::SetUp();

    expected_client_id_.InitializeWithNew();
    expected_report_id_.InitializeWithNew();

    // Create a stability report.
    // TODO(manzagop): flesh out the report once proto is more detailed.
    ProcessState* process_state = expected_report_.add_process_states();
    CodeModule* module = process_state->add_modules();
    module->set_base_address(1024);
    module->set_code_file("some_code_file.dll");

    // Set up directory and path.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    minidump_path_ = temp_dir_.GetPath().AppendASCII("minidump.dmp");
  }

  bool WriteDump(bool add_product_details) {
    // Make a copy of the expected report as product details are stripped from
    // the proto written to the minidump stream.
    StabilityReport report(expected_report_);

    if (add_product_details) {
      google::protobuf::Map<std::string, TypedValue>& global_data =
          *report.mutable_global_data();
      global_data[kStabilityChannel].set_string_value(kChannel);
      global_data[kStabilityPlatform].set_string_value(kPlatform);
      global_data[kStabilityProduct].set_string_value(kProductName);
      global_data[kStabilityVersion].set_string_value(kVersion);
    }

    crashpad::FileWriter writer;
    if (!writer.Open(minidump_path_, crashpad::FileWriteMode::kCreateOrFail,
                     crashpad::FilePermissions::kWorldReadable)) {
      return false;
    }

    return WritePostmortemDump(&writer, expected_client_id_,
                               expected_report_id_, &report);
  }

  const base::FilePath& minidump_path() { return minidump_path_; }
  const UUID& expected_client_id() { return expected_client_id_; }
  const UUID& expected_report_id() { return expected_report_id_; }
  const StabilityReport& expected_report() { return expected_report_; }

 private:
  base::ScopedTempDir temp_dir_;
  base::FilePath minidump_path_;
  UUID expected_client_id_;
  UUID expected_report_id_;
  StabilityReport expected_report_;
};

TEST_F(WritePostmortemDumpTest, MissingProductDetailsFailureTest) {
  ASSERT_FALSE(WriteDump(false));
}

TEST_F(WritePostmortemDumpTest, ValidateStabilityReportTest) {
  ASSERT_TRUE(WriteDump(true));

  // Read back the minidump to extract the proto.
  // TODO(manzagop): rely on crashpad for reading the proto once crashpad
  //     supports it (https://crashpad.chromium.org/bug/10).
  base::ScopedFILE minidump_file;
  minidump_file.reset(base::OpenFile(minidump_path(), "rb"));
  ASSERT_TRUE(minidump_file.get());

  MINIDUMP_HEADER header = {};
  ASSERT_EQ(1U, fread(&header, sizeof(header), 1, minidump_file.get()));
  ASSERT_EQ(static_cast<ULONG32>(MINIDUMP_SIGNATURE), header.Signature);
  ASSERT_EQ(2U, header.NumberOfStreams);
  RVA directory_rva = header.StreamDirectoryRva;

  MINIDUMP_DIRECTORY directory = {};
  ASSERT_EQ(0, fseek(minidump_file.get(), directory_rva, SEEK_SET));
  ASSERT_EQ(1U, fread(&directory, sizeof(directory), 1, minidump_file.get()));
  ASSERT_EQ(0x4B6B0002U, directory.StreamType);
  RVA report_rva = directory.Location.Rva;
  ULONG32 report_size_bytes = directory.Location.DataSize;

  std::string recovered_serialized_report;
  recovered_serialized_report.resize(report_size_bytes);
  ASSERT_EQ(0, fseek(minidump_file.get(), report_rva, SEEK_SET));
  ASSERT_EQ(report_size_bytes, fread(&recovered_serialized_report.at(0), 1,
                                     report_size_bytes, minidump_file.get()));

  // Validate the recovered report.
  std::string expected_serialized_report;
  expected_report().SerializeToString(&expected_serialized_report);
  ASSERT_EQ(expected_serialized_report, recovered_serialized_report);

  StabilityReport recovered_report;
  ASSERT_TRUE(recovered_report.ParseFromString(recovered_serialized_report));
}

TEST_F(WritePostmortemDumpTest, CrashpadCanReadTest) {
  ASSERT_TRUE(WriteDump(true));

  // Validate crashpad can read the produced minidump.
  crashpad::FileReader minidump_file_reader;
  ASSERT_TRUE(minidump_file_reader.Open(minidump_path()));

  crashpad::ProcessSnapshotMinidump minidump_process_snapshot;
  ASSERT_TRUE(minidump_process_snapshot.Initialize(&minidump_file_reader));

  // Validate the crashpadinfo.
  UUID client_id;
  minidump_process_snapshot.ClientID(&client_id);
  ASSERT_EQ(expected_client_id(), client_id);

  UUID report_id;
  minidump_process_snapshot.ReportID(&report_id);
  ASSERT_EQ(expected_report_id(), report_id);

  std::map<std::string, std::string> parameters =
      minidump_process_snapshot.AnnotationsSimpleMap();
  auto it = parameters.find("prod");
  ASSERT_NE(parameters.end(), it);
  ASSERT_EQ(kExpectedProductName, it->second);

  it = parameters.find("ver");
  ASSERT_NE(parameters.end(), it);
  ASSERT_EQ(kVersion, it->second);

  it = parameters.find("channel");
  ASSERT_NE(parameters.end(), it);
  ASSERT_EQ(kChannel, it->second);

  it = parameters.find("plat");
  ASSERT_NE(parameters.end(), it);
  ASSERT_EQ(kPlatform, it->second);
}

}  // namespace browser_watcher
