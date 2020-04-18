// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_TRUETYPE_FONT_H_
#define PPAPI_CPP_TRUETYPE_FONT_H_

#include <stdint.h>

#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/pass_ref.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the API to create a TrueType font object.

namespace pp {

class InstanceHandle;

// TrueTypeFontDesc_Dev --------------------------------------------------------

/// The <code>TrueTypeFontDesc_Dev</code> class represents a TrueType font
/// descriptor, used to Create and Describe fonts.
class TrueTypeFontDesc_Dev {
 public:
  /// Default constructor for creating a <code>TrueTypeFontDesc_Dev</code>
  /// object.
  TrueTypeFontDesc_Dev();

  /// Constructor that takes an existing <code>PP_TrueTypeFontDesc_Dev</code>
  /// structure. The 'family' PP_Var field in the structure will be managed by
  /// this instance.
  TrueTypeFontDesc_Dev(PassRef, const PP_TrueTypeFontDesc_Dev& pp_desc);

  /// The copy constructor for <code>TrueTypeFontDesc_Dev</code>.
  ///
  /// @param[in] other A reference to a <code>TrueTypeFontDesc_Dev</code>.
  TrueTypeFontDesc_Dev(const TrueTypeFontDesc_Dev& other);

  ~TrueTypeFontDesc_Dev();

  TrueTypeFontDesc_Dev& operator=(const TrueTypeFontDesc_Dev& other);

  const PP_TrueTypeFontDesc_Dev& pp_desc() const {
    return desc_;
  }

  Var family() const {
    return family_;
  }
  void set_family(const Var& family) {
    family_ = family;
    // The assignment above manages the underlying PP_Vars. Copy the new one
    // into the wrapped desc struct.
    desc_.family = family_.pp_var();
  }

  PP_TrueTypeFontFamily_Dev generic_family() const {
    return desc_.generic_family;
  }
  void set_generic_family(PP_TrueTypeFontFamily_Dev family) {
    desc_.generic_family = family;
  }

  PP_TrueTypeFontStyle_Dev style() const { return desc_.style; }
  void set_style(PP_TrueTypeFontStyle_Dev style) {
    desc_.style = style;
  }

  PP_TrueTypeFontWeight_Dev weight() const { return desc_.weight; }
  void set_weight(PP_TrueTypeFontWeight_Dev weight) {
    desc_.weight = weight;
  }

  PP_TrueTypeFontWidth_Dev width() const { return desc_.width; }
  void set_width(PP_TrueTypeFontWidth_Dev width) {
    desc_.width = width;
  }

  PP_TrueTypeFontCharset_Dev charset() const { return desc_.charset; }
  void set_charset(PP_TrueTypeFontCharset_Dev charset) {
    desc_.charset = charset;
  }

 private:
  pp::Var family_;  // This manages the PP_Var embedded in desc_.
  PP_TrueTypeFontDesc_Dev desc_;
};

// TrueTypeFont_Dev ------------------------------------------------------------

/// The <code>TrueTypeFont_Dev</code> class represents a TrueType font resource.
class TrueTypeFont_Dev : public Resource {
 public:
  /// Default constructor for creating a <code>TrueTypeFont_Dev</code> object.
  TrueTypeFont_Dev();

  /// A constructor used to create a <code>TrueTypeFont_Dev</code> and associate
  /// it with the provided <code>Instance</code>.
  ///
  /// @param[in] instance The instance that owns this resource.
  TrueTypeFont_Dev(const InstanceHandle& instance,
                   const TrueTypeFontDesc_Dev& desc);

  /// The copy constructor for <code>TrueTypeFont_Dev</code>.
  ///
  /// @param[in] other A reference to a <code>TrueTypeFont_Dev</code>.
  TrueTypeFont_Dev(const TrueTypeFont_Dev& other);

  /// A constructor used when you have received a PP_Resource as a return
  /// value that has had its reference count incremented for you.
  ///
  /// @param[in] resource A PP_Resource corresponding to a TrueType font.
  TrueTypeFont_Dev(PassRef, PP_Resource resource);

  /// Gets an array of TrueType font family names available on the host.
  /// These names can be used to create a font from a specific family.
  ///
  /// @param[in] instance A <code>InstanceHandle</code> requesting the family
  /// names.
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion of GetFontFamilies.
  ///
  /// @return If >= 0, the number of family names returned, otherwise an error
  /// code from <code>pp_errors.h</code>.
  static int32_t GetFontFamilies(
      const InstanceHandle& instance,
      const CompletionCallbackWithOutput<std::vector<Var> >& callback);

  /// Gets an array of TrueType font descriptors for a given font family. These
  /// descriptors can be used to create a font in that family and matching the
  /// descriptor attributes.
  ///
  /// @param[in] instance A <code>PP_Instance</code> requesting the font
  /// descriptors.
  /// @param[in] family A <code>Var</code> holding a string specifying the font
  /// family.
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion of GetFontsInFamily.
  ///
  /// @return If >= 0, the number of font descriptors returned, otherwise an
  /// error code from <code>pp_errors.h</code>.
  static int32_t GetFontsInFamily(
      const InstanceHandle& instance,
      const Var& family,
      const CompletionCallbackWithOutput<std::vector<TrueTypeFontDesc_Dev> >&
          callback);

  /// Returns a description of the given font resource. This description may
  /// differ from the description passed to Create, reflecting the host's font
  /// matching and fallback algorithm.
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion of Describe.
  ///
  /// @return A return code from <code>pp_errors.h</code>. If an error code is
  /// returned, the descriptor will be left unchanged.
  int32_t Describe(
      const CompletionCallbackWithOutput<TrueTypeFontDesc_Dev>& callback);

  /// Gets an array of identifying tags for each table in the font.
  /// These tags can be used to request specific tables using GetTable.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion of GetTableTags.
  ///
  /// @return If >= 0, the number of table tags returned, otherwise an error
  /// code from <code>pp_errors.h</code>.
  int32_t GetTableTags(
      const CompletionCallbackWithOutput<std::vector<uint32_t> >& callback);

  /// Copies the given font table into client memory.
  ///
  /// @param[in] table A 4 byte value indicating which table to copy.
  /// For example, 'glyf' will cause the outline table to be copied into the
  /// output array. A zero tag value will cause the entire font to be copied.
  /// @param[in] offset The offset into the font table.
  /// @param[in] max_data_length The maximum number of bytes to transfer from
  /// <code>offset</code>.
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion of GetTable.
  ///
  /// @return If >= 0, the table size in bytes, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t GetTable(
      uint32_t table,
      int32_t offset,
      int32_t max_data_length,
      const CompletionCallbackWithOutput<std::vector<char> >& callback);
};

namespace internal {

// A specialization of CallbackOutputTraits to provide the callback system the
 // information on how to handle pp::TrueTypeFontDesc_Dev. This converts
// PP_TrueTypeFontDesc_Dev to pp::TrueTypeFontDesc_Dev when passing to the
// plugin, and specifically manages the PP_Var embedded in the desc_ field.
template<>
struct CallbackOutputTraits<TrueTypeFontDesc_Dev> {
  typedef PP_TrueTypeFontDesc_Dev* APIArgType;
  typedef PP_TrueTypeFontDesc_Dev StorageType;

  static inline APIArgType StorageToAPIArg(StorageType& t) {
    return &t;
  }

  static inline TrueTypeFontDesc_Dev StorageToPluginArg(StorageType& t) {
    return TrueTypeFontDesc_Dev(PASS_REF, t);
  }

  static inline void Initialize(StorageType* t) {
    // Use the same defaults as TrueTypeFontDesc_Dev does.
    TrueTypeFontDesc_Dev dummy;
    *t = dummy.pp_desc();
  }
};

class TrueTypeFontDescArrayOutputAdapterWithStorage
    : public ArrayOutputAdapter<PP_TrueTypeFontDesc_Dev> {
 public:
  TrueTypeFontDescArrayOutputAdapterWithStorage() {
    set_output(&temp_storage_);
  };

  virtual ~TrueTypeFontDescArrayOutputAdapterWithStorage() {
    if (!temp_storage_.empty()) {
      // An easy way to release the resource references held by |temp_storage_|.
      output();
    }
  };

  std::vector<TrueTypeFontDesc_Dev>& output() {
    PP_DCHECK(output_storage_.empty());
    typedef std::vector<PP_TrueTypeFontDesc_Dev> Entries;
    for (Entries::iterator it = temp_storage_.begin();
         it != temp_storage_.end(); ++it)
      output_storage_.push_back(TrueTypeFontDesc_Dev(PASS_REF, *it));
    temp_storage_.clear();
    return output_storage_;
  }

 private:
  std::vector<PP_TrueTypeFontDesc_Dev> temp_storage_;
  std::vector<TrueTypeFontDesc_Dev> output_storage_;
};

// A specialization of CallbackOutputTraits to provide the callback system the
// information on how to handle vectors of TrueTypeFontDesc_Dev. This converts
// PP_TrueTypeFontDesc_Dev to TrueTypeFontDesc_Dev when passing to the plugin.
template<>
struct CallbackOutputTraits< std::vector<TrueTypeFontDesc_Dev> > {
  typedef PP_ArrayOutput APIArgType;
  typedef TrueTypeFontDescArrayOutputAdapterWithStorage StorageType;

  static inline APIArgType StorageToAPIArg(StorageType& t) {
    return t.pp_array_output();
  }

  static inline std::vector<TrueTypeFontDesc_Dev>& StorageToPluginArg(
      StorageType& t) {
    return t.output();
  }

  static inline void Initialize(StorageType* /* t */) {}
};

}  // namespace internal

}  // namespace pp

#endif  // PPAPI_CPP_TRUETYPE_FONT_H_
