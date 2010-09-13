
#include "mediastream.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cond=1;

void stop_handler(int signum)
{
	cond--;
	if (cond<0) exit(-1);
}

int main(int argc, char * argv[])
{
	AudioStream *audio;

	ortp_init();
	rtp_profile_set_payload(&av_profile, 110, &speex_wb);
	rtp_profile_set_payload(&av_profile,102,&payload_type_ilbc);
	
	ms_init();
	ms_speex_codec_init();
	ms_ilbc_codec_init();

	signal(SIGINT, stop_handler);

	audio = audio_stream_start(&av_profile, 2000, "127.0.0.1", 2000, 110, 250);
	
	for (;;) sleep(1);
	
	audio_stream_stop(audio);
}
