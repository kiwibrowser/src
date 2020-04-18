// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/xcode_object.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Instantiate a PBXSourcesBuildPhase object.
std::unique_ptr<PBXSourcesBuildPhase> GetPBXSourcesBuildPhaseObject() {
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase(
      new PBXSourcesBuildPhase());
  return pbx_sources_build_phase;
}

// Instantiate a PBXFrameworksBuildPhase object.
std::unique_ptr<PBXFrameworksBuildPhase> GetPBXFrameworksBuildPhaseObject() {
  std::unique_ptr<PBXFrameworksBuildPhase> pbx_frameworks_build_phase(
      new PBXFrameworksBuildPhase());
  return pbx_frameworks_build_phase;
}

// Instantiate a PBXShellScriptBuildPhase object with arbitrary names.
std::unique_ptr<PBXShellScriptBuildPhase> GetPBXShellScriptBuildPhaseObject() {
  std::unique_ptr<PBXShellScriptBuildPhase> pbx_shell_script_build_phase(
      new PBXShellScriptBuildPhase("name", "shell_script"));
  return pbx_shell_script_build_phase;
}

// Instantiate a PBXGroup object with arbitrary names.
std::unique_ptr<PBXGroup> GetPBXGroupObject() {
  std::unique_ptr<PBXGroup> pbx_group(new PBXGroup("/dir1/dir2", "group"));
  return pbx_group;
}

// Instantiate a PBXProject object with arbitrary names.
std::unique_ptr<PBXProject> GetPBXProjectObject() {
  std::unique_ptr<PBXProject> pbx_project(
      new PBXProject("project", "config", "out/build", PBXAttributes()));
  return pbx_project;
}

// Instantiate a PBXFileReference object with arbitrary names.
std::unique_ptr<PBXFileReference> GetPBXFileReferenceObject() {
  std::unique_ptr<PBXFileReference> pbx_file_reference(new PBXFileReference(
      "product.app", "product.app", "wrapper.application"));
  return pbx_file_reference;
}

// Instantiate a PBXBuildFile object.
std::unique_ptr<PBXBuildFile> GetPBXBuildFileObject(
    const PBXFileReference* file_reference,
    const PBXSourcesBuildPhase* build_phase) {
  std::unique_ptr<PBXBuildFile> pbx_build_file(
      new PBXBuildFile(file_reference, build_phase, CompilerFlags::NONE));
  return pbx_build_file;
}

// Instantiate a PBXAggregateTarget object with arbitrary names.
std::unique_ptr<PBXAggregateTarget> GetPBXAggregateTargetObject() {
  std::unique_ptr<PBXAggregateTarget> pbx_aggregate_target(
      new PBXAggregateTarget("target_name", "shell_script", "config_name",
                             PBXAttributes()));
  return pbx_aggregate_target;
}

// Instantiate a PBXNativeTarget object with arbitrary names.
std::unique_ptr<PBXNativeTarget> GetPBXNativeTargetObject(
    const PBXFileReference* product_reference) {
  std::unique_ptr<PBXNativeTarget> pbx_native_target(new PBXNativeTarget(
      "target_name", "ninja gn_unittests", "config_name", PBXAttributes(),
      "com.apple.product-type.application", "product_name", product_reference));
  return pbx_native_target;
}

// Instantiate a PBXContainerItemProxy object.
std::unique_ptr<PBXContainerItemProxy> GetPBXContainerItemProxyObject(
    const PBXProject* project,
    const PBXTarget* target) {
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy(
      new PBXContainerItemProxy(project, target));
  return pbx_container_item_proxy;
}

// Instantiate a PBXTargetDependency object.
std::unique_ptr<PBXTargetDependency> GetPBXTargetDependencyObject(
    const PBXTarget* target,
    std::unique_ptr<PBXContainerItemProxy> container_item_proxy) {
  std::unique_ptr<PBXTargetDependency> pbx_target_dependency(
      new PBXTargetDependency(target, std::move(container_item_proxy)));
  return pbx_target_dependency;
}

// Instantiate a XCBuildConfiguration object with arbitrary names.
std::unique_ptr<XCBuildConfiguration> GetXCBuildConfigurationObject() {
  std::unique_ptr<XCBuildConfiguration> xc_build_configuration(
      new XCBuildConfiguration("config_name", PBXAttributes()));
  return xc_build_configuration;
}

// Instantiate a XCConfigurationList object with arbitrary names.
std::unique_ptr<XCConfigurationList> GetXCConfigurationListObject(
    const PBXObject* owner_reference) {
  std::unique_ptr<XCConfigurationList> xc_configuration_list(
      new XCConfigurationList("config_list_name", PBXAttributes(),
                              owner_reference));
  return xc_configuration_list;
}

}  // namespace

// Tests that instantiating Xcode objects doesn't crash.
TEST(XcodeObject, InstantiatePBXSourcesBuildPhase) {
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase =
      GetPBXSourcesBuildPhaseObject();
}

TEST(XcodeObject, InstantiatePBXFrameworksBuildPhase) {
  std::unique_ptr<PBXFrameworksBuildPhase> pbx_frameworks_build_phase =
      GetPBXFrameworksBuildPhaseObject();
}

TEST(XcodeObject, InstantiatePBXShellScriptBuildPhase) {
  std::unique_ptr<PBXShellScriptBuildPhase> pbx_shell_script_build_phase =
      GetPBXShellScriptBuildPhaseObject();
}

TEST(XcodeObject, InstantiatePBXGroup) {
  std::unique_ptr<PBXGroup> pbx_group = GetPBXGroupObject();
}

TEST(XcodeObject, InstantiatePBXProject) {
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
}

TEST(XcodeObject, InstantiatePBXFileReference) {
  std::unique_ptr<PBXFileReference> pbx_file_reference =
      GetPBXFileReferenceObject();
}

TEST(XcodeObject, InstantiatePBXBuildFile) {
  std::unique_ptr<PBXFileReference> pbx_file_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase =
      GetPBXSourcesBuildPhaseObject();
  std::unique_ptr<PBXBuildFile> pbx_build_file = GetPBXBuildFileObject(
      pbx_file_reference.get(), pbx_sources_build_phase.get());
}

TEST(XcodeObject, InstantiatePBXAggregateTarget) {
  std::unique_ptr<PBXAggregateTarget> pbx_aggregate_target =
      GetPBXAggregateTargetObject();
}

TEST(XcodeObject, InstantiatePBXNativeTarget) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
}

TEST(XcodeObject, InstantiatePBXContainerItemProxy) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy =
      GetPBXContainerItemProxyObject(pbx_project.get(),
                                     pbx_native_target.get());
}

TEST(XcodeObject, InstantiatePBXTargetDependency) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy =
      GetPBXContainerItemProxyObject(pbx_project.get(),
                                     pbx_native_target.get());
  std::unique_ptr<PBXTargetDependency> pbx_target_dependency =
      GetPBXTargetDependencyObject(pbx_native_target.get(),
                                   std::move(pbx_container_item_proxy));
}

TEST(XcodeObject, InstantiateXCBuildConfiguration) {
  std::unique_ptr<XCBuildConfiguration> xc_build_configuration =
      GetXCBuildConfigurationObject();
}

TEST(XcodeObject, InstantiateXCConfigurationList) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<XCConfigurationList> xc_configuration_list =
      GetXCConfigurationListObject(pbx_native_target.get());
}

// Tests that the mapping between PBXObject and PBXObjectClass.
TEST(XcodeObject, PBXSourcesBuildPhaseObjectToClass) {
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase =
      GetPBXSourcesBuildPhaseObject();
  EXPECT_EQ(PBXSourcesBuildPhaseClass, pbx_sources_build_phase->Class());
}

TEST(XcodeObject, PBXFrameworksBuildPhaseObjectToClass) {
  std::unique_ptr<PBXFrameworksBuildPhase> pbx_frameworks_build_phase =
      GetPBXFrameworksBuildPhaseObject();
  EXPECT_EQ(PBXFrameworksBuildPhaseClass, pbx_frameworks_build_phase->Class());
}

TEST(XcodeObject, PBXShellScriptBuildPhaseObjectToClass) {
  std::unique_ptr<PBXShellScriptBuildPhase> pbx_shell_script_build_phase =
      GetPBXShellScriptBuildPhaseObject();
  EXPECT_EQ(PBXShellScriptBuildPhaseClass,
            pbx_shell_script_build_phase->Class());
}

TEST(XcodeObject, PBXGroupObjectToClass) {
  std::unique_ptr<PBXGroup> pbx_group = GetPBXGroupObject();
  EXPECT_EQ(PBXGroupClass, pbx_group->Class());
}

TEST(XcodeObject, PBXProjectObjectToClass) {
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  EXPECT_EQ(PBXProjectClass, pbx_project->Class());
}

TEST(XcodeObject, PBXFileReferenceObjectToClass) {
  std::unique_ptr<PBXFileReference> pbx_file_reference =
      GetPBXFileReferenceObject();
  EXPECT_EQ(PBXFileReferenceClass, pbx_file_reference->Class());
}

TEST(XcodeObject, PBXBuildFileObjectToClass) {
  std::unique_ptr<PBXFileReference> pbx_file_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase =
      GetPBXSourcesBuildPhaseObject();
  std::unique_ptr<PBXBuildFile> pbx_build_file = GetPBXBuildFileObject(
      pbx_file_reference.get(), pbx_sources_build_phase.get());
  EXPECT_EQ(PBXBuildFileClass, pbx_build_file->Class());
}

TEST(XcodeObject, PBXAggregateTargetObjectToClass) {
  std::unique_ptr<PBXAggregateTarget> pbx_aggregate_target =
      GetPBXAggregateTargetObject();
  EXPECT_EQ(PBXAggregateTargetClass, pbx_aggregate_target->Class());
}

TEST(XcodeObject, PBXNativeTargetObjectToClass) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  EXPECT_EQ(PBXNativeTargetClass, pbx_native_target->Class());
}

TEST(XcodeObject, PBXContainerItemProxyObjectToClass) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy =
      GetPBXContainerItemProxyObject(pbx_project.get(),
                                     pbx_native_target.get());
  EXPECT_EQ(PBXContainerItemProxyClass, pbx_container_item_proxy->Class());
}

TEST(XcodeObject, PBXTargetDependencyObjectToClass) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy =
      GetPBXContainerItemProxyObject(pbx_project.get(),
                                     pbx_native_target.get());
  std::unique_ptr<PBXTargetDependency> pbx_target_dependency =
      GetPBXTargetDependencyObject(pbx_native_target.get(),
                                   std::move(pbx_container_item_proxy));
  EXPECT_EQ(PBXTargetDependencyClass, pbx_target_dependency->Class());
}

TEST(XcodeObject, XCBuildConfigurationObjectToClass) {
  std::unique_ptr<XCBuildConfiguration> xc_build_configuration =
      GetXCBuildConfigurationObject();
  EXPECT_EQ(XCBuildConfigurationClass, xc_build_configuration->Class());
}

TEST(XcodeObject, XCConfigurationListObjectToClass) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<XCConfigurationList> xc_configuration_list =
      GetXCConfigurationListObject(pbx_native_target.get());
  EXPECT_EQ(XCConfigurationListClass, xc_configuration_list->Class());
}

// Tests the mapping between PBXObjectClass and it's name as a string.
TEST(XcodeObject, ClassToString) {
  EXPECT_STREQ("PBXAggregateTarget", ToString(PBXAggregateTargetClass));
  EXPECT_STREQ("PBXBuildFile", ToString(PBXBuildFileClass));
  EXPECT_STREQ("PBXAggregateTarget", ToString(PBXAggregateTargetClass));
  EXPECT_STREQ("PBXBuildFile", ToString(PBXBuildFileClass));
  EXPECT_STREQ("PBXContainerItemProxy", ToString(PBXContainerItemProxyClass));
  EXPECT_STREQ("PBXFileReference", ToString(PBXFileReferenceClass));
  EXPECT_STREQ("PBXFrameworksBuildPhase",
               ToString(PBXFrameworksBuildPhaseClass));
  EXPECT_STREQ("PBXGroup", ToString(PBXGroupClass));
  EXPECT_STREQ("PBXNativeTarget", ToString(PBXNativeTargetClass));
  EXPECT_STREQ("PBXProject", ToString(PBXProjectClass));
  EXPECT_STREQ("PBXSourcesBuildPhase", ToString(PBXSourcesBuildPhaseClass));
  EXPECT_STREQ("PBXTargetDependency", ToString(PBXTargetDependencyClass));
  EXPECT_STREQ("XCBuildConfiguration", ToString(XCBuildConfigurationClass));
  EXPECT_STREQ("XCConfigurationList", ToString(XCConfigurationListClass));
  EXPECT_STREQ("PBXShellScriptBuildPhase",
               ToString(PBXShellScriptBuildPhaseClass));
}

// Tests the mapping between PBXObject and it's name as a string.
TEST(XcodeObject, PBXSourcesBuildPhaseName) {
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase =
      GetPBXSourcesBuildPhaseObject();
  EXPECT_EQ("Sources", pbx_sources_build_phase->Name());
}

TEST(XcodeObject, PBXFrameworksBuildPhaseName) {
  std::unique_ptr<PBXFrameworksBuildPhase> pbx_frameworks_build_phase =
      GetPBXFrameworksBuildPhaseObject();
  EXPECT_EQ("Frameworks", pbx_frameworks_build_phase->Name());
}

TEST(XcodeObject, PBXShellScriptBuildPhaseName) {
  std::unique_ptr<PBXShellScriptBuildPhase> pbx_shell_script_build_phase =
      GetPBXShellScriptBuildPhaseObject();
  EXPECT_EQ("Action \"Compile and copy name via ninja\"",
            pbx_shell_script_build_phase->Name());
}

TEST(XcodeObject, PBXGroupName) {
  PBXGroup pbx_group_with_name(std::string(), "name");
  EXPECT_EQ("name", pbx_group_with_name.Name());

  PBXGroup pbx_group_with_path("path", std::string());
  EXPECT_EQ("path", pbx_group_with_path.Name());

  PBXGroup pbx_group_empty{std::string(), std::string()};
  EXPECT_EQ(std::string(), pbx_group_empty.Name());
}

TEST(XcodeObject, PBXProjectName) {
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  EXPECT_EQ("project", pbx_project->Name());
}

TEST(XcodeObject, PBXFileReferenceName) {
  std::unique_ptr<PBXFileReference> pbx_file_reference =
      GetPBXFileReferenceObject();
  EXPECT_EQ("product.app", pbx_file_reference->Name());
}

TEST(XcodeObject, PBXBuildFileName) {
  std::unique_ptr<PBXFileReference> pbx_file_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXSourcesBuildPhase> pbx_sources_build_phase =
      GetPBXSourcesBuildPhaseObject();
  std::unique_ptr<PBXBuildFile> pbx_build_file = GetPBXBuildFileObject(
      pbx_file_reference.get(), pbx_sources_build_phase.get());
  EXPECT_EQ("product.app in Sources", pbx_build_file->Name());
}

TEST(XcodeObject, PBXAggregateTargetName) {
  std::unique_ptr<PBXAggregateTarget> pbx_aggregate_target =
      GetPBXAggregateTargetObject();
  EXPECT_EQ("target_name", pbx_aggregate_target->Name());
}

TEST(XcodeObject, PBXNativeTargetName) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  EXPECT_EQ("target_name", pbx_native_target->Name());
}

TEST(XcodeObject, PBXContainerItemProxyName) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy =
      GetPBXContainerItemProxyObject(pbx_project.get(),
                                     pbx_native_target.get());
  EXPECT_EQ("PBXContainerItemProxy", pbx_container_item_proxy->Name());
}

TEST(XcodeObject, PBXTargetDependencyName) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXProject> pbx_project = GetPBXProjectObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<PBXContainerItemProxy> pbx_container_item_proxy =
      GetPBXContainerItemProxyObject(pbx_project.get(),
                                     pbx_native_target.get());
  std::unique_ptr<PBXTargetDependency> pbx_target_dependency =
      GetPBXTargetDependencyObject(pbx_native_target.get(),
                                   std::move(pbx_container_item_proxy));
  EXPECT_EQ("PBXTargetDependency", pbx_target_dependency->Name());
}

TEST(XcodeObject, XCBuildConfigurationName) {
  std::unique_ptr<XCBuildConfiguration> xc_build_configuration =
      GetXCBuildConfigurationObject();
  EXPECT_EQ("config_name", xc_build_configuration->Name());
}

TEST(XcodeObject, XCConfigurationListName) {
  std::unique_ptr<PBXFileReference> product_reference =
      GetPBXFileReferenceObject();
  std::unique_ptr<PBXNativeTarget> pbx_native_target =
      GetPBXNativeTargetObject(product_reference.get());
  std::unique_ptr<XCConfigurationList> xc_configuration_list =
      GetXCConfigurationListObject(pbx_native_target.get());
  EXPECT_EQ("Build configuration list for PBXNativeTarget \"target_name\"",
            xc_configuration_list->Name());
}
