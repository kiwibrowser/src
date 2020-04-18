/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *
 * COPYRIGHT NVIDIA CORPORATION 2003. ALL RIGHTS RESERVED.
 * BY ACCESSING OR USING THIS SOFTWARE, YOU AGREE TO:
 *
 *  1) ACKNOWLEDGE NVIDIA'S EXCLUSIVE OWNERSHIP OF ALL RIGHTS
 *     IN AND TO THE SOFTWARE;
 *
 *  2) NOT MAKE OR DISTRIBUTE COPIES OF THE SOFTWARE WITHOUT
 *     INCLUDING THIS NOTICE AND AGREEMENT;
 *
 *  3) ACKNOWLEDGE THAT TO THE MAXIMUM EXTENT PERMITTED BY
 *     APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS* AND
 *     THAT NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES,
 *     EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED
 *     TO, IMPLIED WARRANTIES OF MERCHANTABILITY  AND FITNESS
 *     FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
 * SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS
 * OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS
 * INFORMATION, OR ANY OTHER PECUNIARY LOSS), INCLUDING ATTORNEYS'
 * FEES, RELATING TO THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 */


/**
 * Does the heavy lifting of calculating the clientArray buffer. The only thing
 * this modifies is conf.clientArray; everything else is read-only.
 *
 * @param {Object}
 *              conf the calculation config object holding data to parse.
 */
function calculate(conf, precalc, slabData) {

  var loX = new PeriodicIterator(precalc.constants.SIN_ARRAY_SIZE,
          2 * Math.PI,
          conf.config.phase,
          (1.0 / conf.config.tileSize) * conf.config.lofreq * Math.PI);
  var hiX = new PeriodicIterator(precalc.constants.SIN_ARRAY_SIZE,
          2 * Math.PI,
          conf.config.phase2,
          (1.0 / conf.config.tileSize) * conf.config.hifreq * Math.PI);

  var vertexIndex = 0;

  var locoef_tmp = conf.config.locoef;
  var hicoef_tmp = conf.config.hicoef;
  var ysinlo_tmp = slabData.arrays[conf.slab].ysinlo;
  var ysinhi_tmp = slabData.arrays[conf.slab].ysinhi;
  var ycoslo_tmp = slabData.arrays[conf.slab].ycoslo;
  var ycoshi_tmp = slabData.arrays[conf.slab].ycoshi;
  var xyArray_tmp = slabData.xyArray;
  var sinArray_tmp = precalc.sinArray;
  var cosArray_tmp = precalc.cosArray;

  for (var i = conf.config.tileSize; --i >= 0; ) {
      var x = xyArray_tmp[i];
      var loXIndex = loX.getIndex();
      var hiXIndex = hiX.getIndex();

      var jOffset = (precalc.constants.STRIP_SIZE - 1) * conf.slab;
      var nx = (locoef_tmp * -cosArray_tmp[loXIndex] + (hicoef_tmp *-cosArray_tmp[hiXIndex] ));

      var v = conf.clientArray;

      for (var j = precalc.constants.STRIP_SIZE; --j >= 0; ) {
          var y = xyArray_tmp[j + jOffset];
          var nx1 = nx;
          var ny = locoef_tmp * -ycoslo_tmp[j] + hicoef_tmp * -ycoshi_tmp[j];

          v[vertexIndex] = x;
          v[vertexIndex + 1] = y;
          v[vertexIndex + 2] = (locoef_tmp * (sinArray_tmp[loXIndex] + ysinlo_tmp[j]) +
                  hicoef_tmp * (sinArray_tmp[hiXIndex] +
                  ysinhi_tmp[j]));
          v[vertexIndex + 3] = nx1;
          v[vertexIndex + 4] = ny;
          v[vertexIndex + 5] = 0.15*(1.0 - Math.sqrt(nx1 * nx1 + ny * ny));
          vertexIndex += 6;
      }
      loX.incr();
      hiX.incr();
  }
  loX.reset();
  hiX.reset();

}
