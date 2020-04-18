// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/file_manager_browsertest_base.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/containers/circular_deque.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/chromeos/file_manager/mount_test_util.h"
#include "chrome/browser/chromeos/file_manager/path_util.h"
#include "chrome/browser/chromeos/file_manager/volume_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/chromeos_switches.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "components/drive/service/fake_drive_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/test/test_api.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "media/base/media_switches.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "ui/message_center/public/cpp/notification.h"

namespace file_manager {
namespace {

// During test, the test extensions can send a list of entries (directories
// or files) to add to a target volume using an AddEntriesMessage command.

enum TargetVolume { LOCAL_VOLUME, DRIVE_VOLUME, USB_VOLUME };

struct AddEntriesMessage {
  // The volume to add |entries| to.
  TargetVolume volume;

  // The |entries| to be added.
  std::vector<std::unique_ptr<struct TestEntryInfo>> entries;

  // Converts |value| to an AddEntriesMessage: true on success.
  static bool ConvertJSONValue(const base::DictionaryValue& value,
                               AddEntriesMessage* message) {
    base::JSONValueConverter<AddEntriesMessage> converter;
    return converter.Convert(value, message);
  }

  // Registers AddEntriesMessage member info to the |converter|.
  static void RegisterJSONConverter(
      base::JSONValueConverter<AddEntriesMessage>* converter) {
    converter->RegisterCustomField("volume", &AddEntriesMessage::volume,
                                   &MapStringToTargetVolume);
    converter->RegisterRepeatedMessage<struct TestEntryInfo>(
        "entries", &AddEntriesMessage::entries);
  }

  // Maps |value| to TargetVolume. Returns true on success.
  static bool MapStringToTargetVolume(base::StringPiece value,
                                      TargetVolume* volume) {
    if (value == "drive")
      *volume = DRIVE_VOLUME;
    else if (value == "local")
      *volume = LOCAL_VOLUME;
    else if (value == "usb")
      *volume = USB_VOLUME;
    else
      return false;
    return true;
  }
};

// The AddEntriesMessage contains a vector of TestEntryInfo: the elements of
// the vector provide the file or directory entry details.

enum EntryType { FILE, DIRECTORY };

enum SharedOption { NONE, SHARED };

struct TestEntryInfo {
  TestEntryInfo() : type(FILE), shared_option(NONE) {}

  TestEntryInfo(EntryType type,
                const std::string& source_file_name,
                const std::string& target_path,
                const std::string& mime_type,
                SharedOption shared_option,
                const base::Time& last_modified_time)
      : type(type),
        shared_option(shared_option),
        source_file_name(source_file_name),
        target_path(target_path),
        mime_type(mime_type),
        last_modified_time(last_modified_time) {}

  EntryType type;                 // Entry type: file or directory.
  SharedOption shared_option;     // File entry sharing option.
  std::string source_file_name;   // Source file name prototype.
  std::string target_path;        // Target file or directory path.
  std::string mime_type;          // File entry content mime type.
  base::Time last_modified_time;  // Entry last modified time.

  // Registers the member information to the given converter.
  static void RegisterJSONConverter(
      base::JSONValueConverter<TestEntryInfo>* converter) {
    converter->RegisterCustomField("type", &TestEntryInfo::type,
                                   &MapStringToEntryType);
    converter->RegisterStringField("sourceFileName",
                                   &TestEntryInfo::source_file_name);
    converter->RegisterStringField("targetPath", &TestEntryInfo::target_path);
    converter->RegisterStringField("mimeType", &TestEntryInfo::mime_type);
    converter->RegisterCustomField("sharedOption",
                                   &TestEntryInfo::shared_option,
                                   &MapStringToSharedOption);
    converter->RegisterCustomField("lastModifiedTime",
                                   &TestEntryInfo::last_modified_time,
                                   &MapStringToTime);
  }

  // Maps |value| to an EntryType. Returns true on success.
  static bool MapStringToEntryType(base::StringPiece value, EntryType* type) {
    if (value == "file")
      *type = FILE;
    else if (value == "directory")
      *type = DIRECTORY;
    else
      return false;
    return true;
  }

  // Maps |value| to SharedOption. Returns true on success.
  static bool MapStringToSharedOption(base::StringPiece value,
                                      SharedOption* option) {
    if (value == "shared")
      *option = SHARED;
    else if (value == "none")
      *option = NONE;
    else
      return false;
    return true;
  }

  // Maps |value| to base::Time. Returns true on success.
  static bool MapStringToTime(base::StringPiece value, base::Time* time) {
    return base::Time::FromString(value.as_string().c_str(), time);
  }
};

// Listens for chrome.test messages: PASS, FAIL, and SendMessage.
class FileManagerTestMessageListener : public content::NotificationObserver {
 public:
  struct Message {
    int type;
    std::string message;
    scoped_refptr<extensions::TestSendMessageFunction> function;
  };

  FileManagerTestMessageListener() {
    registrar_.Add(this, extensions::NOTIFICATION_EXTENSION_TEST_PASSED,
                   content::NotificationService::AllSources());
    registrar_.Add(this, extensions::NOTIFICATION_EXTENSION_TEST_FAILED,
                   content::NotificationService::AllSources());
    registrar_.Add(this, extensions::NOTIFICATION_EXTENSION_TEST_MESSAGE,
                   content::NotificationService::AllSources());
  }

  Message GetNextMessage() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    if (messages_.empty()) {
      base::RunLoop run_loop;
      quit_closure_ = run_loop.QuitClosure();
      run_loop.Run();
    }

    DCHECK(!messages_.empty());
    const Message next = messages_.front();
    messages_.pop_front();
    return next;
  }

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    Message message{type, std::string(), nullptr};
    if (type == extensions::NOTIFICATION_EXTENSION_TEST_PASSED) {
      test_complete_ = true;
    } else if (type == extensions::NOTIFICATION_EXTENSION_TEST_FAILED) {
      message.message = *content::Details<std::string>(details).ptr();
      test_complete_ = true;
    } else if (type == extensions::NOTIFICATION_EXTENSION_TEST_MESSAGE) {
      message.message = *content::Details<std::string>(details).ptr();
      using SendMessage = content::Source<extensions::TestSendMessageFunction>;
      message.function = SendMessage(source).ptr();
      using WillReply = content::Details<std::pair<std::string, bool*>>;
      *WillReply(details).ptr()->second = true;  // crbug.com/668680
      CHECK(!test_complete_) << "LATE MESSAGE: " << message.message;
    }

    messages_.push_back(message);
    if (quit_closure_) {
      std::move(quit_closure_).Run();
    }
  }

 private:
  bool test_complete_ = false;
  base::OnceClosure quit_closure_;
  base::circular_deque<Message> messages_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(FileManagerTestMessageListener);
};

// Test volume.
class TestVolume {
 protected:
  explicit TestVolume(const std::string& name) : name_(name) {}
  virtual ~TestVolume() = default;

  bool CreateRootDirectory(const Profile* profile) {
    if (root_initialized_)
      return true;
    root_initialized_ = root_.Set(profile->GetPath().Append(name_));
    return root_initialized_;
  }

  const std::string& name() const { return name_; }
  const base::FilePath& root_path() const { return root_.GetPath(); }

  static base::FilePath GetTestDataFilePath(const std::string& file_name) {
    // Get the path to file manager's test data directory.
    base::FilePath source_dir;
    CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &source_dir));
    auto test_data_dir = source_dir.AppendASCII("chrome")
                             .AppendASCII("test")
                             .AppendASCII("data")
                             .AppendASCII("chromeos")
                             .AppendASCII("file_manager");
    // Return full test data path to the given |file_name|.
    return test_data_dir.Append(base::FilePath::FromUTF8Unsafe(file_name));
  }

 private:
  base::ScopedTempDir root_;
  bool root_initialized_ = false;
  std::string name_;

  DISALLOW_COPY_AND_ASSIGN(TestVolume);
};

}  // anonymous namespace

// LocalTestVolume: test volume for a local drive.
class LocalTestVolume : public TestVolume {
 public:
  explicit LocalTestVolume(const std::string& name) : TestVolume(name) {}
  ~LocalTestVolume() override = default;

  // Adds this local volume. Returns true on success.
  virtual bool Mount(Profile* profile) = 0;

  void CreateEntry(const TestEntryInfo& entry) {
    const base::FilePath target_path =
        root_path().AppendASCII(entry.target_path);

    entries_.insert(std::make_pair(target_path, entry));
    switch (entry.type) {
      case FILE: {
        const base::FilePath source_path =
            TestVolume::GetTestDataFilePath(entry.source_file_name);
        ASSERT_TRUE(base::CopyFile(source_path, target_path))
            << "Copy from " << source_path.value() << " to "
            << target_path.value() << " failed.";
        break;
      }
      case DIRECTORY:
        ASSERT_TRUE(base::CreateDirectory(target_path))
            << "Failed to create a directory: " << target_path.value();
        break;
    }

    ASSERT_TRUE(UpdateModifiedTime(entry));
  }

 private:
  // Updates ModifiedTime of the entry and its parents by referring
  // TestEntryInfo. Returns true on success.
  bool UpdateModifiedTime(const TestEntryInfo& entry) {
    const base::FilePath path = root_path().AppendASCII(entry.target_path);
    if (!base::TouchFile(path, entry.last_modified_time,
                         entry.last_modified_time))
      return false;

    // Update the modified time of parent directories because it may be also
    // affected by the update of child items.
    if (path.DirName() != root_path()) {
      const std::map<base::FilePath, const TestEntryInfo>::iterator it =
          entries_.find(path.DirName());
      if (it == entries_.end())
        return false;
      return UpdateModifiedTime(it->second);
    }

    return true;
  }

  std::map<base::FilePath, const TestEntryInfo> entries_;

  DISALLOW_COPY_AND_ASSIGN(LocalTestVolume);
};

// DownloadsTestVolume: local test volume for the "Downloads" directory.
class DownloadsTestVolume : public LocalTestVolume {
 public:
  DownloadsTestVolume() : LocalTestVolume("Downloads") {}
  ~DownloadsTestVolume() override = default;

  bool Mount(Profile* profile) override {
    if (!CreateRootDirectory(profile))
      return false;
    auto* volume = VolumeManager::Get(profile);
    return volume->RegisterDownloadsDirectoryForTesting(root_path());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsTestVolume);
};

// FakeTestVolume: local test volume with a given volume and device type.
class FakeTestVolume : public LocalTestVolume {
 public:
  FakeTestVolume(const std::string& name,
                 VolumeType volume_type,
                 chromeos::DeviceType device_type)
      : LocalTestVolume(name),
        volume_type_(volume_type),
        device_type_(device_type) {}
  ~FakeTestVolume() override = default;

  // Add the fake test volume entries.
  bool PrepareTestEntries(Profile* profile) {
    if (!CreateRootDirectory(profile))
      return false;

    // Note: must be kept in sync with BASIC_FAKE_ENTRY_SET defined in the
    // integration_tests/file_manager JS code.
    CreateEntry(TestEntryInfo(FILE, "text.txt", "hello.txt", "text/plain",
                              SharedOption::NONE, base::Time::Now()));
    CreateEntry(TestEntryInfo(DIRECTORY, std::string(), "A", std::string(),
                              SharedOption::NONE, base::Time::Now()));
    base::RunLoop().RunUntilIdle();
    return true;
  }

  bool Mount(Profile* profile) override {
    if (!CreateRootDirectory(profile))
      return false;

    // Revoke name() mount point first, then re-add its mount point.
    GetMountPoints()->RevokeFileSystem(name());
    const bool added = GetMountPoints()->RegisterFileSystem(
        name(), storage::kFileSystemTypeNativeLocal,
        storage::FileSystemMountOption(), root_path());
    if (!added)
      return false;

    // Expose the mount point with the given volume and device type.
    VolumeManager::Get(profile)->AddVolumeForTesting(root_path(), volume_type_,
                                                     device_type_, read_only_);
    base::RunLoop().RunUntilIdle();
    return true;
  }

 private:
  storage::ExternalMountPoints* GetMountPoints() {
    return storage::ExternalMountPoints::GetSystemInstance();
  }

  const VolumeType volume_type_;
  const chromeos::DeviceType device_type_;
  const bool read_only_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeTestVolume);
};

// DriveTestVolume: test volume for Google Drive.
class DriveTestVolume : public TestVolume {
 public:
  DriveTestVolume() : TestVolume("drive") {}
  ~DriveTestVolume() override = default;

  void CreateEntry(const TestEntryInfo& entry) {
    const base::FilePath path =
        base::FilePath::FromUTF8Unsafe(entry.target_path);
    const std::string target_name = path.BaseName().AsUTF8Unsafe();

    // Obtain the parent entry.
    drive::FileError error = drive::FILE_ERROR_OK;
    std::unique_ptr<drive::ResourceEntry> parent_entry(
        new drive::ResourceEntry);
    integration_service_->file_system()->GetResourceEntry(
        drive::util::GetDriveMyDriveRootPath().Append(path).DirName(),
        google_apis::test_util::CreateCopyResultCallback(&error,
                                                         &parent_entry));
    content::RunAllTasksUntilIdle();
    ASSERT_EQ(drive::FILE_ERROR_OK, error);
    ASSERT_TRUE(parent_entry);

    switch (entry.type) {
      case FILE:
        CreateFile(entry.source_file_name, parent_entry->resource_id(),
                   target_name, entry.mime_type, entry.shared_option == SHARED,
                   entry.last_modified_time);
        break;
      case DIRECTORY:
        CreateDirectory(parent_entry->resource_id(), target_name,
                        entry.last_modified_time);
        break;
    }

    // Files and directories in drive will only appear after CheckUpdates
    // has completed.
    CheckForUpdates();
    content::RunAllTasksUntilIdle();
  }

  // Creates an empty directory with the given |name| and |modification_time|.
  void CreateDirectory(const std::string& parent_id,
                       const std::string& target_name,
                       const base::Time& modification_time) {
    google_apis::DriveApiErrorCode error = google_apis::DRIVE_OTHER_ERROR;

    std::unique_ptr<google_apis::FileResource> entry;
    fake_drive_service_->AddNewDirectory(
        parent_id, target_name, drive::AddNewDirectoryOptions(),
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(google_apis::HTTP_CREATED, error);
    ASSERT_TRUE(entry);

    fake_drive_service_->SetLastModifiedTime(
        entry->file_id(), modification_time,
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(error == google_apis::HTTP_SUCCESS);
    ASSERT_TRUE(entry);

    CheckForUpdates();
  }

  // Creates a test file with the given spec.
  // Serves |test_file_name| file. Pass an empty string for an empty file.
  void CreateFile(const std::string& source_file_name,
                  const std::string& parent_id,
                  const std::string& target_name,
                  const std::string& mime_type,
                  bool shared_with_me,
                  const base::Time& modification_time) {
    google_apis::DriveApiErrorCode error = google_apis::DRIVE_OTHER_ERROR;

    std::string content_data;
    if (!source_file_name.empty()) {
      base::FilePath source_path =
          TestVolume::GetTestDataFilePath(source_file_name);
      ASSERT_TRUE(base::ReadFileToString(source_path, &content_data));
    }

    std::unique_ptr<google_apis::FileResource> entry;
    fake_drive_service_->AddNewFile(
        mime_type, content_data, parent_id, target_name, shared_with_me,
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(google_apis::HTTP_CREATED, error);
    ASSERT_TRUE(entry);

    fake_drive_service_->SetLastModifiedTime(
        entry->file_id(), modification_time,
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(google_apis::HTTP_SUCCESS, error);
    ASSERT_TRUE(entry);

    CheckForUpdates();
  }

  // Notifies FileSystem that the contents in FakeDriveService have changed,
  // hence the new contents should be fetched.
  void CheckForUpdates() {
    if (integration_service_ && integration_service_->file_system()) {
      integration_service_->file_system()->CheckForUpdates();
    }
  }

  // Sets the url base for the test server to be used to generate share urls
  // on the files and directories.
  void ConfigureShareUrlBase(const GURL& share_url_base) {
    fake_drive_service_->set_share_url_base(share_url_base);
  }

  drive::DriveIntegrationService* CreateDriveIntegrationService(
      Profile* profile) {
    if (!CreateRootDirectory(profile))
      return nullptr;

    EXPECT_FALSE(profile_);
    profile_ = profile;

    EXPECT_FALSE(fake_drive_service_);
    fake_drive_service_ = new drive::FakeDriveService;
    fake_drive_service_->LoadAppListForDriveApi("drive/applist.json");

    EXPECT_FALSE(integration_service_);
    integration_service_ = new drive::DriveIntegrationService(
        profile, nullptr, fake_drive_service_, std::string(), root_path(),
        nullptr);

    return integration_service_;
  }

 private:
  // Profile associated with this volume: not owned.
  Profile* profile_ = nullptr;
  // Fake drive service used for testing: not owned.
  drive::FakeDriveService* fake_drive_service_ = nullptr;
  // Integration service used for testing: not owned.
  drive::DriveIntegrationService* integration_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(DriveTestVolume);
};

FileManagerBrowserTestBase::FileManagerBrowserTestBase() = default;

FileManagerBrowserTestBase::~FileManagerBrowserTestBase() = default;

void FileManagerBrowserTestBase::SetUp() {
  net::NetworkChangeNotifier::SetTestNotificationsOnly(true);
  extensions::ExtensionApiTest::SetUp();
}

void FileManagerBrowserTestBase::SetUpCommandLine(
    base::CommandLine* command_line) {
  // Use a fake audio stream crbug.com/835626
  command_line->AppendSwitch(switches::kDisableAudioOutput);

  if (IsGuestModeTest()) {
    command_line->AppendSwitch(chromeos::switches::kGuestSession);
    command_line->AppendSwitchNative(chromeos::switches::kLoginUser, "");
    command_line->AppendSwitch(switches::kIncognito);
  }

  if (IsIncognitoModeTest()) {
    command_line->AppendSwitch(switches::kIncognito);
  }

  // Block NaCl loading Files.app components crbug.com/788671
  command_line->AppendSwitch(chromeos::switches::kDisableZipArchiverUnpacker);
  command_line->AppendSwitch(chromeos::switches::kDisableZipArchiverPacker);

  extensions::ExtensionApiTest::SetUpCommandLine(command_line);
}

void FileManagerBrowserTestBase::SetUpInProcessBrowserTestFixture() {
  extensions::ExtensionApiTest::SetUpInProcessBrowserTestFixture();

  local_volume_ = std::make_unique<DownloadsTestVolume>();

  if (!IsGuestModeTest()) {
    create_drive_integration_service_ =
        base::Bind(&FileManagerBrowserTestBase::CreateDriveIntegrationService,
                   base::Unretained(this));
    service_factory_for_test_ = std::make_unique<
        drive::DriveIntegrationServiceFactory::ScopedFactoryForTest>(
        &create_drive_integration_service_);
  }
}

void FileManagerBrowserTestBase::SetUpOnMainThread() {
  extensions::ExtensionApiTest::SetUpOnMainThread();
  CHECK(profile());

  CHECK(local_volume_->Mount(profile()));

  if (!IsGuestModeTest()) {
    // Start the embedded test server to serve the mocked share dialog.
    CHECK(embedded_test_server()->Start());
    const GURL share_url_base(embedded_test_server()->GetURL(
        "/chromeos/file_manager/share_dialog_mock/index.html"));
    drive_volume_ = std::move(drive_volumes_[profile()->GetOriginalProfile()]);
    drive_volume_->ConfigureShareUrlBase(share_url_base);
    test_util::WaitUntilDriveMountPointIsAdded(profile());
  }

  display_service_ =
      std::make_unique<NotificationDisplayServiceTester>(profile());

  // The test resources are setup: enable and add default ChromeOS component
  // extensions now and not before: crbug.com/831074, crbug.com/804413
  extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
  ExtensionService* service =
      extensions::ExtensionSystem::Get(profile())->extension_service();
  service->component_loader()->AddDefaultComponentExtensions(false);

  // The File Manager component extension should have been added for loading
  // into the user profile, but not into the sign-in profile.
  CHECK(extensions::ExtensionSystem::Get(profile())
            ->extension_service()
            ->component_loader()
            ->Exists(kFileManagerAppId));
  CHECK(!extensions::ExtensionSystem::Get(
             chromeos::ProfileHelper::GetSigninProfile())
             ->extension_service()
             ->component_loader()
             ->Exists(kFileManagerAppId));
}

void FileManagerBrowserTestBase::StartTest() {
  LOG(INFO) << "FileManagerBrowserTest::StartTest " << GetTestCaseName();
  static const base::FilePath test_extension_dir =
      base::FilePath(FILE_PATH_LITERAL("ui/file_manager/integration_tests"));
  LaunchExtension(test_extension_dir, GetTestExtensionManifestName());
  RunTestMessageLoop();
}

void FileManagerBrowserTestBase::LaunchExtension(const base::FilePath& path,
                                                 const char* manifest_name) {
  base::FilePath source_dir;
  CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &source_dir));

  const base::FilePath source_path = source_dir.Append(path);
  const extensions::Extension* const extension_launched =
      LoadExtensionAsComponentWithManifest(source_path, manifest_name);
  CHECK(extension_launched) << "Launching: " << manifest_name;
}

void FileManagerBrowserTestBase::RunTestMessageLoop() {
  FileManagerTestMessageListener listener;

  while (true) {
    auto message = listener.GetNextMessage();

    if (message.type == extensions::NOTIFICATION_EXTENSION_TEST_PASSED)
      return;  // Test PASSED.
    if (message.type == extensions::NOTIFICATION_EXTENSION_TEST_FAILED) {
      ADD_FAILURE() << message.message;
      return;  // Test FAILED.
    }

    // If the message in JSON format has no command, ignore it
    // but note a reply is required: use std::string().
    const auto json = base::JSONReader::Read(message.message);
    const base::DictionaryValue* dictionary = nullptr;
    std::string command;
    if (!json || !json->GetAsDictionary(&dictionary) ||
        !dictionary->GetString("name", &command)) {
      message.function->Reply(std::string());
      continue;
    }

    // Process the command, reply with the result.
    std::string result;
    OnCommand(command, *dictionary, &result);
    if (!HasFatalFailure()) {
      message.function->Reply(result);
      continue;
    }

    // Test FAILED: while processing the command.
    LOG(INFO) << "[FAILED] " << GetTestCaseName();
    return;
  }
}

void FileManagerBrowserTestBase::OnCommand(const std::string& name,
                                           const base::DictionaryValue& value,
                                           std::string* output) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  if (name == "isInGuestMode") {
    // Obtain if the test runs in guest or incognito mode, or not.
    if (IsGuestModeTest() || IsIncognitoModeTest()) {
      LOG(INFO) << GetTestCaseName() << " isInGuestMode: true";
      *output = "true";
    } else {
      ASSERT_EQ(NOT_IN_GUEST_MODE, GetGuestMode());
      *output = "false";
    }

    return;
  }

  if (name == "getRootPaths") {
    // Obtain the root paths.
    const auto downloads_root = util::GetDownloadsMountPointName(profile());
    const auto drive = drive::util::GetDriveMountPointPath(profile());

    base::DictionaryValue dictionary;
    auto drive_root = drive.BaseName().AsUTF8Unsafe().append("/root");
    dictionary.SetString("drive", "/" + drive_root);
    dictionary.SetString("downloads", "/" + downloads_root);

    base::JSONWriter::Write(dictionary, output);
    return;
  }

  if (name == "getTestName") {
    // Obtain the test case name.
    *output = GetTestCaseName();
    return;
  }

  if (name == "getCwsWidgetContainerMockUrl") {
    // Obtain the mock CWS widget container URL and URL.origin.
    const GURL url = embedded_test_server()->GetURL(
        "/chromeos/file_manager/cws_container_mock/index.html");
    std::string origin = url.GetOrigin().spec();
    if (*origin.rbegin() == '/')  // Strip origin trailing '/'.
      origin.resize(origin.length() - 1);

    base::DictionaryValue dictionary;
    dictionary.SetString("url", url.spec());
    dictionary.SetString("origin", origin);

    base::JSONWriter::Write(dictionary, output);
    return;
  }

  if (name == "addEntries") {
    // Add the message.entries to the message.volume.
    AddEntriesMessage message;
    ASSERT_TRUE(AddEntriesMessage::ConvertJSONValue(value, &message));

    for (size_t i = 0; i < message.entries.size(); ++i) {
      switch (message.volume) {
        case LOCAL_VOLUME:
          local_volume_->CreateEntry(*message.entries[i]);
          break;
        case DRIVE_VOLUME:
          if (drive_volume_) {
            drive_volume_->CreateEntry(*message.entries[i]);
          } else if (!IsGuestModeTest()) {
            LOG(FATAL) << "Add entry: but no Drive volume.";
          }
          break;
        case USB_VOLUME:
          if (usb_volume_) {
            usb_volume_->CreateEntry(*message.entries[i]);
          } else {
            LOG(FATAL) << "Add entry: but no USB volume.";
          }
          break;
      }
    }

    return;
  }

  if (name == "mountFakeUsb" || name == "mountFakeUsbEmpty") {
    usb_volume_ = std::make_unique<FakeTestVolume>(
        "fake-usb", VOLUME_TYPE_REMOVABLE_DISK_PARTITION,
        chromeos::DEVICE_TYPE_USB);

    if (name == "mountFakeUsb")
      ASSERT_TRUE(usb_volume_->PrepareTestEntries(profile()));

    ASSERT_TRUE(usb_volume_->Mount(profile()));
    return;
  }

  if (name == "mountFakeMtp" || name == "mountFakeMtpEmpty") {
    mtp_volume_ = std::make_unique<FakeTestVolume>(
        "fake-mtp", VOLUME_TYPE_MTP, chromeos::DEVICE_TYPE_UNKNOWN);

    if (name == "mountFakeMtp")
      ASSERT_TRUE(mtp_volume_->PrepareTestEntries(profile()));

    ASSERT_TRUE(mtp_volume_->Mount(profile()));
    return;
  }

  if (name == "useCellularNetwork") {
    net::NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChangeForTests(
        net::NetworkChangeNotifier::GetMaxBandwidthMbpsForConnectionSubtype(
            net::NetworkChangeNotifier::SUBTYPE_HSPA),
        net::NetworkChangeNotifier::CONNECTION_3G);
    return;
  }

  if (name == "clickNotificationButton") {
    std::string extension_id;
    std::string notification_id;
    ASSERT_TRUE(value.GetString("extensionId", &extension_id));
    ASSERT_TRUE(value.GetString("notificationId", &notification_id));

    const std::string delegate_id = extension_id + "-" + notification_id;
    base::Optional<message_center::Notification> notification =
        display_service_->GetNotification(delegate_id);
    EXPECT_TRUE(notification);

    int index;
    ASSERT_TRUE(value.GetInteger("index", &index));
    display_service_->SimulateClick(NotificationHandler::Type::EXTENSION,
                                    delegate_id, index, base::nullopt);
    return;
  }

  if (name == "launchProviderExtension") {
    std::string manifest;
    ASSERT_TRUE(value.GetString("manifest", &manifest));
    LaunchExtension(base::FilePath(FILE_PATH_LITERAL(
                        "ui/file_manager/integration_tests/testing_provider")),
                    manifest.c_str());
    return;
  }

  FAIL() << "Unknown test message: " << name;
}

drive::DriveIntegrationService*
FileManagerBrowserTestBase::CreateDriveIntegrationService(Profile* profile) {
  drive_volumes_[profile->GetOriginalProfile()] =
      std::make_unique<DriveTestVolume>();
  return drive_volumes_[profile->GetOriginalProfile()]
      ->CreateDriveIntegrationService(profile);
}

}  // namespace file_manager
