#include "msrtpsend.h"
#include "ms.h"
#include "msread.h"
#include "msossread.h"
#include "msMUlawenc.h"
#include "mstimer.h"
#include "msfdispatcher.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

static int cond=1;

void usage(){
    printf(
            "\nUsage: \ntest_rtpsend <output_file> <address> <port>\n"
            "<output_file> is a file to log the flow\n"
            "<address> is a remote ip address to listen\n"
            "<port> is a remote udp port to listen\n"
        );

}

void stop_handler(int signum){
    cond=0;
}

int main(int argc, char *argv[]){
    MSFilter *sender,*enc,*mic;
    MSSync *timer;
    RtpSession *rtps;
    int port;
   
    if (argc < 4){
        usage();
        exit(0);
    }
    port = atoi(argv[3]);

    printf("#################################\n"
           "Test Program\n"
           "Sending RTP flow with oRTP\n"
           "#################################\n"
          );
    /*create the rtp session */
    ortp_init();
    ortp_set_debug_file("oRTP",NULL);
    rtps=rtp_session_new(RTP_SESSION_SENDONLY);
    rtp_session_set_remote_addr(rtps,argv[2],port);
    rtp_session_set_scheduling_mode(rtps,0);
    rtp_session_set_blocking_mode(rtps,0);

    printf(
        "##########################################################################\n"
        "Inicialized to write on the %s (remote address) in the port %d\n"
        "##########################################################################\n", argv[2], port);
    ms_init();
    signal(SIGINT,stop_handler);
   
    sender=(MSFilter*)ms_rtp_send_new();
    mic=(MSFilter*)ms_oss_read_new();
    ms_sound_read_set_device(MS_SOUND_READ(mic),0);
    enc=(MSFilter*)ms_MULAWencoder_new();

    timer=ms_timer_new();

    ms_rtp_send_set_session(MS_RTP_SEND(sender),rtps);
   
    ms_filter_add_link(mic,enc);
    ms_filter_add_link(enc,sender);
    ms_sync_attach(timer,mic);
    printf(
        "############\n"
        "gran=%i\n"
        "############\n",MS_SYNC(timer)->samples_per_tick);
   
    ms_start(timer);
    ms_sound_read_start(MS_SOUND_READ(mic));
    while(cond)
    {
        sleep(1);
    }
   
    printf(
        "#################################\n"
        "stoping sync...\n"
        "#################################\n");
    ms_stop(timer);
    ms_sound_read_stop(MS_SOUND_READ(mic));
    printf(
        "#################################\n"
        "unlinking filters...\n"
        "#################################\n");
    ms_filter_remove_links(enc,sender);
    ms_filter_remove_links(mic,enc);
    printf( "#################################\n"
            "destroying filters...\n"
            "#################################\n");
    ms_filter_destroy(sender);
    ms_filter_destroy(enc);
    ms_filter_destroy(mic);
   
    rtp_session_destroy(rtps);
    ms_sync_destroy(timer);
    ortp_global_stats_display();
   
    return 0;
}
