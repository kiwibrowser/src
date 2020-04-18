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
package android.databinding;

import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

/**
 * Used within an {@link BindingMethods} annotation to describe a renaming of an attribute to
 * the setter used to set that attribute. By default, an attribute attr will be associated with
 * setter setAttr.
 */
@Target(ElementType.ANNOTATION_TYPE)
public @interface BindingMethod {

    /**
     * @return the View Class that the attribute is associated with.
     */
    Class type();

    /**
     * @return The attribute to rename. Use android: namespace for all android attributes or
     * no namespace for application attributes.
     */
    String attribute();

    /**
     * @return The method to call to set the attribute value.
     */
    String method();
}
