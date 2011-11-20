/**
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other *materials provided
 *     with the distribution.
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.

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
 **/

#include "net/http/http_getzip_factory.h"
#include "net/socket/client_socket.h"
#include <cutils/log.h>
#include <dlfcn.h>

namespace net
{

HttpGetZipFactory* HttpGetZipFactory::s_pFactory = NULL;

typedef IGetZipManager* mngr_create_();

HttpGetZipFactory::HttpGetZipFactory() :
  m_pMngr(NULL), libHandle(NULL)
{
}

HttpGetZipFactory::~HttpGetZipFactory()
{
  delete m_pMngr;
  m_pMngr = NULL;

  if (NULL != libHandle)
  {
    ::dlclose(libHandle);
    libHandle = NULL;
  }
}

void HttpGetZipFactory::InitGETZipManager()
{
  if (NULL != s_pFactory)
    return;

  s_pFactory = new HttpGetZipFactory();

  s_pFactory->libHandle = ::dlopen("libgetzip.so", RTLD_NOW);

  if (s_pFactory->libHandle)
  {
    SLOGD("%s: libgetzip.so successfully loaded", __FILE__);
    dlerror();
    mngr_create_* mngrCreate = (mngr_create_*) dlsym(s_pFactory->libHandle,
        "createGETZipManager");

    if (mngrCreate)
    {
      SLOGD("%s,: GETzip initializing method was found in libgetzip.so",
          __FILE__);
      s_pFactory->m_pMngr = (IGetZipManager*) mngrCreate();
      if( NULL == s_pFactory->m_pMngr)
      {
        s_pFactory->m_pMngr = new GetZipManager();
      }
      return;
    }
    SLOGD("netstack: Failed to find createGETZipManager sybmol in libgetzip.so");
    ::dlclose(s_pFactory->libHandle);
    s_pFactory->libHandle = NULL;
    s_pFactory->m_pMngr = new GetZipManager();
    return;
  }

  SLOGD("%s: Failed to construct GETzip manager, didn't find the library!",
      __FILE__);
  s_pFactory->m_pMngr = new GetZipManager();
}

IGetZipManager* HttpGetZipFactory::GetGETZipManager()
{
  return s_pFactory->m_pMngr;
}

void HttpGetZipFactory::StopGETZipManager()
{
  if (libHandle == NULL)
    return;

  delete m_pMngr;
  m_pMngr = new GetZipManager();
  ::dlclose(libHandle);
  libHandle = NULL;
}

IGetZipManager::IGetZipManager()
{
}

IGetZipManager::~IGetZipManager()
{
}

GetZipManager::GetZipManager()
{
}

}
; //end network
