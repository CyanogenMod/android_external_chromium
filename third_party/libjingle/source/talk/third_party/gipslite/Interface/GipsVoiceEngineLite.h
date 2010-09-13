// GipsVoiceEngineLib.h
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// 
//  Created by: Fredrik Galschiödt
//  Date      : 011202
//
//  $Change$
//
//	Public API for GIPS Voice Engine on a PC platform.
// 
//  Copyright (c) 2001
//  Global IP Sound AB, Organization number: 5565739017
//  Rosenlundsgatan 54, SE-118 63 Stockholm, Sweden
//  All rights reserved.
//  
//////////////////////////////////////////////////////////////////////

#ifndef PUBLIC_GIPS_VOICE_ENGINE_LITE_H
#define PUBLIC_GIPS_VOICE_ENGINE_LITE_H

#include "GIPS_common_types.h"

#ifdef GIPS_EXPORT
#define VOICEENGINE_DLLEXPORT _declspec(dllexport)
#elif GIPS_DLL
#define VOICEENGINE_DLLEXPORT _declspec(dllimport)
#else
#define VOICEENGINE_DLLEXPORT
#endif


//////////////////////////////////////////////////////////////////////
// GipsVoiceEngineLib
//
// Public interface to the GIPS Voice Engine for PC platforms
//////////////////////////////////////////////////////////////////////

#ifndef NULL
#define NULL 0L
#endif

class VOICEENGINE_DLLEXPORT GipsVoiceEngineLite
{
public:
	virtual int GIPSVE_Init(bool recordAEC =false, bool multiCore = false,int month = 0,int day = 0,int year = 0 ) = 0;
	virtual int GIPSVE_SetNetworkStatus(int networktype) = 0;
	virtual int GIPSVE_GetNetworkStatus() = 0;
	virtual int GIPSVE_CreateChannel() = 0;
	virtual int GIPSVE_DeleteChannel(int channel) = 0;
	virtual int GIPSVE_GetCodec(short listnr, GIPS_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_GetNofCodecs() = 0;
	virtual int GIPSVE_SetSendCodec(int channel, GIPS_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_GetCurrentSendCodec(short channel, GIPS_CodecInst *gipsve_inst) = 0;
	virtual int GIPSVE_GetRecCodec(int channel, GIPS_CodecInst *recCodec) = 0;
	virtual int GIPSVE_SetRecPort(int channel, int portnr, char * multiCastAddr = NULL, char * ip = NULL) = 0;
	virtual int GIPSVE_GetRecPort(int channel) = 0;
	virtual int GIPSVE_SetSendPort(int channel, int portnr) = 0;
	virtual int GIPSVE_SetSrcPort(int channel, int portnr) = 0;
	virtual int GIPSVE_GetSendPort(int channel) = 0;
	virtual int GIPSVE_SetSendIP(int channel, char *ipadr) = 0;
	virtual int GIPSVE_GetSendIP(int channel, char *ipadr, int bufsize) = 0;
	virtual int GIPSVE_StartListen(int channel) = 0;
	virtual int GIPSVE_StartPlayout(int channel) = 0;
	virtual int GIPSVE_StartSend(int channel) = 0;
	virtual int GIPSVE_StopListen(int channel) = 0;
	virtual int GIPSVE_StopPlayout(int channel) = 0;
	virtual int GIPSVE_StopSend(int channel) = 0;
	virtual int GIPSVE_GetLastError() = 0;
	virtual int GIPSVE_SetSpeakerVolume(unsigned int level) = 0;
	virtual int GIPSVE_GetSpeakerVolume() = 0;
	virtual int GIPSVE_SetMicVolume(unsigned int level) = 0;
	virtual int GIPSVE_GetMicVolume() = 0;
	virtual int GIPSVE_SetAGCStatus(int mode) = 0;
	virtual int GIPSVE_GetAGCStatus() = 0;
	virtual int GIPSVE_GetVersion(char *version, int buflen) = 0;
	virtual int GIPSVE_Terminate() = 0;
	virtual int GIPSVE_SetDTMFPayloadType(int channel, int payloadType) = 0;
	virtual int GIPSVE_SendDTMF(int channel, int eventnr, int inBand) = 0;
	virtual int GIPSVE_PlayDTMFTone(int eventnr) = 0;	
	virtual unsigned short GIPSVE_GetFromPort(int channel)=0;
	virtual int GIPSVE_SetFilterPort(int channel,unsigned short filter) = 0;
	virtual int GIPSVE_SetFilterIP(int channel,char *IPaddress) = 0;
	virtual unsigned short GIPSVE_GetFilterPort(int channel) = 0;
	virtual int GIPSVE_SetRecPayloadType(short channel, GIPS_CodecInst *codec_inst)=0;
	virtual int GIPSVE_RTCPStat(int channel, unsigned short *fraction_lost, unsigned long *cum_lost, unsigned long *ext_max, unsigned long *jitter, int *RTT)=0;
	virtual int GIPSVE_SetSoundDevices(unsigned int WaveInDevice, unsigned int WaveOutDevice, bool disableMicBoost = false)= 0;
	virtual int GIPSVE_SetDTMFFeedbackStatus(int mode) = 0;
	virtual int GIPSVE_GetDTMFFeedbackStatus() = 0;

	virtual int GIPSVE_GetNoOfChannels() = 0;
	virtual int GIPSVE_GetInputLevel() = 0;
	virtual int GIPSVE_GetOutputLevel(int channel = -1) = 0;
	virtual int GIPSVE_MuteMic(int channel,int Mute) = 0;
	virtual int GIPSVE_PutOnHold(int channel,bool enable) = 0;
	virtual int GIPSVE_AddToConference(int channel,bool enable, bool includeCSRCs = false, bool includeVoiceLevel = false) = 0;

	virtual int GIPSVE_CheckIfAudioIsAvailable(int checkPlay, int checkRec) = 0;
	
	// RTCP calls
	virtual int GIPSVE_EnableRTCP(int channel, int enable) = 0;
	virtual int GIPSVE_SetRTCPCNAME(int channel, char * str) = 0;
	virtual int GIPSVE_getRemoteRTCPCNAME(int channel, char * str) = 0;

	virtual int GIPSVE_SetPacketTimeout(int channel, bool enable, int time_sec) = 0;

	// Send extra packet over RTP / RTCP channel (no RTP headers added)
	virtual int sendExtraPacket_RTP(int channel, unsigned char* data, int nbytes) = 0;
	virtual int sendExtraPacket_RTCP(int channel, unsigned char* data, int nbytes) = 0;

	// Voice Activity
	virtual int GIPSVE_GetVoiceActivityIndicator(int channel) = 0;

	// Use these function calls ONLY when a customer specific transport protocol is going to be used
	virtual int GIPSVE_SetSendTransport(int channel, GIPS_transport &transport) = 0;
	virtual int GIPSVE_ReceivedRTPPacket(int channel, const void *data, int len) = 0;
	virtual int GIPSVE_ReceivedRTCPPacket(int channel, const void *data, int len) = 0;
	
	virtual ~GipsVoiceEngineLite();
};

//////////////////////////////////////////////////////////////////////
// Factory method
//////////////////////////////////////////////////////////////////////

VOICEENGINE_DLLEXPORT GipsVoiceEngineLite &GetGipsVoiceEngineLite();



#endif // PUBLIC_GIPS_VOICE_ENGINE_LIB_H
