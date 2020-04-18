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
goog.provide('i18n.input.chrome.inputview.elements.content.GaussianEstimator');


goog.scope(function() {

/**
 * A tool class to calculate probability with Gaussian distribution.
 * Gaussian(x, y) = Norm * exp (-(1/2) * ((x - centerX) ^ 2 * CinvX + (y -
 * centerY) ^ 2 * CinvY))
 * where
 * CinvX = 1 / (AmplitudeX * Covariance)
 * CinvY = 1 / (AmplitudeY * Covariance)
 * Norm = 1 / (2 * PI) * Sqrt(CinX * CinY)
 * LogGaussian(x, y) = LogNorm + (-1/2) * ((x - centerX) ^ 2 * CinvX
 * + (y - centerY) ^ 2 * CinvY))
 * In this class we assumes amplitude Y is normalized to 1, so
 * amplitude X is real amplitude X relative to amplitude Y.
 *
 * @param {!goog.math.Coordinate} center .
 * @param {number} covariance .
 * @param {!number} amplitude Amplitude on dimension X of the distribution. The
 * estimator assumes amplitude on dimension Y is 1, so this value is real
 * amplitude X relative to amplitude Y.
 * @constructor
 */
i18n.input.chrome.inputview.elements.content.GaussianEstimator = function(
    center, covariance, amplitude) {
  /**
   * The center point.
   *
   * @private {!goog.math.Coordinate}
   */
  this.center_ = center;

  /**
   * The CinvX.
   *
   * @private {number}
   */
  this.cinvX_ = 1 / (amplitude * covariance);

  /**
   * The CinvY.
   *
   * @private {number}
   */
  this.cinvY_ = 1 / covariance;

  /**
   * The Norm in log space.
   *
   * @private {number}
   */
  this.logNorm_ = Math.log(1 / (2 * Math.PI * Math.sqrt(amplitude *
      covariance * covariance)));
};
var GaussianEstimator = i18n.input.chrome.inputview.elements.content.
    GaussianEstimator;


/**
 * Estimates the possibility in log space.
 *
 * @param {number} x .
 * @param {number} y .
 */
GaussianEstimator.prototype.estimateInLogSpace = function(x, y) {
  var dx = x - this.center_.x;
  var dy = y - this.center_.y;
  var exponent = this.cinvX_ * dx * dx + this.cinvY_ * dy * dy;
  return this.logNorm_ + (-0.5) * exponent;
};

}); // goog.scope
