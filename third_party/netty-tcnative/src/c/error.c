/*
 * Copyright 2016 The Netty Project
 *
 * The Netty Project licenses this file to you under the Apache License,
 * version 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tcn.h"
#include "apr_strings.h"

/*
 * Convenience function to help throw an java.lang.Exception.
 */
void tcn_ThrowException(JNIEnv *env, const char *msg)
{
    jclass javaExceptionClass;

    javaExceptionClass = (*env)->FindClass(env, "java/lang/Exception");
    if (javaExceptionClass == NULL) {
        fprintf(stderr, "Cannot find java/lang/Exception class\n");
        return;
    }
    (*env)->ThrowNew(env, javaExceptionClass, msg);
    (*env)->DeleteLocalRef(env, javaExceptionClass);

}

void tcn_Throw(JNIEnv *env, const char *fmt, ...)
{
    char msg[TCN_BUFFER_SZ] = {'\0'};
    va_list ap;

    va_start(ap, fmt);
    apr_vsnprintf(msg, TCN_BUFFER_SZ, fmt, ap);
    tcn_ThrowException(env, msg);
    va_end(ap);
}

void tcn_ThrowAPRException(JNIEnv *e, apr_status_t err)
{
    char serr[512] = {0};

    apr_strerror(err, serr, 512);
    tcn_ThrowException(e, serr);
}
