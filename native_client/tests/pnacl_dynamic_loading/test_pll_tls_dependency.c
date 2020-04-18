/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This is an example library, for testing the ConvertToPSO pass.
 * Additionally, this library specifically tests exporting TLS variables.
 */

__thread int tls_var_exported1 = 1234;
__thread int tls_var_exported2 = 5678;
