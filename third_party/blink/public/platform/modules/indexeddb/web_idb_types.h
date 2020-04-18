/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_TYPES_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_TYPES_H_

namespace blink {

enum WebIDBKeyType {
  kWebIDBKeyTypeInvalid = 0,
  kWebIDBKeyTypeArray,
  kWebIDBKeyTypeBinary,
  kWebIDBKeyTypeString,
  kWebIDBKeyTypeDate,
  kWebIDBKeyTypeNumber,
  kWebIDBKeyTypeNull,
  kWebIDBKeyTypeMin,
};

enum WebIDBKeyPathType {
  kWebIDBKeyPathTypeNull = 0,
  kWebIDBKeyPathTypeString,
  kWebIDBKeyPathTypeArray,
};

enum WebIDBDataLoss {
  kWebIDBDataLossNone = 0,
  kWebIDBDataLossTotal,
};

enum WebIDBCursorDirection {
  kWebIDBCursorDirectionNext = 0,
  kWebIDBCursorDirectionNextNoDuplicate = 1,
  kWebIDBCursorDirectionPrev = 2,
  kWebIDBCursorDirectionPrevNoDuplicate = 3,
};

enum WebIDBTaskType {
  kWebIDBTaskTypeNormal = 0,
  kWebIDBTaskTypePreemptive,
};

enum WebIDBPutMode {
  kWebIDBPutModeAddOrUpdate,
  kWebIDBPutModeAddOnly,
  kWebIDBPutModeCursorUpdate,
};

enum WebIDBOperationType {
  kWebIDBAdd = 0,
  kWebIDBPut,
  kWebIDBDelete,
  kWebIDBClear,
  kWebIDBOperationTypeCount,
};

enum WebIDBTransactionMode {
  kWebIDBTransactionModeReadOnly = 0,
  kWebIDBTransactionModeReadWrite,
  kWebIDBTransactionModeVersionChange,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_TYPES_H_
