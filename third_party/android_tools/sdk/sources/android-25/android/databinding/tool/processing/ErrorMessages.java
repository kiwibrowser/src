/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.processing;

public class ErrorMessages {
    public static final String INCLUDE_INSIDE_MERGE =
            "Data binding does not support include elements as direct children of a merge element.";
    public static final String UNDEFINED_VARIABLE =
            "Identifiers must have user defined types from the XML file. %s is missing it";
    public static final String CANNOT_FIND_SETTER_CALL =
            "Cannot find the setter for attribute '%s' with parameter type %s.";
    public static final String CANNOT_RESOLVE_TYPE =
            "Cannot resolve type for %s";
    public static final String MULTI_CONFIG_LAYOUT_CLASS_NAME_MISMATCH =
            "Classname (%s) does not match the class name defined for layout(%s) in other"
                    + " configurations";
    public static final String MULTI_CONFIG_VARIABLE_TYPE_MISMATCH =
            "Variable declaration (%s - %s) does not match the type defined for layout(%s) in other"
                    + " configurations";
    public static final String MULTI_CONFIG_IMPORT_TYPE_MISMATCH =
            "Import declaration (%s - %s) does not match the import defined for layout(%s) in other"
                    + " configurations";
    public static final String MULTI_CONFIG_ID_USED_AS_IMPORT =
            "Cannot use the same id (%s) for a View and an include tag.";
    public static final String ROOT_TAG_NOT_SUPPORTED = "android:tag is not supported on root " +
            "elements of data bound layouts unless targeting API version 14 or greater. Value " +
            "is '%s'";
    public static final String SYNTAX_ERROR = "Syntax error: %s";
    public static final String CANNOT_FIND_GETTER_CALL =
            "Cannot find the getter for attribute '%s' with value type %s on %s.";
    public static final String EXPRESSION_NOT_INVERTIBLE =
            "The expression %s cannot cannot be inverted: %s";
    public static final String TWO_WAY_EVENT_ATTRIBUTE =
            "The attribute %s is a two-way binding event attribute and cannot be assigned.";
}
