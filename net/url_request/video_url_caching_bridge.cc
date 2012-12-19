/*
 * Copyright (C)  2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "build/build_config.h"

#include <unistd.h>
#include <string>
#include <vector>
#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "googleurl/src/gurl.h"
#include "base/compiler_specific.h"
#include "net/url_request/video_url_caching_bridge.h"

#define LOG_TAG "VideoUrlCaching"

static bool wasUrlCachingOnInitialized = false;
static bool urlCachingOn = false;

const char* kFeatureEnabelingSysPropName = "net.media.redirect.cache.enable";
const char* kEnabledAppName = "/system/bin/mediaserver";

static bool (*FeatureState)() = NULL;

static std::string (*GetVersion)() = NULL;

static void (*DoObserveStart)(std::vector<GURL>& orig_url_chain) = NULL;

static void (*DoObserveResponseStarted)(std::vector<GURL>& orig_url_chain) = NULL;

static bool InitOnce() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        void* fh = dlopen("libvideo_cor.so", RTLD_LAZY);
        dlerror(); //see man dlopen
        if (fh) {
            SLOGD("libvideo_cor.so was successfully loaded");
            *(void **)(&FeatureState) = dlsym(fh, "FeatureState");
            *(void **)(&GetVersion) = dlsym(fh, "GetVersion");
            *(void **)(&DoObserveStart) = dlsym(fh, "DoObserveStart");
            *(void **)(&DoObserveResponseStarted) = dlsym(fh, "DoObserveResponseStarted");
        }
        else{
            SLOGD("libvideo_cor.so wasn't loaded");
            return false;
        }
        if (NULL == FeatureState){
            SLOGD("Failed to find FeatureState symbol in libvideo_cor.so");
            return false;
        }
        if (NULL == GetVersion){
            SLOGD("Failed to find GetVersion symbol in libvideo_cor.so");
            return false;
        }
        if (NULL == DoObserveStart){
            SLOGD("Failed to find DoObserveStart symbol in libvideo_cor.so");
            return false;
        }
        if (NULL == DoObserveResponseStarted){
            SLOGD("Failed to find DoObserveResponseStarted symbol in libvideo_cor.so");
            return false;
        }
    }
    return true;
}

void ObserveStart(std::vector<GURL>& orig_url_chain){
    if (DoObserveStart){
        DoObserveStart(orig_url_chain);
    }
}

void ObserveResponseStarted(std::vector<GURL>& orig_url_chain){
    if (DoObserveResponseStarted){
        DoObserveResponseStarted(orig_url_chain);
    }
}

bool GetUrlCachingState(){
    char path[128] = {'\0'};
    pid_t pid = getpid();
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0){
        return false;
    }
    int rd_len = read(fd, path , sizeof(path)-1);
    close(fd);
    if (0 > rd_len) {
        rd_len = 0;
    }
    path[rd_len] = 0;

    if (strcmp(path, kEnabledAppName)) {
        return false;
    }
    char value[PROPERTY_VALUE_MAX]={'\0'};

    property_get(kFeatureEnabelingSysPropName, value, "1");

    if (value[0] == '1'){
        if (InitOnce()) {
            if (FeatureState()){
                SLOGD("VideoCOR is: ON, Version: %s", GetVersion().c_str());
                return true;
            }
        }
    }
    SLOGD("VideoCOR is OFF");
    return false;
}

bool UrlCachingState()
{
    if (!wasUrlCachingOnInitialized){
        wasUrlCachingOnInitialized = true;
        urlCachingOn = GetUrlCachingState();
    }
    return urlCachingOn;
}


