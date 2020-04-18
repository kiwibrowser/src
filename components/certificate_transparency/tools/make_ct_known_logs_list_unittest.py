#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import hashlib
import sys
import unittest
import make_ct_known_logs_list


def b64e(x):
    return base64.encodestring(x)


class FormattingTest(unittest.TestCase):
    def testSplitAndHexifyBinData(self):
        bin_data = ''.join([chr(i) for i in range(32,60)])
        expected_encoded_array = [
                ('"\\x20\\x21\\x22\\x23\\x24\\x25\\x26\\x27\\x28\\x29\\x2a'
                 '\\x2b\\x2c\\x2d\\x2e\\x2f\\x30"'),
                '"\\x31\\x32\\x33\\x34\\x35\\x36\\x37\\x38\\x39\\x3a\\x3b"']
        self.assertEqual(
                make_ct_known_logs_list._split_and_hexify_binary_data(
                        bin_data),
                expected_encoded_array)

        # This data should fit in exactly one line - 17 bytes.
        short_bin_data = ''.join([chr(i) for i in range(32, 49)])
        expected_short_array = [
                ('"\\x20\\x21\\x22\\x23\\x24\\x25\\x26\\x27\\x28\\x29\\x2a'
                 '\\x2b\\x2c\\x2d\\x2e\\x2f\\x30"')]
        self.assertEqual(
                make_ct_known_logs_list._split_and_hexify_binary_data(
                        short_bin_data),
                expected_short_array)

        # This data should fit exactly in two lines - 34 bytes.
        two_line_data = ''.join([chr(i) for i in range(32, 66)])
        expected_two_line_data_array = [
                ('"\\x20\\x21\\x22\\x23\\x24\\x25\\x26\\x27\\x28\\x29\\x2a'
                 '\\x2b\\x2c\\x2d\\x2e\\x2f\\x30"'),
                ('"\\x31\\x32\\x33\\x34\\x35\\x36\\x37\\x38\\x39\\x3a\\x3b'
                 '\\x3c\\x3d\\x3e\\x3f\\x40\\x41"')]
        self.assertEqual(
                make_ct_known_logs_list._split_and_hexify_binary_data(
                        short_bin_data),
                expected_short_array)

    def testGetLogIDsArray(self):
        log_ids = ["def", "abc", "ghi"]
        expected_log_ids_code = [
                "// The list is sorted.\n",
                "const char kTestIDs[][4] = {\n",
                '    "\\x61\\x62\\x63",\n',
                '    "\\x64\\x65\\x66",\n',
                '    "\\x67\\x68\\x69"',
                "};\n\n"]
        self.assertEqual(
                make_ct_known_logs_list._get_log_ids_array(
                        log_ids, "kTestIDs"),
                expected_log_ids_code)

    def testToLogInfoStruct(self):
        log = {"key": "YWJj",
               "description": "Test Description",
               "url": "ct.example.com",
               "dns_api_endpoint": "dns.ct.example.com"}
        expected_loginfo = (
                '    {"\\x61\\x62\\x63",\n     3,\n     "Test Description",\n'
                '     "dns.ct.example.com"}')
        self.assertEqual(
                make_ct_known_logs_list._to_loginfo_struct(log),
                expected_loginfo)


class OperatorIDHandlingTest(unittest.TestCase):
    def testFindingGoogleOperatorID(self):
        ops_list = {"operators": [
                {"id": 0, "name": "First"},
                {"id": 1, "name": "Second"}]}
        self.assertRaises(
                RuntimeError,
                make_ct_known_logs_list._find_google_operator_id,
                ops_list)
        ops_list["operators"].append({"id": 2, "name": "Google"})
        self.assertEqual(
                make_ct_known_logs_list._find_google_operator_id(ops_list),
                2)
        ops_list["operators"].append({"id": 3, "name": "Google"})
        self.assertRaises(
                RuntimeError,
                make_ct_known_logs_list._find_google_operator_id,
                ops_list)

    def testCollectingLogIDsByOperator(self):
        logs = [
                {"operated_by": (1,), "key": b64e('a')},
                {"operated_by": (2,), "key": b64e('b')},
                {"operated_by": (3,), "key": b64e('c')},
                {"operated_by": (1,), "key": b64e('d')}
        ]
        log_ids = make_ct_known_logs_list._get_log_ids_for_operator(logs, 1)
        self.assertEqual(2, len(log_ids))
        self.assertItemsEqual(
                [hashlib.sha256(t).digest() for t in ('a', 'd')],
                log_ids)


class DisqualifiedLogsHandlingTest(unittest.TestCase):
    def testCorrectlyIdentifiesDisqualifiedLog(self):
        self.assertTrue(
                make_ct_known_logs_list._is_log_disqualified(
                        {"disqualified_at" : 12345}))
        self.assertFalse(
                make_ct_known_logs_list._is_log_disqualified(
                        {"name" : "example"}))

    def testTranslatingToDisqualifiedLogDefinition(self):
        log = {"key": "YWJj",
               "description": "Test Description",
               "url": "ct.example.com",
               "dns_api_endpoint": "dns.ct.example.com",
               "disqualified_at": 1464566400}
        expected_disqualified_log_info = (
            '    {"\\xba\\x78\\x16\\xbf\\x8f\\x01\\xcf\\xea\\x41\\x41\\x40'
            '\\xde\\x5d\\xae\\x22\\x23\\xb0"\n     "\\x03\\x61\\xa3\\x96\\x17'
            '\\x7a\\x9c\\xb4\\x10\\xff\\x61\\xf2\\x00\\x15\\xad",\n    {"\\x61'
            '\\x62\\x63",\n     3,\n     "Test Description",\n     '
            '"dns.ct.example.com"},\n     '
            'base::TimeDelta::FromSeconds(1464566400)}')

        self.assertEqual(
            make_ct_known_logs_list._to_disqualified_loginfo_struct(log),
            expected_disqualified_log_info)

    def testSortingAndFilteringDisqualifiedLogs(self):
        logs = [
                {"disqualified_at": 1, "key": b64e('a')},
                {"key": b64e('b')},
                {"disqualified_at": 3, "key": b64e('c')},
                {"disqualified_at": 2, "key": b64e('d')},
                {"key": b64e('e')}
        ]
        disqualified_logs = make_ct_known_logs_list._sorted_disqualified_logs(
                logs)
        self.assertEqual(3, len(disqualified_logs))
        self.assertEqual(b64e('d'), disqualified_logs[0]["key"])
        self.assertEqual(b64e('c'), disqualified_logs[1]["key"])
        self.assertEqual(b64e('a'), disqualified_logs[2]["key"])


if __name__ == '__main__':
  unittest.main()
