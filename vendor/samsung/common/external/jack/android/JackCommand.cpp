#define LOG_TAG "JackCommand"
#define LOG_NDEBUG 0

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <IAPAService.h>
#include <IJackService.h>

#include <JackCommand.h>

using namespace android;

void usage()
{
    fprintf (stderr, "\n\n"
             "usage: jackc command [option]\n"
             "             command : dumppcm, dumplog\n"
             "                - dumppcm : dump read and write pcm of alsa driver \n"
             "                   : option - seconds (default dump seconds is 5 second)\n"
             "                   : ex) jackc dumppcm 15\n"
             "                   : ex) jackc dumppcm\n\n"
             "                - dumplog : dump each jack releative service's information\n"
             "                   : option - jackservice, jackd, alsaservice, all\n"
             "                   : ex) jackc dumplog jackd\n"
             "                   : ex) jackc dumplog jackservice\n"
             "                   : ex) jackc dumplog all\n\n"
            );
    exit(1);
}

int
main (int argc, char *argv[])
{

    ALOGI("main argc %d, argv '%s'", argc, argv[0]);

	if (argc >= 2) {
        if(!strcmp("dumplog", argv[1])){
            if(argc >= 3){
                sp<IJackService> iJackService = interface_cast<IJackService>(defaultServiceManager()->checkService(String16("com.samsung.android.jam.IJackService")));                
                sp<IAPAService> iAPAService = interface_cast<IAPAService>(defaultServiceManager()->checkService(String16("com.samsung.android.jam.IAPAService")));                
                if(!strcmp("jackservice", argv[2])){
                    if(iJackService != NULL){
                        iJackService->sendCommand("dump_logs");
                    }else{
                        ALOGE("Can't get iJackService");
                    }
                } else if(!strcmp("apaservice", argv[2])){
                    if(iAPAService != NULL){
                        iAPAService->sendCommand("mJackLogger!dump_logs");
                    }
                } else if(!strcmp("androidshm", argv[2])){

                } else if(!strcmp("jackd", argv[2])){
                    if(iJackService != NULL){
                        iJackService->sendCommand("dump_logs_jackd");                        
                    }else{
                        ALOGE("Can't get iJackService");
                    }
                } else if(!strcmp("all", argv[2])){
                    if(iJackService != NULL){
                        iJackService->sendCommand("dump_logs");
                        iJackService->sendCommand("dump_logs_jackd");                        
                    }else{
                        ALOGE("Can't get iJackService");
                    }
                    if(iAPAService != NULL){
                        iAPAService->sendCommand("mJackLogger!dump_logs");
                    }
                }
            }
            else{
                ALOGE("dump log need more argument, check usage");
                usage();
            }
        }
        else if(!strcmp("dumppcm", argv[1])){
            int dumpTime = 5;

            if(argc >= 3){
                int arTime = atoi(argv[2]);
                if(arTime > 600 || arTime < 0){
                    ALOGE("invalid dump time %d", arTime);
                }else{
                    dumpTime = arTime;
                }
            }
            
            sp<IJackService> iJackService = interface_cast<IJackService>(defaultServiceManager()->checkService(String16("com.samsung.android.jam.IJackService")));
            if(iJackService != NULL){
                int sleepSeq = dumpTime / 5;
                int sleepRemain = dumpTime % 5;
                ALOGI("jackc start pcm dump %d sec [%d:%d]", dumpTime, sleepSeq, sleepRemain);
                iJackService->sendCommand("pcm_dump");
                while(sleepSeq--){
                    ALOGE("jackc getting pcm dump");
                    usleep(5 * 1000000);
                }
                ALOGI("jackc getting pcm dump");
                usleep(sleepRemain * 1000000);
                iJackService->sendCommand("pcm_dump");
                ALOGI("jackc getting pcm dump done!");
            }else{
                ALOGE("Can't get iJackService");
            }
        }
        else if(!strcmp("setdebug", argv[1])){
    	    if(argc >= 3){
                int level = atoi(argv[2]);
                if(level < 0 || level > 2){
                    ALOGE("invaild debug level");
                }else{
                    char command_debug[16];
                    sp<IJackService> iJackService = interface_cast<IJackService>(defaultServiceManager()->checkService(String16("com.samsung.android.jam.IJackService")));
                    snprintf(command_debug, 15, "set_vervose=%d", level);
                    if(iJackService != NULL)
                        iJackService->sendCommand(command_debug);
                }
            }else{
                ALOGE("setdebug need addtional debug level, check usage()");
                usage();    
            }
    	} 
	}
    else {
	    ALOGE("no addtional option. check below usage");
        usage();
	}

	exit (0);
}

