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
 * Annotate methods that are used to automatically convert from the expression type to the value
 * used in the setter. The converter should take one parameter, the expression type, and the
 * return value should be the target value type used in the setter. Converters are used
 * whenever they can be applied and are not specific to any attribute.
 */
@Target({ElementType.METHOD})
public @interface BindingConversion {
}
