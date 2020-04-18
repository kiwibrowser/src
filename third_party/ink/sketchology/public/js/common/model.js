// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.Model');

goog.require('goog.asserts');
goog.require('goog.events.EventTarget');
goog.require('ink.util');



/**
 * A generic model base class. Each models here is a singleton per EventTarget
 * hierarchy tree.
 *
 * @extends {goog.events.EventTarget}
 * @constructor
 * @struct
 */
ink.Model = function() {
  ink.Model.base(this, 'constructor');
};
goog.inherits(ink.Model, goog.events.EventTarget);


/**
 * The models are attached to the root parent EventTargets. To have the models
 * be automatically gc properly the relevant models need to actually be
 * properties of those EventTargets. This property is initialized to avoid
 * collisions with other JavaScript on the same page, similarly to the property
 * that goog.getUid uses.
 * @private {string}
 */
ink.Model.MODEL_INSTANCES_PROPERTY_ = 'ink_model_instances_' + Math.random();


/**
 * Adds the getter to the model constructor, allowing for the simpler
 * ink.BrushModel.getInstance(this); instead of
 * ink.Model.get(ink.BrushModel, this);
 * @param {!function(new:ink.Model, !goog.events.EventTarget)} modelCtor
 *      The model constructor.
 */
ink.Model.addGetter = function(modelCtor) {
  /**
   * @param {!goog.events.EventTarget} observer
   * @return {!ink.Model}
   */
  modelCtor.getInstance = function(observer) {
    goog.asserts.assertObject(observer);
    return ink.Model.get(modelCtor, observer);
  };
};


/**
 * Gets the relevant model for the provided viewer. The viewer should be a
 * goog.ui.Component that has entered the document or a goog.events.EventTarget
 * that has already had its parentEventTarget set.
 *
 * Note: This currently assumes that the provided models are singletons per
 * EventTarget hierarchy tree. A more flexible design for deciding what level
 * to have models should be added here if usage demands it.
 *
 * @param {!function(new:ink.Model, !goog.events.EventTarget)} modelCtor
 * @param {!goog.events.EventTarget} observer
 * @return {!ink.Model}
 */
ink.Model.get = function(modelCtor, observer) {
  // TODO(esrauch): Maybe this should be implemented based on dom elements
  // instead of the goog.ui.Component hierarchy. As it is, a stray setParent()
  // call could cause the Model instance to suprisingly change for the same
  // observer. On the other hand, reading the dom is slower and also can cause
  // a brower reflow unnecessarily and this way also allows for vanilla
  // EventTargets to get the relevant Models.
  var root = ink.util.getRootParentComponent(observer);
  var models = root[ink.Model.MODEL_INSTANCES_PROPERTY_];
  if (!models) {
    root[ink.Model.MODEL_INSTANCES_PROPERTY_] = models = {};
  }
  var key = goog.getUid(modelCtor);
  var oldInstance = models[key];
  if (oldInstance) {
    return oldInstance;
  }
  var newInstance = new modelCtor(root);
  models[key] = newInstance;
  return newInstance;
};
