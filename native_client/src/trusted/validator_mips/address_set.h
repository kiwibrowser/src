/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 * Copyright 2012, Google Inc.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_ADDRESS_SET_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_ADDRESS_SET_H

#include <stdint.h>

namespace nacl_mips_val {

/*
 * A set of word addresses, implemented as a dense bitset.
 *
 * An AddressSet has a base address and a size, in bytes, of the space to
 * describe.  Describing N bytes costs N/32 bytes here, since we only record
 * word addresses (4-byte alignment) and pack 8 per byte.
 *
 * Thus, an AddressSet covering the entire 256MB code region costs 8MB, plus
 * a small constant overhead (~16 bytes).
 */
class AddressSet {
 public:
  /*
   * Creates an AddressSet describing 'size' bytes starting at 'base'.
   */
  AddressSet(uint32_t base, uint32_t size);
  ~AddressSet();

  /*
   * Adds an address to the set.
   * - If the address is already in the set, nothing changes.
   * - If the address is outside of this set's range (delimited by base and size
   *   provided at construction), nothing changes.
   * - If the address is not 4-byte aligned, the address of the 4-byte word
   *   containing the address is added instead.
   */
  void Add(uint32_t address);

  /*
   * Checks whether the AddressSet contains an address.  If the address is not
   * 4-byte aligned, the address of the 4-byte word containing the address is
   * checked instead.
   */
  bool Contains(uint32_t address) const;

  class Iterator {
   public:
    Iterator(const AddressSet &, uint32_t index, uint32_t shift);
    Iterator& Next();
    bool Equals(const Iterator &) const;
    uint32_t GetAddress() const;

   private:
    void Advance();

    const AddressSet &parent_;
    uint32_t index_;
    uint32_t shift_;
  };

  Iterator Begin() const;
  Iterator End() const;

 private:
  const uint32_t base_;
  const uint32_t size_;
  const uint32_t end_;
  uint32_t * const bits_;

  // Disallow copies - we don't need them, and don't want to refcount bits_.
  AddressSet(const AddressSet &);
  AddressSet &operator=(const AddressSet &);
};

}  // namespace

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_ADDRESS_SET_H
