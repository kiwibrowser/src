// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/protocol_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace update_client {

const char* kValidXml =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "       <url codebasediff='http://diff.example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

const char* valid_xml_with_hash =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx' hash_sha256='1234'"
    "             hashdiff_sha256='5678'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

const char* valid_xml_with_invalid_sizes =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='1' size='1234'/>"
    "         <package name='2' size='-1234'/>"
    "         <package name='3' />"
    "         <package name='4' size='-a'/>"
    "         <package name='5' size='-123467890123456789'/>"
    "         <package name='6' size='123467890123456789'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

const char* kInvalidValidXmlMissingCodebase =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebasediff='http://diff.example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package namediff='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

const char* kInvalidValidXmlMissingManifest =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "   </updatecheck>"
    " </app>"
    "</response>";

const char* kMissingAppId =
    "<?xml version='1.0'?>"
    "<response protocol='3.1'>"
    " <app>"
    "  <updatecheck codebase='http://example.com/extension_1.2.3.4.crx'"
    "               version='1.2.3.4'/>"
    " </app>"
    "</response>";

const char* kInvalidCodebase =
    "<?xml version='1.0'?>"
    "<response protocol='3.1'>"
    " <app appid='12345' status='ok'>"
    "  <updatecheck codebase='example.com/extension_1.2.3.4.crx'"
    "               version='1.2.3.4'/>"
    " </app>"
    "</response>";

const char* kMissingVersion =
    "<?xml version='1.0'?>"
    "<response protocol='3.1'>"
    " <app appid='12345' status='ok'>"
    "  <updatecheck codebase='http://example.com/extension_1.2.3.4.crx'/>"
    " </app>"
    "</response>";

const char* kInvalidVersion =
    "<?xml version='1.0'?>"
    "<response protocol='3.1'>"
    " <app appid='12345' status='ok'>"
    "  <updatecheck codebase='http://example.com/extension_1.2.3.4.crx' "
    "               version='1.2.3.a'/>"
    " </app>"
    "</response>";

// The v3 version of the protocol is not using namespaces. However, the parser
// must be able to parse responses that include namespaces.
const char* kUsesNamespacePrefix =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<g:response xmlns:g='http://www.google.com/update2/response' "
    "protocol='3.1'>"
    " <g:app appid='12345'>"
    "   <g:updatecheck status='ok'>"
    "     <g:urls>"
    "       <g:url codebase='http://example.com/'/>"
    "     </g:urls>"
    "     <g:manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <g:packages>"
    "         <g:package name='extension_1_2_3_4.crx'/>"
    "       </g:packages>"
    "     </g:manifest>"
    "   </g:updatecheck>"
    " </g:app>"
    "</g:response>";

// Includes unrelated <app> tags from other xml namespaces - this should
// not cause problems.
const char* kSimilarTagnames =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response xmlns:a='http://a' protocol='3.1'>"
    " <a:app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </a:app>"
    " <b:app appid='xyz' xmlns:b='http://b'>"
    "   <updatecheck status='noupdate'/>"
    " </b:app>"
    "</response>";

// Includes a <daystart> tag.
const char* kWithDaystart =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <daystart elapsed_seconds='456'/>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

// Indicates no updates available - this should not be a parse error.
const char* kNoUpdate =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "  <updatecheck status='noupdate'/>"
    " </app>"
    "</response>";

// Includes two <app> tags, one with an error.
const char* kTwoAppsOneError =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='aaaaaaaa' status='error-unknownApplication'>"
    "  <updatecheck status='error-internal'/>"
    " </app>"
    " <app appid='bbbbbbbb'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

// Includes two <app> tags, both of which set the cohort.
const char* kTwoAppsSetCohort =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='aaaaaaaa' cohort='1:2q3/'>"
    "  <updatecheck status='noupdate'/>"
    " </app>"
    " <app appid='bbbbbbbb' cohort='1:33z@0.33' cohortname='cname'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "   </updatecheck>"
    " </app>"
    "</response>";

// Includes a run action for an update check with status='ok'.
const char* kUpdateCheckStatusOkWithRunAction =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='ok'>"
    "     <urls>"
    "       <url codebase='http://example.com/'/>"
    "       <url codebasediff='http://diff.example.com/'/>"
    "     </urls>"
    "     <manifest version='1.2.3.4' prodversionmin='2.0.143.0'>"
    "       <packages>"
    "         <package name='extension_1_2_3_4.crx'/>"
    "       </packages>"
    "     </manifest>"
    "     <actions>"
    "       <action run='this'/>"
    "     </actions>"
    "   </updatecheck>"
    " </app>"
    "</response>";

// Includes a run action for an update check with status='noupdate'.
const char* kUpdateCheckStatusNoUpdateWithRunAction =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345'>"
    "   <updatecheck status='noupdate'>"
    "     <actions>"
    "       <action run='this'/>"
    "     </actions>"
    "   </updatecheck>"
    " </app>"
    "</response>";

// Includes a run action for an update check with status='error'.
const char* kUpdateCheckStatusErrorWithRunAction =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<response protocol='3.1'>"
    " <app appid='12345' status='ok'>"
    "  <updatecheck status='error-osnotsupported'>"
    "     <actions>"
    "       <action run='this'/>"
    "     </actions>"
    "   </updatecheck>"
    " </app>"
    "</response>";

TEST(ComponentUpdaterProtocolParserTest, Parse) {
  ProtocolParser parser;

  // Test parsing of a number of invalid xml cases
  EXPECT_FALSE(parser.Parse(std::string()));
  EXPECT_FALSE(parser.errors().empty());

  EXPECT_TRUE(parser.Parse(kMissingAppId));
  EXPECT_TRUE(parser.results().list.empty());
  EXPECT_FALSE(parser.errors().empty());

  EXPECT_TRUE(parser.Parse(kInvalidCodebase));
  EXPECT_TRUE(parser.results().list.empty());
  EXPECT_FALSE(parser.errors().empty());

  EXPECT_TRUE(parser.Parse(kMissingVersion));
  EXPECT_TRUE(parser.results().list.empty());
  EXPECT_FALSE(parser.errors().empty());

  EXPECT_TRUE(parser.Parse(kInvalidVersion));
  EXPECT_TRUE(parser.results().list.empty());
  EXPECT_FALSE(parser.errors().empty());

  EXPECT_TRUE(parser.Parse(kInvalidValidXmlMissingCodebase));
  EXPECT_TRUE(parser.results().list.empty());
  EXPECT_FALSE(parser.errors().empty());

  EXPECT_TRUE(parser.Parse(kInvalidValidXmlMissingManifest));
  EXPECT_TRUE(parser.results().list.empty());
  EXPECT_FALSE(parser.errors().empty());

  // Parse some valid XML, and check that all params came out as expected
  EXPECT_TRUE(parser.Parse(kValidXml));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_EQ(1u, parser.results().list.size());
  const ProtocolParser::Result* firstResult = &parser.results().list[0];
  EXPECT_STREQ("ok", firstResult->status.c_str());
  EXPECT_EQ(1u, firstResult->crx_urls.size());
  EXPECT_EQ(GURL("http://example.com/"), firstResult->crx_urls[0]);
  EXPECT_EQ(GURL("http://diff.example.com/"), firstResult->crx_diffurls[0]);
  EXPECT_EQ("1.2.3.4", firstResult->manifest.version);
  EXPECT_EQ("2.0.143.0", firstResult->manifest.browser_min_version);
  EXPECT_EQ(1u, firstResult->manifest.packages.size());
  EXPECT_EQ("extension_1_2_3_4.crx", firstResult->manifest.packages[0].name);

  // Parse some xml that uses namespace prefixes.
  EXPECT_TRUE(parser.Parse(kUsesNamespacePrefix));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_TRUE(parser.Parse(kSimilarTagnames));
  EXPECT_TRUE(parser.errors().empty());

  // Parse xml with hash value
  EXPECT_TRUE(parser.Parse(valid_xml_with_hash));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_FALSE(parser.results().list.empty());
  firstResult = &parser.results().list[0];
  EXPECT_FALSE(firstResult->manifest.packages.empty());
  EXPECT_EQ("1234", firstResult->manifest.packages[0].hash_sha256);
  EXPECT_EQ("5678", firstResult->manifest.packages[0].hashdiff_sha256);

  // Parse xml with package size value
  EXPECT_TRUE(parser.Parse(valid_xml_with_invalid_sizes));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_FALSE(parser.results().list.empty());
  firstResult = &parser.results().list[0];
  EXPECT_FALSE(firstResult->manifest.packages.empty());
  EXPECT_EQ(1234, firstResult->manifest.packages[0].size);
  EXPECT_EQ(-1234, firstResult->manifest.packages[1].size);
  EXPECT_EQ(0, firstResult->manifest.packages[2].size);
  EXPECT_EQ(0, firstResult->manifest.packages[3].size);
  EXPECT_EQ(0, firstResult->manifest.packages[4].size);
  EXPECT_EQ(0, firstResult->manifest.packages[5].size);

  // Parse xml with a <daystart> element.
  EXPECT_TRUE(parser.Parse(kWithDaystart));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_FALSE(parser.results().list.empty());
  EXPECT_EQ(parser.results().daystart_elapsed_seconds, 456);

  // Parse a no-update response.
  EXPECT_TRUE(parser.Parse(kNoUpdate));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_FALSE(parser.results().list.empty());
  firstResult = &parser.results().list[0];
  EXPECT_STREQ("noupdate", firstResult->status.c_str());
  EXPECT_EQ(firstResult->extension_id, "12345");
  EXPECT_EQ(firstResult->manifest.version, "");

  // Parse xml with one error and one success <app> tag.
  EXPECT_TRUE(parser.Parse(kTwoAppsOneError));
  EXPECT_FALSE(parser.errors().empty());
  EXPECT_EQ(1u, parser.results().list.size());
  firstResult = &parser.results().list[0];
  EXPECT_EQ(firstResult->extension_id, "bbbbbbbb");
  EXPECT_STREQ("ok", firstResult->status.c_str());
  EXPECT_EQ("1.2.3.4", firstResult->manifest.version);

  // Parse xml with two apps setting the cohort info.
  EXPECT_TRUE(parser.Parse(kTwoAppsSetCohort));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_EQ(2u, parser.results().list.size());
  firstResult = &parser.results().list[0];
  EXPECT_EQ(firstResult->extension_id, "aaaaaaaa");
  EXPECT_NE(firstResult->cohort_attrs.find("cohort"),
            firstResult->cohort_attrs.end());
  EXPECT_EQ(firstResult->cohort_attrs.find("cohort")->second, "1:2q3/");
  EXPECT_EQ(firstResult->cohort_attrs.find("cohortname"),
            firstResult->cohort_attrs.end());
  EXPECT_EQ(firstResult->cohort_attrs.find("cohorthint"),
            firstResult->cohort_attrs.end());
  const ProtocolParser::Result* secondResult = &parser.results().list[1];
  EXPECT_EQ(secondResult->extension_id, "bbbbbbbb");
  EXPECT_NE(secondResult->cohort_attrs.find("cohort"),
            secondResult->cohort_attrs.end());
  EXPECT_EQ(secondResult->cohort_attrs.find("cohort")->second, "1:33z@0.33");
  EXPECT_NE(secondResult->cohort_attrs.find("cohortname"),
            secondResult->cohort_attrs.end());
  EXPECT_EQ(secondResult->cohort_attrs.find("cohortname")->second, "cname");
  EXPECT_EQ(secondResult->cohort_attrs.find("cohorthint"),
            secondResult->cohort_attrs.end());

  EXPECT_TRUE(parser.Parse(kUpdateCheckStatusOkWithRunAction));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_FALSE(parser.results().list.empty());
  firstResult = &parser.results().list[0];
  EXPECT_STREQ("ok", firstResult->status.c_str());
  EXPECT_EQ(firstResult->extension_id, "12345");
  EXPECT_STREQ("this", firstResult->action_run.c_str());

  EXPECT_TRUE(parser.Parse(kUpdateCheckStatusNoUpdateWithRunAction));
  EXPECT_TRUE(parser.errors().empty());
  EXPECT_FALSE(parser.results().list.empty());
  firstResult = &parser.results().list[0];
  EXPECT_STREQ("noupdate", firstResult->status.c_str());
  EXPECT_EQ(firstResult->extension_id, "12345");
  EXPECT_STREQ("this", firstResult->action_run.c_str());

  EXPECT_TRUE(parser.Parse(kUpdateCheckStatusErrorWithRunAction));
  EXPECT_FALSE(parser.errors().empty());
  EXPECT_TRUE(parser.results().list.empty());
}

}  // namespace update_client
