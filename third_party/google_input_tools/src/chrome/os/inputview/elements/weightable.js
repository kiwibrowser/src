// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.inputview.elements.Weightable');



/**
 * Interface defines the weight operation to an element which is computed
 * with weight.
 *
 * @interface
 */
i18n.input.chrome.inputview.elements.Weightable = function() {};


/**
 * Gets the width of the element in weight.
 *
 * @return {number} The width.
 */
i18n.input.chrome.inputview.elements.Weightable.prototype.getWidthInWeight =
    goog.abstractMethod;


/**
 * Gets the height of the element in weight.
 *
 * @return {number} The height.
 */
i18n.input.chrome.inputview.elements.Weightable.prototype.getHeightInWeight =
    goog.abstractMethod;


