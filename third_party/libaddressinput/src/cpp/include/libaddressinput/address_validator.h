// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// The public interface to the address validation features of libaddressinput.
// The AddressValidator will examine an AddressData struct and return a map of
// the problems found with the different fields of this struct.

#ifndef I18N_ADDRESSINPUT_ADDRESS_VALIDATOR_H_
#define I18N_ADDRESSINPUT_ADDRESS_VALIDATOR_H_

#include <libaddressinput/address_field.h>
#include <libaddressinput/address_problem.h>
#include <libaddressinput/callback.h>

#include <map>

namespace i18n {
namespace addressinput {

class Supplier;
struct AddressData;

typedef std::multimap<AddressField, AddressProblem> FieldProblemMap;

// Validates an AddressData struct. Sample usage:
//    class MyClass {
//     public:
//      MyClass()
//          : supplier_(new MySupplier),
//            validator_(new AddressValidator(supplier_.get())),
//            validated_(BuildCallback(this, &MyClass::Validated)) {}
//
//      virtual ~MyClass() {}
//
//      void ValidateAddress() const {
//        address_.region_code = "US";
//        address_.administrative_area = "CA";
//        validator_.Validate(address_, filter_, &problems_, *validated_);
//      }
//
//      void Validated(bool success,
//                     const AddressData& address,
//                     const FieldProblemMap& problems) {
//        if (success && problems.empty()) {
//          ...
//        }
//      }
//
//     private:
//      AddressData address_;
//      FieldProblemMap filter_;
//      FieldProblemMap problems_;
//      const std::unique_ptr<Supplier> supplier_;
//      const std::unique_ptr<AddressValidator> validator_;
//      const std::unique_ptr<const AddressValidator::Callback> validated_;
//    };
class AddressValidator {
 public:
  typedef i18n::addressinput::Callback<const AddressData&,
                                       const FieldProblemMap&> Callback;

  AddressValidator(const AddressValidator&) = delete;
  AddressValidator& operator=(const AddressValidator&) = delete;

  // Does not take ownership of |supplier|.
  AddressValidator(Supplier* supplier);
  ~AddressValidator();

  // Validates the |address| and populates |problems| with the validation
  // problems, filtered according to the |filter| parameter.
  //
  // Set |allow_postal| to allow postal addresses, rather than only addresses
  // describing physical locations.
  //
  // Set |require_name| if recipient should be considered a required field.
  //
  // If the |filter| is nullptr or empty, then all discovered validation
  // problems are returned. If the |filter| contains problem elements, then only
  // those field-problem pairs present in the |filter| will be returned.
  //
  // Calls the |validated| callback when validation is done. All objects passed
  // as parameters must be kept available until the callback has been called.
  //
  // The |success| parameter of the callback indicates whether it was possible
  // to perform validation. If |success| is true, then |problems| will contain
  // information about any problems found with the |address|.
  void Validate(const AddressData& address,
                bool allow_postal,
                bool require_name,
                const FieldProblemMap* filter,
                FieldProblemMap* problems,
                const Callback& validated) const;

 private:
  Supplier* const supplier_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_VALIDATOR_H_
