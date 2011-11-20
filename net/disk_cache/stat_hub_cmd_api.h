/** ---------------------------------------------------------------------------
Copyright (c) 2011, 2012 Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Code Aurora Forum, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------**/

#ifndef STAT_HUB_CMD_API_H_
#define STAT_HUB_CMD_API_H_

typedef enum {
    INPUT_CMD_TBD_0,                // 0 TBD
    INPUT_CMD_WK_MAIN_URL,          // 1
    INPUT_CMD_WK_SUB_URL_REQUEST,   // 2
    INPUT_CMD_TBD_3,                // 3 TBD
    INPUT_CMD_WK_MAIN_URL_LOADED,   // 4
    INPUT_CMD_WK_RES_MMC_STATUS,    // 5
    INPUT_CMD_WK_MMC_CLEAR,         // 6
    INPUT_CMD_TBD_7,                // 7 TBD
    INPUT_CMD_CH_URL_REQUEST,       // 8
    INPUT_CMD_WK_RES_LOAD_FINISHED, // 9
    INPUT_CMD_WK_START_PAGE_LOAD,   // 10
    INPUT_CMD_WK_FINISH_PAGE_LOAD,  // 11
    INPUT_CMD_TBD_12,               // 12 TBD
    INPUT_CMD_CH_URL_REQUEST_DONE,  // 13
    INPUT_CMD_WK_JS_SEQ,            // 14

    INPUT_CMD_USER_DEFINED = 32     // 256
} StatHubInputCmd;

typedef union {
    unsigned value;
    struct bf {
        unsigned cacheable:1;       // first bit
        unsigned mime_type:3;       // 3 bits: up to 6 types defined in CachedResource.cpp
    } bf;
} UrlProperty;

// ================================ StatHub CMD Interface ====================================
extern unsigned int StatHubHash(const char* str)
    __attribute__ ((visibility ("default"), used));
extern void StatHubUpdateMainUrl(const char* url)
    __attribute__ ((visibility ("default"), used));
extern void StatHubUpdateSubUrl(const char* main_url, const char* sub_url)
    __attribute__ ((visibility ("default"), used));
extern void StatHubMainUrlLoaded(const char* url)
    __attribute__ ((visibility ("default"), used));
extern void StatHubCmd(unsigned short cmd, void* param1, int sizeofparam1, void* param2, int sizeofparam2)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubIsProcReady(const char* name)
    __attribute__ ((visibility ("default"), used));
#endif /* STAT_HUB_CMD_API_H_ */
