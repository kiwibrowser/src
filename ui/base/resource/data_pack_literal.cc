// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "ui/base/resource/data_pack_literal.h"

namespace ui {

const char kSamplePakContentsV4[] = {
    0x04, 0x00, 0x00, 0x00,              // header(version
    0x04, 0x00, 0x00, 0x00,              //        no. entries
    0x01,                                //        encoding)
    0x01, 0x00, 0x27, 0x00, 0x00, 0x00,  // index entry 1
    0x04, 0x00, 0x27, 0x00, 0x00, 0x00,  // index entry 4
    0x06, 0x00, 0x33, 0x00, 0x00, 0x00,  // index entry 6
    0x0a, 0x00, 0x3f, 0x00, 0x00, 0x00,  // index entry 10
    0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,  // extra entry for the size of last
    't',  'h',  'i',  's',  ' ',  'i',  's', ' ', 'i', 'd', ' ', '4',
    't',  'h',  'i',  's',  ' ',  'i',  's', ' ', 'i', 'd', ' ', '6'};

const size_t kSamplePakSizeV4 = sizeof(kSamplePakContentsV4);

const char kSamplePakContentsV5[] = {
    0x05, 0x00, 0x00, 0x00,              // version
    0x01, 0x00, 0x00, 0x00,              // encoding + padding
    0x03, 0x00, 0x01, 0x00,              // num_resources, num_aliases
    0x01, 0x00, 0x28, 0x00, 0x00, 0x00,  // index entry 1
    0x04, 0x00, 0x28, 0x00, 0x00, 0x00,  // index entry 4
    0x06, 0x00, 0x34, 0x00, 0x00, 0x00,  // index entry 6
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00,  // extra entry for the size of last
    0x0a, 0x00, 0x01, 0x00,              // alias table
    't',  'h',  'i',  's',  ' ',  'i',  's', ' ', 'i', 'd', ' ', '4',
    't',  'h',  'i',  's',  ' ',  'i',  's', ' ', 'i', 'd', ' ', '6'};

const size_t kSamplePakSizeV5 = sizeof(kSamplePakContentsV5);

const char kSampleCorruptPakContents[] = {
    0x04, 0x00, 0x00, 0x00,              // header(version
    0x04, 0x00, 0x00, 0x00,              //        no. entries
    0x01,                                //        encoding)
    0x01, 0x00, 0x27, 0x00, 0x00, 0x00,  // index entry 1
    0x04, 0x00, 0x27, 0x00, 0x00, 0x00,  // index entry 4
    0x06, 0x00, 0x33, 0x00, 0x00, 0x00,  // index entry 6
    0x0a, 0x00, 0x3f, 0x00, 0x00, 0x00,  // index entry 10
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00,  // extra entry for the size of last,
                                         // extends past END OF FILE.
    't', 'h', 'i', 's', ' ', 'i', 's', ' ', 'i', 'd', ' ', '4', 't', 'h', 'i',
    's', ' ', 'i', 's', ' ', 'i', 'd', ' ', '6'};

const size_t kSampleCorruptPakSize = sizeof(kSampleCorruptPakContents);

const char kSamplePakContents2x[] = {
    0x04, 0x00, 0x00, 0x00,              // header(version
    0x01, 0x00, 0x00, 0x00,              //        no. entries
    0x01,                                //        encoding)
    0x04, 0x00, 0x15, 0x00, 0x00, 0x00,  // index entry 4
    0x00, 0x00, 0x24, 0x00, 0x00, 0x00,  // extra entry for the size of last
    't',  'h',  'i',  's',  ' ',  'i',  's', ' ',
    'i',  'd',  ' ',  '4',  ' ',  '2',  'x'};

const size_t kSamplePakSize2x = sizeof(kSamplePakContents2x);

const char kEmptyPakContents[] = {
    0x04, 0x00, 0x00, 0x00,             // header(version
    0x00, 0x00, 0x00, 0x00,             //        no. entries
    0x01,                               //        encoding)
    0x00, 0x00, 0x0f, 0x00, 0x00, 0x00  // extra entry for the size of last
};

const size_t kEmptyPakSize = sizeof(kEmptyPakContents);

}  // namespace ui
