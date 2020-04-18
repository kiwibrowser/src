// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/resource_entry_conversion.h"

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "components/drive/drive.pb.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system_core_util.h"
#include "google_apis/drive/drive_api_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {

namespace {

base::Time GetTestTime() {
  // 2011-12-14-T00:40:47.330Z
  base::Time::Exploded exploded;
  exploded.year = 2011;
  exploded.month = 12;
  exploded.day_of_month = 14;
  exploded.day_of_week = 2;  // Tuesday
  exploded.hour = 0;
  exploded.minute = 40;
  exploded.second = 47;
  exploded.millisecond = 330;
  base::Time out_time;
  EXPECT_TRUE(base::Time::FromUTCExploded(exploded, &out_time));
  return out_time;
}

}  // namespace

TEST(ResourceEntryConversionTest, ConvertToResourceEntry_File) {
  google_apis::FileResource file_resource;
  file_resource.set_title("File 1.mp3");
  file_resource.set_file_id("resource_id");
  file_resource.set_created_date(GetTestTime());
  file_resource.set_modified_date(
      GetTestTime() + base::TimeDelta::FromSeconds(10));
  file_resource.set_modified_by_me_date(GetTestTime() +
                                        base::TimeDelta::FromSeconds(5));
  file_resource.set_mime_type("audio/mpeg");
  file_resource.set_alternate_link(GURL("https://file_link_alternate"));
  file_resource.set_file_size(892721);
  file_resource.set_md5_checksum("3b4382ebefec6e743578c76bbd0575ce");

  google_apis::FileResourceCapabilities capabilities;
  capabilities.set_can_copy(true);
  capabilities.set_can_delete(false);
  capabilities.set_can_rename(true);
  capabilities.set_can_add_children(false);
  capabilities.set_can_share(true);
  file_resource.set_capabilities(capabilities);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertFileResourceToResourceEntry(
      file_resource, &entry, &parent_resource_id));

  EXPECT_EQ(file_resource.title(), entry.title());
  EXPECT_EQ(file_resource.title(), entry.base_name());
  EXPECT_EQ(file_resource.file_id(), entry.resource_id());
  EXPECT_EQ("", parent_resource_id);

  EXPECT_FALSE(entry.deleted());
  EXPECT_FALSE(entry.starred());
  EXPECT_FALSE(entry.shared_with_me());
  EXPECT_FALSE(entry.shared());

  EXPECT_EQ(file_resource.modified_date().ToInternalValue(),
            entry.file_info().last_modified());
  EXPECT_EQ(file_resource.modified_by_me_date().ToInternalValue(),
            entry.last_modified_by_me());
  // Last accessed value equal to 0 means that the file has never been viewed.
  EXPECT_EQ(0, entry.file_info().last_accessed());
  EXPECT_EQ(file_resource.created_date().ToInternalValue(),
            entry.file_info().creation_time());
  EXPECT_EQ(file_resource.alternate_link().spec(), entry.alternate_url());

  EXPECT_EQ(file_resource.mime_type(),
            entry.file_specific_info().content_mime_type());
  EXPECT_FALSE(entry.file_specific_info().is_hosted_document());

  // Regular file specific fields.
  EXPECT_EQ(file_resource.file_size(), entry.file_info().size());
  EXPECT_EQ(file_resource.md5_checksum(), entry.file_specific_info().md5());
  EXPECT_FALSE(entry.file_info().is_directory());

  // Capabilities.
  EXPECT_TRUE(entry.capabilities_info().can_copy());
  EXPECT_FALSE(entry.capabilities_info().can_delete());
  EXPECT_TRUE(entry.capabilities_info().can_rename());
  EXPECT_FALSE(entry.capabilities_info().can_add_children());
  EXPECT_TRUE(entry.capabilities_info().can_share());
}

TEST(ResourceEntryConversionTest,
     ConvertFileResourceToResourceEntry_HostedDocument) {
  google_apis::FileResource file_resource;
  file_resource.set_title("Document 1");
  file_resource.set_file_id("resource_id");
  file_resource.set_created_date(GetTestTime());
  file_resource.set_modified_date(
      GetTestTime() + base::TimeDelta::FromSeconds(10));
  file_resource.set_modified_by_me_date(GetTestTime() +
                                        base::TimeDelta::FromSeconds(5));
  file_resource.set_last_viewed_by_me_date(
      GetTestTime() + base::TimeDelta::FromSeconds(20));
  file_resource.set_mime_type(util::kGoogleDocumentMimeType);
  file_resource.set_alternate_link(GURL("https://file_link_alternate"));
  // Do not set file size to represent a hosted document.

  google_apis::FileResourceCapabilities capabilities;
  capabilities.set_can_copy(false);
  capabilities.set_can_delete(true);
  capabilities.set_can_rename(false);
  capabilities.set_can_add_children(true);
  capabilities.set_can_share(false);
  file_resource.set_capabilities(capabilities);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertFileResourceToResourceEntry(
      file_resource, &entry, &parent_resource_id));

  EXPECT_EQ(file_resource.title(), entry.title());
  EXPECT_EQ(file_resource.title() + ".gdoc",
            entry.base_name());  // The suffix added.
  EXPECT_EQ(".gdoc", entry.file_specific_info().document_extension());
  EXPECT_EQ(file_resource.file_id(), entry.resource_id());
  EXPECT_EQ("", parent_resource_id);

  EXPECT_FALSE(entry.deleted());
  EXPECT_FALSE(entry.starred());
  EXPECT_FALSE(entry.shared_with_me());
  EXPECT_FALSE(entry.shared());

  EXPECT_EQ(file_resource.modified_date().ToInternalValue(),
            entry.file_info().last_modified());
  EXPECT_EQ(file_resource.modified_by_me_date().ToInternalValue(),
            entry.last_modified_by_me());
  EXPECT_EQ(file_resource.last_viewed_by_me_date().ToInternalValue(),
            entry.file_info().last_accessed());
  EXPECT_EQ(file_resource.created_date().ToInternalValue(),
            entry.file_info().creation_time());
  EXPECT_EQ(file_resource.alternate_link().spec(), entry.alternate_url());

  EXPECT_EQ(file_resource.mime_type(),
            entry.file_specific_info().content_mime_type());
  EXPECT_TRUE(entry.file_specific_info().is_hosted_document());

  // The size should be 0 for a hosted document.
  EXPECT_EQ(0, entry.file_info().size());
  EXPECT_FALSE(entry.file_info().is_directory());

  // Capabilities.
  EXPECT_FALSE(entry.capabilities_info().can_copy());
  EXPECT_TRUE(entry.capabilities_info().can_delete());
  EXPECT_FALSE(entry.capabilities_info().can_rename());
  EXPECT_TRUE(entry.capabilities_info().can_add_children());
  EXPECT_FALSE(entry.capabilities_info().can_share());
}

TEST(ResourceEntryConversionTest,
     ConvertFileResourceToResourceEntry_Directory) {
  google_apis::FileResource file_resource;
  file_resource.set_title("Folder");
  file_resource.set_file_id("resource_id");
  file_resource.set_created_date(GetTestTime());
  file_resource.set_modified_date(
      GetTestTime() + base::TimeDelta::FromSeconds(10));
  file_resource.set_modified_by_me_date(GetTestTime() +
                                        base::TimeDelta::FromSeconds(5));
  file_resource.set_last_viewed_by_me_date(
      GetTestTime() + base::TimeDelta::FromSeconds(20));
  file_resource.set_mime_type(util::kDriveFolderMimeType);

  google_apis::ParentReference parent;
  parent.set_file_id("parent_resource_id");
  file_resource.mutable_parents()->push_back(parent);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertFileResourceToResourceEntry(
      file_resource, &entry, &parent_resource_id));

  EXPECT_EQ(file_resource.title(), entry.title());
  EXPECT_EQ(file_resource.title(), entry.base_name());
  EXPECT_EQ(file_resource.file_id(), entry.resource_id());
  EXPECT_EQ(file_resource.alternate_link().spec(), entry.alternate_url());
  // The parent resource ID should be obtained as this is a sub directory
  // under a non-root directory.
  EXPECT_EQ(parent.file_id(), parent_resource_id);

  EXPECT_FALSE(entry.deleted());
  EXPECT_FALSE(entry.starred());
  EXPECT_FALSE(entry.shared_with_me());
  EXPECT_FALSE(entry.shared());

  EXPECT_EQ(file_resource.modified_date().ToInternalValue(),
            entry.file_info().last_modified());
  EXPECT_EQ(file_resource.modified_by_me_date().ToInternalValue(),
            entry.last_modified_by_me());
  EXPECT_EQ(file_resource.last_viewed_by_me_date().ToInternalValue(),
            entry.file_info().last_accessed());
  EXPECT_EQ(file_resource.created_date().ToInternalValue(),
            entry.file_info().creation_time());

  EXPECT_TRUE(entry.file_info().is_directory());
}

TEST(ResourceEntryConversionTest,
     ConvertFileResourceToResourceEntry_DeletedHostedDocument) {
  google_apis::FileResource file_resource;
  file_resource.set_title("Document 1");
  file_resource.set_file_id("resource_id");
  file_resource.set_created_date(GetTestTime());
  file_resource.set_modified_date(
      GetTestTime() + base::TimeDelta::FromSeconds(10));
  file_resource.set_modified_by_me_date(GetTestTime() +
                                        base::TimeDelta::FromSeconds(5));
  file_resource.set_last_viewed_by_me_date(
      GetTestTime() + base::TimeDelta::FromSeconds(20));
  file_resource.set_mime_type(util::kGoogleDocumentMimeType);
  file_resource.set_alternate_link(GURL("https://file_link_alternate"));
  file_resource.mutable_labels()->set_trashed(true);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertFileResourceToResourceEntry(
      file_resource, &entry, &parent_resource_id));

  EXPECT_EQ(file_resource.title(), entry.title());
  EXPECT_EQ(file_resource.title() + ".gdoc", entry.base_name());
  EXPECT_EQ(file_resource.file_id(), entry.resource_id());
  EXPECT_EQ(file_resource.alternate_link().spec(), entry.alternate_url());
  EXPECT_EQ("", parent_resource_id);

  EXPECT_TRUE(entry.deleted());  // The document was deleted.
  EXPECT_FALSE(entry.starred());
  EXPECT_FALSE(entry.shared_with_me());
  EXPECT_FALSE(entry.shared());

  EXPECT_EQ(file_resource.modified_date().ToInternalValue(),
            entry.file_info().last_modified());
  EXPECT_EQ(file_resource.modified_by_me_date().ToInternalValue(),
            entry.last_modified_by_me());
  EXPECT_EQ(file_resource.last_viewed_by_me_date().ToInternalValue(),
            entry.file_info().last_accessed());
  EXPECT_EQ(file_resource.created_date().ToInternalValue(),
            entry.file_info().creation_time());

  EXPECT_EQ(file_resource.mime_type(),
            entry.file_specific_info().content_mime_type());
  EXPECT_TRUE(entry.file_specific_info().is_hosted_document());

  // The size should be 0 for a hosted document.
  EXPECT_EQ(0, entry.file_info().size());
}

TEST(ResourceEntryConversionTest, ConvertChangeResourceToResourceEntry) {
  google_apis::ChangeResource change_resource;
  change_resource.set_type(google_apis::ChangeResource::FILE);
  change_resource.set_file(base::WrapUnique(new google_apis::FileResource));
  change_resource.set_file_id("resource_id");
  change_resource.set_modification_date(GetTestTime());

  google_apis::FileResource* file_resource = change_resource.mutable_file();
  file_resource->set_title("File 1.mp3");
  file_resource->set_file_id("resource_id");
  // Set dummy file size to declare that this is a regular file.
  file_resource->set_file_size(12345);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertChangeResourceToResourceEntry(
      change_resource, &entry, &parent_resource_id));

  EXPECT_EQ(change_resource.file_id(), entry.resource_id());
  EXPECT_EQ(change_resource.modification_date().ToInternalValue(),
            entry.modification_date());

  EXPECT_EQ(file_resource->title(), entry.title());
  EXPECT_EQ(file_resource->title(), entry.base_name());
  EXPECT_EQ("", parent_resource_id);

  EXPECT_FALSE(entry.deleted());
}

TEST(ResourceEntryConversionTest,
     ConvertChangeResourceToResourceEntry_Trashed) {
  google_apis::ChangeResource change_resource;
  change_resource.set_type(google_apis::ChangeResource::FILE);
  change_resource.set_file(base::WrapUnique(new google_apis::FileResource));
  change_resource.set_file_id("resource_id");
  change_resource.set_modification_date(GetTestTime());

  google_apis::FileResource* file_resource = change_resource.mutable_file();
  file_resource->set_title("File 1.mp3");
  file_resource->set_file_id("resource_id");
  // Set dummy file size to declare that this is a regular file.
  file_resource->set_file_size(12345);
  file_resource->mutable_labels()->set_trashed(true);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertChangeResourceToResourceEntry(
      change_resource, &entry, &parent_resource_id));

  EXPECT_EQ(change_resource.file_id(), entry.resource_id());
  EXPECT_EQ(change_resource.modification_date().ToInternalValue(),
            entry.modification_date());

  EXPECT_EQ(file_resource->title(), entry.title());
  EXPECT_EQ(file_resource->title(), entry.base_name());
  EXPECT_EQ("", parent_resource_id);

  EXPECT_TRUE(entry.deleted());
}

TEST(ResourceEntryConversionTest,
     ConvertChangeResourceToResourceEntry_Deleted) {
  google_apis::ChangeResource change_resource;
  change_resource.set_type(google_apis::ChangeResource::FILE);
  change_resource.set_deleted(true);
  change_resource.set_file_id("resource_id");
  change_resource.set_modification_date(GetTestTime());

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertChangeResourceToResourceEntry(
      change_resource, &entry, &parent_resource_id));

  EXPECT_EQ(change_resource.file_id(), entry.resource_id());
  EXPECT_EQ("", parent_resource_id);

  EXPECT_TRUE(entry.deleted());

  EXPECT_EQ(change_resource.modification_date().ToInternalValue(),
            entry.modification_date());
}

TEST(ResourceEntryConversionTest,
     ConvertFileResourceToResourceEntry_StarredEntry) {
  google_apis::FileResource file_resource;
  file_resource.mutable_labels()->set_starred(true);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertFileResourceToResourceEntry(
      file_resource, &entry, &parent_resource_id));
  EXPECT_TRUE(entry.starred());
}

TEST(ResourceEntryConversionTest,
     ConvertFileResourceToResourceEntry_SharedWithMeEntry) {
  google_apis::FileResource file_resource;
  file_resource.set_shared(true);
  file_resource.set_shared_with_me_date(GetTestTime());

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertFileResourceToResourceEntry(
      file_resource, &entry, &parent_resource_id));
  EXPECT_TRUE(entry.shared_with_me());
  EXPECT_TRUE(entry.shared());
}

TEST(ResourceEntryConversionTest, ToPlatformFileInfo) {
  ResourceEntry entry;
  entry.mutable_file_info()->set_size(12345);
  entry.mutable_file_info()->set_is_directory(true);
  entry.mutable_file_info()->set_is_symbolic_link(true);
  entry.mutable_file_info()->set_creation_time(999);
  entry.mutable_file_info()->set_last_modified(123456789);
  entry.mutable_file_info()->set_last_accessed(987654321);

  base::File::Info file_info;
  ConvertResourceEntryToFileInfo(entry, &file_info);
  EXPECT_EQ(entry.file_info().size(), file_info.size);
  EXPECT_EQ(entry.file_info().is_directory(), file_info.is_directory);
  EXPECT_EQ(entry.file_info().is_symbolic_link(), file_info.is_symbolic_link);
  EXPECT_EQ(base::Time::FromInternalValue(entry.file_info().creation_time()),
            file_info.creation_time);
  EXPECT_EQ(base::Time::FromInternalValue(entry.file_info().last_modified()),
            file_info.last_modified);
  EXPECT_EQ(base::Time::FromInternalValue(entry.file_info().last_accessed()),
            file_info.last_accessed);
}

TEST(ResourceEntryConversionTest,
     ConvertFileResourceToResourceEntry_ImageMediaMetadata) {
  google_apis::FileResource entry_all_fields;
  google_apis::FileResource entry_zero_fields;
  google_apis::FileResource entry_no_fields;

  entry_all_fields.mutable_image_media_metadata()->set_width(640);
  entry_all_fields.mutable_image_media_metadata()->set_height(480);
  entry_all_fields.mutable_image_media_metadata()->set_rotation(90);

  entry_zero_fields.mutable_image_media_metadata()->set_width(0);
  entry_zero_fields.mutable_image_media_metadata()->set_height(0);
  entry_zero_fields.mutable_image_media_metadata()->set_rotation(0);

  {
    ResourceEntry entry;
    std::string parent_resource_id;
    EXPECT_TRUE(ConvertFileResourceToResourceEntry(
        entry_all_fields, &entry, &parent_resource_id));
    EXPECT_EQ(640, entry.file_specific_info().image_width());
    EXPECT_EQ(480, entry.file_specific_info().image_height());
    EXPECT_EQ(90, entry.file_specific_info().image_rotation());
  }
  {
    ResourceEntry entry;
    std::string parent_resource_id;
    EXPECT_TRUE(ConvertFileResourceToResourceEntry(
        entry_zero_fields, &entry, &parent_resource_id));
    EXPECT_TRUE(entry.file_specific_info().has_image_width());
    EXPECT_TRUE(entry.file_specific_info().has_image_height());
    EXPECT_TRUE(entry.file_specific_info().has_image_rotation());
    EXPECT_EQ(0, entry.file_specific_info().image_width());
    EXPECT_EQ(0, entry.file_specific_info().image_height());
    EXPECT_EQ(0, entry.file_specific_info().image_rotation());
  }
  {
    ResourceEntry entry;
    std::string parent_resource_id;
    EXPECT_TRUE(ConvertFileResourceToResourceEntry(
        entry_no_fields, &entry, &parent_resource_id));
    EXPECT_FALSE(entry.file_specific_info().has_image_width());
    EXPECT_FALSE(entry.file_specific_info().has_image_height());
    EXPECT_FALSE(entry.file_specific_info().has_image_rotation());
  }
}

TEST(ResourceEntryConversionTest,
     ConvertTeamDriveChangeResourceToResourceEntry) {
  google_apis::ChangeResource change_resource;
  change_resource.set_type(google_apis::ChangeResource::TEAM_DRIVE);
  change_resource.set_team_drive(
      base::WrapUnique(new google_apis::TeamDriveResource));
  change_resource.set_team_drive_id("team_drive_id");
  change_resource.set_modification_date(GetTestTime());
  change_resource.set_deleted(false);

  google_apis::TeamDriveResource* team_drive_resource =
      change_resource.mutable_team_drive();
  team_drive_resource->set_name("ABC Team Drive");
  team_drive_resource->set_id("team_drive_id");

  google_apis::TeamDriveCapabilities team_drive_capabilities;
  team_drive_capabilities.set_can_copy(true);
  team_drive_capabilities.set_can_delete_team_drive(false);
  team_drive_capabilities.set_can_rename_team_drive(true);
  // Can_rename is ignored for team drives.
  team_drive_capabilities.set_can_rename(false);
  team_drive_capabilities.set_can_add_children(false);
  team_drive_capabilities.set_can_share(true);
  team_drive_resource->set_capabilities(team_drive_capabilities);

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertChangeResourceToResourceEntry(change_resource, &entry,
                                                   &parent_resource_id));

  EXPECT_EQ(change_resource.team_drive_id(), entry.resource_id());
  EXPECT_EQ(team_drive_resource->name(), entry.title());
  EXPECT_EQ(team_drive_resource->name(), entry.base_name());
  EXPECT_EQ(change_resource.modification_date().ToInternalValue(),
            entry.modification_date());
  EXPECT_TRUE(entry.file_info().is_directory());
  EXPECT_EQ(util::kDriveTeamDrivesDirLocalId, entry.parent_local_id());
  EXPECT_EQ("", parent_resource_id);
  EXPECT_FALSE(entry.deleted());

  EXPECT_TRUE(entry.capabilities_info().can_copy());
  EXPECT_FALSE(entry.capabilities_info().can_delete());
  EXPECT_TRUE(entry.capabilities_info().can_rename());
  EXPECT_FALSE(entry.capabilities_info().can_add_children());
  EXPECT_TRUE(entry.capabilities_info().can_share());
}

TEST(ResourceEntryConversionTest, ConvertTeamDriveResourceToResourceEntry) {
  google_apis::TeamDriveResource team_drive_resource;
  team_drive_resource.set_name("ABC Team Drive");
  team_drive_resource.set_id("team_drive_id");

  ResourceEntry entry;
  ConvertTeamDriveResourceToResourceEntry(team_drive_resource, &entry);

  EXPECT_EQ(team_drive_resource.id(), entry.resource_id());
  EXPECT_EQ(team_drive_resource.name(), entry.title());
  EXPECT_EQ(team_drive_resource.name(), entry.base_name());
  EXPECT_TRUE(entry.file_info().is_directory());
  EXPECT_EQ(util::kDriveTeamDrivesDirLocalId, entry.parent_local_id());
}

TEST(ResourceEntryConversionTest,
     ConvertTeamDriveRemovalChangeResourceToResourceEntry) {
  google_apis::ChangeResource change_resource;
  change_resource.set_type(google_apis::ChangeResource::TEAM_DRIVE);
  change_resource.set_team_drive_id("team_drive_id");
  change_resource.set_modification_date(GetTestTime());
  change_resource.set_deleted(true);
  // team_drive field is not filled for a deleted change resource.

  ResourceEntry entry;
  std::string parent_resource_id;
  EXPECT_TRUE(ConvertChangeResourceToResourceEntry(change_resource, &entry,
                                                   &parent_resource_id));

  EXPECT_EQ(change_resource.team_drive_id(), entry.resource_id());
  EXPECT_EQ(change_resource.modification_date().ToInternalValue(),
            entry.modification_date());
  EXPECT_TRUE(entry.file_info().is_directory());
  EXPECT_EQ(util::kDriveTeamDrivesDirLocalId, entry.parent_local_id());
  EXPECT_EQ("", parent_resource_id);
  EXPECT_TRUE(entry.deleted());
}

}  // namespace drive
