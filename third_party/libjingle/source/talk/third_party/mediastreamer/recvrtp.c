#include "msrtprecv.h"
#include "ms.h"
#include "mswrite.h"
#include "msosswrite.h"
#include "msMUlawdec.h"
#include "mstimer.h"
#include "msfdispatcher.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

static int cond=1;

void usage(){
    printf(
            "\nUsage: \ntest_rtprecv <output_file> <address> <port>\n"
            "<output_file> is a file to log the flow\n"
            "<address> is a local ip address to listen\n"
            "<port> is a local udp port to listen\n"
        );

}

void stop_handler(int signum){
    cond=0;
}

int main(int argc, char *argv[]){
    MSFilter *play,*dec,*rec;
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
           "Receiving RTP flow with oRTP\n"
           "#################################\n"
          );
    /*create the rtp session */
    ortp_init();
    ortp_set_debug_file("oRTP",NULL);
    rtps=rtp_session_new(RTP_SESSION_RECVONLY);
    rtp_session_set_local_addr(rtps,argv[2],port);
    rtp_session_set_scheduling_mode(rtps,0);
    rtp_session_set_blocking_mode(rtps,0);

    printf(
        "##########################################################################\n"
        "Inicialized to listen to the %s (local address) in the port %d\n"
        "##########################################################################\n", argv[2], port);
    ms_init();
    signal(SIGINT,stop_handler);
   
    play=ms_rtp_recv_new();
    rec=ms_oss_write_new();
    ms_sound_write_set_device(MS_SOUND_WRITE(rec),0);
    dec=ms_MULAWdecoder_new();
    timer=ms_timer_new();

    ms_rtp_recv_set_session(MS_RTP_RECV(play),rtps);
   
    ms_filter_add_link(play,dec);
    ms_filter_add_link(dec,rec);
    ms_sync_attach(timer,play);
    printf(
        "############\n"
        "gran=%i\n"
        "############\n",MS_SYNC(timer)->samples_per_tick);
   
    ms_start(timer);
    ms_sound_write_start(MS_SOUND_WRITE(rec));
    while(cond)
    {
        sleep(1);
    }
   
    printf(
        "#################################\n"
        "stoping sync...\n"
        "#################################\n");
    ms_stop(timer);
    ms_sound_write_stop(MS_SOUND_WRITE(rec));
    printf(
        "#################################\n"
        "unlinking filters...\n"
        "#################################\n");
    ms_filter_remove_links(play,dec);
    ms_filter_remove_links(dec,rec);
    printf( "#################################\n"
            "destroying filters...\n"
            "#################################\n");
    ms_filter_destroy(play);
    ms_filter_destroy(dec);
    ms_filter_destroy(rec);
   
    rtp_session_destroy(rtps);
    ms_sync_destroy(timer);
    ortp_global_stats_display();
   
    return 0;
}
