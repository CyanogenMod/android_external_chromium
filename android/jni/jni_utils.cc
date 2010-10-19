 // Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android/jni/jni_utils.h"

// We currently delegate our implementation to WebKit, as WebKit controls the
// JNI startup process. However, we can't #include relevant headers because they
// require many extra include paths and #defines that clash with our existing
// ones. So we forward-declare the methods instead.
// TODO: Disentangle Android JNI from WebKit.

// JNIUtility.h
namespace JSC {
namespace Bindings {
JNIEnv* getJNIEnv();
}
}

// WebCoreJni.h
namespace android {
std::string jstringToStdString(JNIEnv* env, jstring jstr);
string16 jstringToString16(JNIEnv* env, jstring jstr);
}

namespace android {

JNIEnv* GetJNIEnv() {
  return JSC::Bindings::getJNIEnv();
}

std::string JstringToStdString(JNIEnv* env, jstring jstr) {
  return jstringToStdString(env, jstr);
}

string16 JstringToString16(JNIEnv* env, jstring jstr)
{
    return jstringToString16(env, jstr);
}

} // namespace android

