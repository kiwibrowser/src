// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Interface for a store that allows objects to be added/removed along with the
// ability to get the digest for the whole or partial set of those objects.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_DIGEST_STORE_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_DIGEST_STORE_H_

#include <vector>

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::vector;

template<typename ElementType>
class DigestStore {
 public:
  virtual ~DigestStore() {}

  /* Returns the number of elements. */
  virtual int size() = 0;

  /* Returns whether element is in the store. */
  virtual bool Contains(const ElementType& element) = 0;

  /* Returns a digest of the desired objects in 'digest'.
   *
   * NOTE: the digest computations MUST NOT depend on the order in which the
   * elements were added.
   */
  virtual string GetDigest() = 0;

  /* Stores iterators bounding the elements whose digest prefixes begin with the
   * bit prefix digest_prefix.  prefix_len is the length of digest_prefix in
   * bits, which may be less than digest_prefix.length (and may be 0). The
   * implementing class can return *more* objects than what has been specified
   * by digest_prefix, e.g., it could return all the objects in the store.
   */
  virtual void GetElements(const string& digest_prefix, int prefix_len,
      vector<ObjectIdP>* result) = 0;

  /* Adds element to the store. No-op if element is already present.
   * Returns whether the element was added.
   */
  virtual bool Add(const ElementType& element) = 0;

  /* Adds elements to the store. If any element in elements is already present,
   * the addition is a no-op for that element. When the function returns,
   * added_elements will have been modified to contain all the elements
   * of elements that were not previously present.
   */
  virtual void Add(const vector<ElementType>& elements,
      vector<ElementType>* added_elements) = 0;

  /* Removes element from the store. No-op if element is not present.
   * Returns whether the element was removed.
   */
  virtual bool Remove(const ElementType& element) = 0;

  /* Remove elements from the store. If any element in element is not present,
   * the removal is a no-op for that element.
   * When the function returns, removed_elements will have been modified to
   * contain all the elements of elements that were previously present
   */
  virtual void Remove(const vector<ElementType>& elements,
      vector<ElementType>* removed_elements) = 0;

  /* Removes all elements in this and stores them in elements. */
  virtual void RemoveAll(vector<ElementType>* elements) = 0;

  /* Returns a string representation of this digest store. */
  virtual string ToString() = 0;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_DIGEST_STORE_H_
