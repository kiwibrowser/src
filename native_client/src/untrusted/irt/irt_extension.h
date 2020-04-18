/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_EXTENSION_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_EXTENSION_H_

/*
 * IRT Extension describes an additional set of interfaces that can optionally
 * be supplied which standard libraries (such as the C Standard Library) may
 * use. These interfaces would be used along side the standard set of interfaces
 * inside of irt.h and irt_dev.h. Some extra interfaces will be declared in this
 * header (irt_extension.h) as well which standard libraries may accept and use.
 *
 * The IRT will not actually itself supply any function definitions for any of
 * these interfaces, but the header will serve as a standard list of declared
 * interfaces that standard libraries expect to use.
 *
 * A standard library that wishes to utilize interfaces supplied should
 * implement nacl_interface_ext_supply() and is free to decide what it wants
 * to do with supplied interfaces.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This function is used to supply extra interfaces to a standard library.
 * An interface user such as a Standard libraries will implement this function.
 *
 * This function returns the number of bytes of TABLE used by the interface
 * user. The number of bytes used will never be larger than TABLESIZE, and
 * will return 0 if the interface was not used at all.
 */
size_t nacl_interface_ext_supply(const char *interface_ident,
                                 const void *table, size_t tablesize);

#if defined(__cplusplus)
}
#endif

#endif /* NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_EXTENSION_H */
