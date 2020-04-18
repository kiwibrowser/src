/* Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Valve Corporation
 * Copyright (c) 2016 LunarG, Inc.
 * Copyright (c) 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef PARAMETER_NAME_H
#define PARAMETER_NAME_H

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

/**
 * Parameter name string supporting deferred formatting for array subscripts.
 *
 * Custom parameter name class with support for deferred formatting of names containing array subscripts.  The class stores
 * a format string and a vector of index values, and performs string formatting when an accessor function is called to
 * retrieve the name string.  This class was primarily designed to be used with validation functions that receive a parameter name
 * string and value as arguments, and print an error message that includes the parameter name when the value fails a validation
 * test.  Using standard strings with these validation functions requires that parameter names containing array subscripts be
 * formatted before each validation function is called, performing the string formatting even when the value passes validation
 * and the string is not used:
 *         sprintf(name, "pCreateInfo[%d].sType", i);
 *         validate_stype(name, pCreateInfo[i].sType);
 *
 * With the ParameterName class, a format string and a vector of format values are stored by the ParameterName object that is
 * provided to the validation function.  String formatting is then performed only when the validation function retrieves the
 * name string from the ParameterName object:
 *         validate_stype(ParameterName("pCreateInfo[%i].sType", IndexVector{ i }), pCreateInfo[i].sType);
 */
class ParameterName {
  public:
    /// Container for index values to be used with parameter name string formatting.
    typedef std::vector<size_t> IndexVector;

    /// Format specifier for the parameter name string, to be replaced by an index value.  The parameter name string must contain
    /// one format specifier for each index value specified.
    const std::string IndexFormatSpecifier = "%i";

  public:
    /**
     * Construct a ParameterName object from a string literal, without formatting.
     *
     * @param source Paramater name string without format specifiers.
     *
     * @pre The source string must not contain the %i format specifier.
     */
    ParameterName(const char *source) : source_(source) { assert(IsValid()); }

    /**
    * Construct a ParameterName object from a std::string object, without formatting.
    *
    * @param source Paramater name string without format specifiers.
    *
    * @pre The source string must not contain the %i format specifier.
    */
    ParameterName(const std::string &source) : source_(source) { assert(IsValid()); }

    /**
    * Construct a ParameterName object from a std::string object, without formatting.
    *
    * @param source Paramater name string without format specifiers.
    *
    * @pre The source string must not contain the %i format specifier.
    */
    ParameterName(const std::string &&source) : source_(std::move(source)) { assert(IsValid()); }

    /**
    * Construct a ParameterName object from a std::string object, with formatting.
    *
    * @param source Paramater name string with format specifiers.
    * @param args Array index values to be used for formatting.
    *
    * @pre The number of %i format specifiers contained by the source string must match the number of elements contained
    *      by the index vector.
    */
    ParameterName(const std::string &source, const IndexVector &args) : source_(source), args_(args) { assert(IsValid()); }

    /**
    * Construct a ParameterName object from a std::string object, with formatting.
    *
    * @param source Paramater name string with format specifiers.
    * @param args Array index values to be used for formatting.
    *
    * @pre The number of %i format specifiers contained by the source string must match the number of elements contained
    *      by the index vector.
    */
    ParameterName(const std::string &&source, const IndexVector &&args) : source_(std::move(source)), args_(std::move(args)) {
        assert(IsValid());
    }

    /// Retrive the formatted name string.
    std::string get_name() const { return (args_.empty()) ? source_ : Format(); }

  private:
    /// Replace the %i format specifiers in the source string with the values from the index vector.
    std::string Format() const {
        std::string::size_type current = 0;
        std::string::size_type last = 0;
        std::stringstream format;

        for (size_t index : args_) {
            current = source_.find(IndexFormatSpecifier, last);
            if (current == std::string::npos) {
                break;
            }
            format << source_.substr(last, (current - last)) << index;
            last = current + IndexFormatSpecifier.length();
        }

        format << source_.substr(last, std::string::npos);

        return format.str();
    }

    /// Check that the number of %i format specifiers in the source string matches the number of elements in the index vector.
    bool IsValid() {
        // Count the number of occurances of the format specifier
        uint32_t count = 0;
        std::string::size_type pos = source_.find(IndexFormatSpecifier);

        while (pos != std::string::npos) {
            ++count;
            pos = source_.find(IndexFormatSpecifier, pos + 1);
        }

        return (count == args_.size());
    }

  private:
    std::string source_; ///< Format string.
    IndexVector args_;   ///< Array index values for formatting.
};

#endif // PARAMETER_NAME_H
