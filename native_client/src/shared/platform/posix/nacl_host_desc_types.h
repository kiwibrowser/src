/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_HOST_DESC_TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_HOST_DESC_TYPES_H_

struct NaClHostDesc {
  int d;
  int flags;

  /*
   * friend int NaClDescIoDescExternalizeSize(...);
   * friend int NaClDescIoDescExternalize(...);
   * friend int NaClDescIoInternalize(...);
   *
   * Translation: NaClDescIoDesc's externalization/internalization
   * interface functions are friend functions that look inside the
   * NaClHostDesc implementation.  Do not make changes here without
   * also looking there.
   */
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_HOST_DESC_TYPES_H_ */
