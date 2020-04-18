/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Dummy empty file to force c++ linking rather than c linking.
 *
 * For consistency with XCode builds, the ninja and make GYP generators will
 * not link libstdc++ unless there is a C++ source file specified in the
 * target (see GYP revision r1721:
 * https://code.google.com/p/gyp/source/detail?r=1721).
 *
 * This file is included as a workaround for this bug.
 */
