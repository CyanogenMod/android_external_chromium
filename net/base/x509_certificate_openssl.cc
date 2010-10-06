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

#include "net/base/x509_certificate.h"

#include "openssl/x509.h"

namespace net {

X509Certificate::OSCertHandle X509Certificate::DupOSCertHandle(
    OSCertHandle cert_handle) {
  return NULL;
}

void X509Certificate::FreeOSCertHandle(OSCertHandle cert) {
}

void X509Certificate::Initialize() {
}

SHA1Fingerprint X509Certificate::CalculateFingerprint(
    OSCertHandle cert) {
  SHA1Fingerprint sha1;
  memcpy(sha1.data, cert->sha1_hash, 20);
  return sha1;
}

X509Certificate::OSCertHandle X509Certificate::CreateOSCertHandleFromBytes(
    char const*, int) {
  return NULL;
}

X509Certificate::OSCertHandles X509Certificate::CreateOSCertHandlesFromBytes(
    const char* data, int length, Format format) {
  OSCertHandles results;
  return results;
}

X509Certificate* X509Certificate::CreateFromPickle(const Pickle& pickle,
                                           void** pickle_iter) {
    return NULL;
}

void X509Certificate::Persist(Pickle* pickle) {
}


} // namespace net
