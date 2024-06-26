#ifndef JACKLOGGER_H
#define JACKLOGGER_H

#include <binder/Parcel.h>

namespace Jack {
    
    class JackLogger 
    {
        private:
        
            static JackLogger * mInstance;
            static int mCalledCount;

            JackLogger();

            FILE *PcmWriteDump;
            FILE *PcmReadDump;            
            int mPcmDumpType;

            bool getCurrentTimeStr(char * dest);
        public:
            class _logAudioDriver {
                public:
                    char mUsingDriver[16];
                    char mPlaybackDeviceId[8];
                    char mCaptureDeviceId[8];
                    char tConnectDriver[2][32];
                    char tStartDriver[2][32];
                    char tFirstProcess[2][32];
                    int cFirstProcess;
            };
            
            typedef enum LogBase{
                LOG_BASE_JACKD = 0,
                LOG_BASE_APASERVICE,
                LOG_BASE_ANDROIDSHM,
                LOG_BASE_JACKSERVICE,
                LOG_BASE_MAX
            } LogBase;

            typedef enum TypeLogTime{
                LOG_TIME_PRINT = 0,
                LOG_TIME_AD_CONNECT,
                LOG_TIME_AD_START,
                LOG_TIME_AD_FIRSTPROCESS,
                LOG_TIME_MAX
            } TypeLogTime;

            typedef enum SaveLogSequence{
                SAVE_LOG_INVALID = -1,
                SAVE_LOG_START,
                SAVE_LOG_END,
                SAVE_LOG_MAX
            } SaveLogSequence;

            typedef enum LogCommand{
                LOG_COMMAND_PCM_DUMP = 0,
                LOG_COMMAND_SET_PARAM,
                LOG_COMMAND_GET_PARAM,
                LOG_COMMAND_MAX
            } LogCommand;

            typedef enum CommandParam{
                COMMAND_PARAM_PCM_DUMP_START = 0,
                COMMAND_PARAM_PCM_DUMP_STOP,
                COMMAND_PARAM_SET_PARAM,
                COMMAND_PARAM_GET_PARAM
            } CommandParam;

            typedef enum PcmDumpType{
                JACK_PCM_DUMP_NONE = 0,
                JACK_PCM_DUMP_WRITE,
                JACK_PCM_DUMP_READ
            } PcmDumpType;
            
            static _logAudioDriver logAudioDriver;
            
            ~JackLogger();
            static void * getInstance();
            void * deleteInstance();
            void prCount();
            void setDriverDevices(const char * playback, const char * capture);
            void checkSystemTime(TypeLogTime type, SaveLogSequence seq);
            bool isFirstProcess();
            void dumpLog(int fd, FILE * fp, LogBase LogType);
            void excuteCommand(LogCommand command, CommandParam param, char * pParam = NULL, int pParamSize = 0);
            int createPcmDumpFile();            
            bool startPcmDump(PcmDumpType type);
            bool stopPcmDump();            
            bool isStartedPcmDump();
            void dumpWriteData(float *buffer, int nframes, int skip);
            void dumpReadData(float *buffer, int nframes, int skip);            
            
    };
}
#endif

