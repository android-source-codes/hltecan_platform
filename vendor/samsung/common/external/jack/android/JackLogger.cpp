#define LOG_TAG "JackLogger"
#define LOG_NDEBUG 0

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <utils/Log.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "JackError.h"

#include <semaphore.h>

#include <utils/CallStack.h>

#include "JackLogger.h"

//using namespace android;

#define JACKLOG(fd,fp,...) \
    if(fd>0) { dprintf(fd,__VA_ARGS__); dprintf(fd,"\n"); } \
    else if(fp!=NULL) { fprintf(fp, __VA_ARGS__); fprintf(fp,"\n");} \
    else ALOGE(__VA_ARGS__);

#define DUMP_FILE_STRING_NUM 48
#define TIME_STRING_NUM 24


const char *_pcmWriteFilePath = "/data/snd/JackLogWritePcm_%s.pcm";
const char *_pcmReadFilePath = "/data/snd/JackLogReadPcm_%s.pcm";
const char *_saveDumpFilePath = "/data/snd/JackLogDump_%s.txt";
const char *_timeString = "%dm%dd_%dh%dm%ds";


namespace Jack {
    class JackLogger::_logAudioDriver;

    extern "C" void * Instance(){
        return JackLogger::getInstance();
    }
    
    JackLogger* JackLogger::mInstance = NULL;
    int JackLogger::mCalledCount = 0;
    JackLogger::_logAudioDriver JackLogger::logAudioDriver;
    
    void* JackLogger::getInstance(){
        if(mInstance == NULL){
            ALOGI("create Instance");
            mInstance = new JackLogger();
            memset(&logAudioDriver, 0, sizeof((_logAudioDriver)logAudioDriver));
        }
        mCalledCount++;
        return mInstance;
    }
    bool JackLogger::getCurrentTimeStr(char * dest){
        bool ret = false;
        struct timeval systemTimeHighRes;
        struct tm buffer;

        char timeStrFormat[TIME_STRING_NUM] = {0, };
        
        if(dest == NULL){
            ALOGE("getCurrentTimeStr destination address is null");
            return ret;
        }

        if (gettimeofday(&systemTimeHighRes, 0) == -1){
            ALOGE("getCurrentTimeStr can't get time");
            return ret;
        }

        const struct tm* st = localtime_r(&systemTimeHighRes.tv_sec, &buffer);
        if(st == NULL){
            ALOGE("getCurrentTimeStr can't get local time");
            return ret;
        }
        
        snprintf(dest, TIME_STRING_NUM - 1, _timeString, st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min, st->tm_sec);
        ALOGI("getCurrentTimeStr write file '%s', len %d", dest, strlen(dest));
        ret = true;
        return ret;
    }
    void * JackLogger::deleteInstance(){
        if(mInstance != NULL){
//            ALOGV("deleteInstance mCalledCount %d", mCalledCount);
            if(--mCalledCount <= 0) {
                char dumpFileName[DUMP_FILE_STRING_NUM] = {0, };
                char timeStr[TIME_STRING_NUM] = {0, };
                FILE *saveDumpFile = NULL;
                
                if(getCurrentTimeStr(timeStr)) {
                    snprintf(dumpFileName, DUMP_FILE_STRING_NUM - 1, _saveDumpFilePath, timeStr);
                    ALOGI("deleteInstance write file '%s', len %d", dumpFileName, strlen(dumpFileName));

                    saveDumpFile = fopen(dumpFileName, "wt");
                    if(saveDumpFile != NULL){
                        fputs("\n--- file dump start ---\n\n", saveDumpFile);
                        dumpLog(-1, saveDumpFile, LOG_BASE_ANDROIDSHM);
                        fputs("\n-----------------------\n", saveDumpFile);
                        dumpLog(-1, saveDumpFile, LOG_BASE_APASERVICE);
                        fputs("\n-----------------------\n", saveDumpFile);
                        dumpLog(-1, saveDumpFile, LOG_BASE_JACKSERVICE);
                        fputs("\n-----------------------\n", saveDumpFile);
                        dumpLog(-1, saveDumpFile, LOG_BASE_JACKD);
                        fputs("\n--- file dump end -----\n", saveDumpFile);
                        fclose(saveDumpFile);
                    }
                }
                ALOGV("delete static instance!!");
                delete mInstance;
                mInstance == NULL;
            }

        }
        return mInstance;
    }

    JackLogger::JackLogger(){
        ALOGI("JackLogger, calledCount %d", mCalledCount);
        mPcmDumpType = 0;
    }
    
    JackLogger::~JackLogger(){
        ALOGI("~JackLogger, calledCount %d", mCalledCount);
    }

    void JackLogger::prCount(){
        /*
        CallStack st;
        st.update(0,30);
        st.log("JackLogger",ANDROID_LOG_ERROR,"prCount");
        */
        ALOGE("[%d:%d] %p instance, prCount %d", getpid(), gettid(), mInstance, mCalledCount);
    }
    void JackLogger::setDriverDevices(const char * playback, const char * capture){
        if(mInstance != NULL){
            ALOGI("setDriverDevices playback [%s:%d], capture [%s:%d], addr [%p, %p]", playback, strlen(playback), capture, strlen(capture), &mInstance->logAudioDriver.mPlaybackDeviceId, &logAudioDriver.mPlaybackDeviceId);
            strncpy(mInstance->logAudioDriver.mPlaybackDeviceId, playback, strlen(playback));
            strncpy(mInstance->logAudioDriver.mCaptureDeviceId, capture, strlen(playback));
        }
    }
    void JackLogger::checkSystemTime(TypeLogTime type, SaveLogSequence seq){
        struct timeval systemTimeHighRes;
        struct tm buffer;

        ALOGI("checkSystemTime type %d, seq %d", type, seq);
        if(seq <= SAVE_LOG_INVALID || seq >= SAVE_LOG_MAX){
            ALOGE("checkSystemTime invalid sequence type %d", seq);
            return;
        }

        if(type < LOG_TIME_PRINT || type >= LOG_TIME_MAX){
            ALOGE("checkSystemTime invalid log time type %d", type);
            return;
        }

        if (gettimeofday(&systemTimeHighRes, 0) == -1){
            ALOGE("checkSystemTime can't get time");
            return;
        }

        const struct tm* st = localtime_r(&systemTimeHighRes.tv_sec, &buffer);
        if(st == NULL){
            ALOGE("checkSystemTime can't get local time");
            return;
        }
        char current_t[32] = {0, };
        const char *t_type = "%d-%d-%d %d:%d:%d:%u";

        snprintf(current_t, 31, t_type, st->tm_year + 1900, st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min, st->tm_sec, systemTimeHighRes.tv_usec / 1000);
        ALOGI("checkSystemTime '%s' len %d, instance %p, _log %p", current_t,strlen(current_t), mInstance, &logAudioDriver);
        switch(type){
            case LOG_TIME_PRINT:
                ALOGE("checkSystemTime print time '%s' len %d", current_t,strlen(current_t));
                break;
            case LOG_TIME_AD_CONNECT:
                strncpy(mInstance->logAudioDriver.tConnectDriver[seq], current_t, strlen(current_t));
                break;
            case LOG_TIME_AD_START:
                if(seq == SAVE_LOG_START && mInstance->logAudioDriver.cFirstProcess == 0)
                    mInstance->logAudioDriver.cFirstProcess = 1;                
                strncpy(mInstance->logAudioDriver.tStartDriver[seq], current_t, strlen(current_t));
                break;
            case LOG_TIME_AD_FIRSTPROCESS:
                if(seq == SAVE_LOG_END && mInstance->logAudioDriver.cFirstProcess == 1)
                    mInstance->logAudioDriver.cFirstProcess = 2;
                strncpy(mInstance->logAudioDriver.tFirstProcess[seq], current_t, strlen(current_t));                
                break;
            default:
                ALOGE("Invalid type %d", type);
                break;
        }
                
        
    }
    bool JackLogger::isFirstProcess(){
//        ALOGE("isFirstProcess cFirstProcess %d", mInstance->logAudioDriver.cFirstProcess);
        return (mInstance->logAudioDriver.cFirstProcess == 2) ? false : true;
    }
    void JackLogger::dumpLog(int fd, FILE *fp, LogBase LogType){
        ALOGI("dumpLog fd %d, fp %p, type %d", fd, fp, LogType);
        if(mInstance == NULL){
            ALOGE("dumpLog instance is null!");
            return;
        }

        switch(LogType){
            case LOG_BASE_JACKD:
                JACKLOG(fd, fp, "- Jackd Dump :");
                JACKLOG(fd, fp, "  : playback device id '%s'", mInstance->logAudioDriver.mPlaybackDeviceId)
                JACKLOG(fd, fp, "  : capture device id '%s'", mInstance->logAudioDriver.mCaptureDeviceId)               
                JACKLOG(fd, fp, "  : pcm connect try time '%s'", mInstance->logAudioDriver.tConnectDriver[0])
                JACKLOG(fd, fp, "  : pcm connect done time '%s'", logAudioDriver.tConnectDriver[1])
                JACKLOG(fd, fp, "  : pcm start try time '%s'", mInstance->logAudioDriver.tStartDriver[0])
                JACKLOG(fd, fp, "  : pcm start done time '%s'", mInstance->logAudioDriver.tStartDriver[1])
                JACKLOG(fd, fp, "  : pcm first processing try time '%s'", mInstance->logAudioDriver.tFirstProcess[0])
                JACKLOG(fd, fp, "  : pcm first processing done time '%s'", mInstance->logAudioDriver.tFirstProcess[1])
                break;
            case LOG_BASE_APASERVICE:
                JACKLOG(fd, fp, "- APA Service Dump :")
                break;
            case LOG_BASE_ANDROIDSHM:
                JACKLOG(fd, fp, "- Android Shm Dump :")
                break;
            case LOG_BASE_JACKSERVICE:
                JACKLOG(fd, fp, "- JackService Dump :")
                break;
            default:
                ALOGE("Invalid Log Type %d", LogType);
                break;

        }
    }
    
    void JackLogger::excuteCommand(LogCommand command, CommandParam param, char * pParam, int pParamSize){
        ALOGI("excuteParam command %d, param %d", command, param);
        switch(command){
            case LOG_COMMAND_PCM_DUMP:
                if(param == COMMAND_PARAM_PCM_DUMP_START){
                    if(createPcmDumpFile() != -1){
                        if(!startPcmDump((PcmDumpType)(JACK_PCM_DUMP_WRITE | JACK_PCM_DUMP_READ))){
                            ALOGE("excuteCommand start pcm dump fail");
                        }
                    }
                }
                if(param == COMMAND_PARAM_PCM_DUMP_STOP){
                    if(!isStartedPcmDump()){
                        ALOGE("excuteCommand pcm dump not started %d", mPcmDumpType);
                    }

                    stopPcmDump();
                    
                }
                
                break;
            case LOG_COMMAND_SET_PARAM:
                if(pParam != NULL){
                    
                }else{
                    ALOGE("set param source param is null");
                }
                break;
            case LOG_COMMAND_GET_PARAM:
                if(pParam != NULL){
                    
                }else{
                    ALOGE("get param source param is null");
                }                
                break;
            default:
                ALOGE("Invalid command %d", command);
                break;
        }
    }

    int JackLogger::createPcmDumpFile(){
        char timeStr[TIME_STRING_NUM] = {0, };
        char pcmWriteFileName[DUMP_FILE_STRING_NUM] = {0, };
        char pcmReadFileName[DUMP_FILE_STRING_NUM] = {0, };        
        
        if(getCurrentTimeStr(timeStr)){
            snprintf(pcmWriteFileName, DUMP_FILE_STRING_NUM - 1, _pcmWriteFilePath, timeStr);
            snprintf(pcmReadFileName, DUMP_FILE_STRING_NUM - 1, _pcmReadFilePath, timeStr);        
            ALOGI("createPcmDumpFile write file '%s', len %d", pcmWriteFileName, strlen(pcmWriteFileName));
            ALOGI("createPcmDumpFile read file '%s', len %d", pcmWriteFileName, strlen(pcmWriteFileName));        
            if(PcmWriteDump != NULL){
                ALOGE("createPcmDumpFile already use pcm dump file");
            }else{
                PcmWriteDump = fopen(pcmWriteFileName, "wb");
                if(PcmWriteDump == NULL){
                    ALOGE("createPcmDumpFile can't make '%s'", pcmWriteFileName);
                }
                
                PcmReadDump = fopen(pcmReadFileName, "wb");
                if(PcmReadDump == NULL){
                    ALOGE("createPcmDumpFile can't make '%s'", pcmReadFileName);
                }
            }
        }
        return 0;
    }
    
    bool JackLogger::startPcmDump(PcmDumpType type){
        bool ret = true;
        ALOGI("startPcmDump type %d, current type %d", type, mPcmDumpType);
        if(mPcmDumpType != JACK_PCM_DUMP_NONE){
            ALOGE("startPcmDump already used or previouse dump is not complte %d", mPcmDumpType);
            ret = false;
        }else{
            mPcmDumpType = type;
        }
        return ret;
    }
    
    bool JackLogger::stopPcmDump(){
        bool ret = true;
        int tDumpType = mPcmDumpType;
        ALOGI("stopPcmDump current type %d", mPcmDumpType);
        if(mPcmDumpType == JACK_PCM_DUMP_NONE){
            ALOGE("stopPcmDump dump is not stared!! %d", mPcmDumpType);
            ret = false;
        }
        mPcmDumpType = JACK_PCM_DUMP_NONE;
        
        usleep(25);
        
        if(tDumpType & JACK_PCM_DUMP_WRITE){
            if(PcmWriteDump){
                fclose(PcmWriteDump);
                PcmWriteDump = NULL;
            }else{
                ALOGE("stopPcmDump write pcm file is not opened");
            }
        }
        if(tDumpType & JACK_PCM_DUMP_READ){
            if(PcmReadDump){
                fclose(PcmReadDump);
                PcmReadDump = NULL;
            }else{
                ALOGE("stopPcmDump read pcm file is not opened");
            }
        }

        return ret;
    }
    
    bool JackLogger::isStartedPcmDump(){
        return (mPcmDumpType == JACK_PCM_DUMP_NONE) ? false : true;
    }
    
    void JackLogger::dumpWriteData(float *buffer, int nframes, int skip){
        if(PcmWriteDump){
            fwrite(buffer, sizeof(float) , nframes, PcmWriteDump);

        }
    }
    
    void JackLogger::dumpReadData(float *buffer, int nframes, int skip){
        if(PcmReadDump){
            fwrite(buffer, sizeof(float), nframes, PcmReadDump);
        }

    }
}



