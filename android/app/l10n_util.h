/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ANDROID_APP_L10N_UTIL_H_
#define ANDROID_APP_L10N_UTIL_H_
#pragma once

#include "base/string16.h"
#include "jni/jni_utils.h"

#define IDS_AUTOFILL_DIALOG_ADDRESS_NAME_SEPARATOR 0
#define IDS_AUTOFILL_DIALOG_ADDRESS_SUMMARY_NAME_FORMAT 1
#define IDS_AUTOFILL_DIALOG_ADDRESS_SUMMARY_SEPARATOR 2
#define IDS_AUTOFILL_DIALOG_ADDRESS_SUMMARY_FORMAT 3
// Don't forget to update the count if you add a new message ID!
// (And also update frameworks/base/core/java/android/webkit/L10nUtils.java)
#define ANDROID_L10N_IDS_MESSAGE_COUNT 4

namespace l10n_util {
string16 GetStringUTF16(int message_id);
string16 GetStringFUTF16(int message_id, const string16& a, const string16& b, const string16& c);
}
#endif
