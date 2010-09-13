/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_P2P_BASE_SESSIONDESCRIPTION_H_
#define TALK_P2P_BASE_SESSIONDESCRIPTION_H_

namespace cricket {

// Describes a session. Individual session types can override this as needed.
class SessionDescription {
 public:
  virtual ~SessionDescription() {}
};

// Indicates whether a SessionDescription was an offer or an answer, as
// described in http://www.ietf.org/rfc/rfc3264.txt. DT_UPDATE
// indicates a jingle update message which contains a subset of a full
// session description
enum DescriptionType {
  DT_OFFER, DT_ANSWER, DT_UPDATE
};

// Indicates whether a SessionDescription was sent by the local client
// or received from the remote client.
enum DescriptionSource {
  DS_LOCAL, DS_REMOTE
};

}  // namespace cricket

#endif  // TALK_P2P_BASE_SESSIONDESCRIPTION_H_
