/*
 * Copyright (C) 2014 EPFL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

/**
 * @file ezp.cpp
 * @brief Simple performance analyzer
 * @author Ayberk Özgür
 * @date 2014-10-15
 */

#include"ezp.hpp"

namespace ezp{

///////////////////////////////////////////////////////////////////////////////
//Platform-dependent defs
///////////////////////////////////////////////////////////////////////////////

#define EZP_CLOCK CLOCK_THREAD_CPUTIME_ID
#ifdef ANDROID
#define EZP_GET_TID gettid()
#else
#define EZP_GET_TID syscall(SYS_gettid)
#endif

#ifdef ANDROID
#define EZP_PRINT(...) __android_log_print(ANDROID_LOG_INFO, ezp::EasyPerformanceAnalyzer::androidTag, __VA_ARGS__)
#define EZP_PERR(...) (ezp::EasyPerformanceAnalyzer::forceStderr ? fprintf(stderr,__VA_ARGS__) :__android_log_print(ANDROID_LOG_ERROR, ezp::EasyPerformanceAnalyzer::androidTag, __VA_ARGS__))
#else
#define EZP_PRINT(...) printf(__VA_ARGS__)
#define EZP_PERR(...) fprintf(stderr,__VA_ARGS__)
#endif

///////////////////////////////////////////////////////////////////////////////
//Static members
///////////////////////////////////////////////////////////////////////////////

const char* EasyPerformanceAnalyzer::androidTag = "EZP";
const char* EasyPerformanceAnalyzer::cmdSocketName = "\0ezp_control";

pthread_t EasyPerformanceAnalyzer::cmdListener;

bool EasyPerformanceAnalyzer::listenerRunning = false;
bool EasyPerformanceAnalyzer::enabled = false;
bool EasyPerformanceAnalyzer::forceStderr = false;

Blk2Clk EasyPerformanceAnalyzer::blocks(BlockKey::compare);
Blk2SMarker EasyPerformanceAnalyzer::smoothBlocks(BlockKey::compare);
Blk2AMarker EasyPerformanceAnalyzer::offlineBlocks(BlockKey::compare);

pthread_mutex_t EasyPerformanceAnalyzer::listenerLauncherLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t EasyPerformanceAnalyzer::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t EasyPerformanceAnalyzer::smoothLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t EasyPerformanceAnalyzer::offlineLock = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////

//This function is not time critical
void EasyPerformanceAnalyzer::controlRemote(Command cmd)
{
    struct sockaddr_un addr;
    int fd;

    if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        EZP_PERR("EZP: socket() error: %s\n",strerror(errno));
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, cmdSocketName, sizeof(addr.sun_path) - 1);

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        EZP_PERR("EZP: connect() error: %s\n",strerror(errno));
        EZP_PERR("EZP: Make sure that there is an EZP instrumented code running on this machine.\n");
        EZP_PERR("EZP: Control listener does not start until the first EZP_START* or EZP_BEGIN_CONTROL is encountered!\n");
        return;
    }

    int ret;
    switch(cmd){
        case CMD_ENABLE:
            ret = write(fd,"e",2);
            break;
        case CMD_DISABLE:
            ret = write(fd,"d",2);
            break;
        case CMD_PRINT:
            ret = write(fd,"p",2);
            break;
        case CMD_CLEAR:
            ret = write(fd,"c",2);
            break;
    }
    if(ret < 0)
        EZP_PERR("EZP: write() error: %s\n",strerror(errno));
    else if(ret < 2)
        EZP_PERR("EZP: write() error: Could not write command completely\n");

    close(fd);
}

//This function is not time critical
void EasyPerformanceAnalyzer::control(Command cmd)
{
    switch(cmd){
        case CMD_ENABLE:
            enabled = true;
            EZP_PRINT("EZP: Enabled local instrumentation.\n");
            break;
        case CMD_DISABLE:
            enabled = false;
            EZP_PRINT("EZP: Disabled local instrumentation.\n");
            break;
        case CMD_PRINT:
            printOfflineProfiles();
            break;
        case CMD_CLEAR:
            clearOfflineProfiles();
            break;
    }
}

//This function is time critical!
void EasyPerformanceAnalyzer::startProfiling(const char* blockName)
{
    //Record begin time even if not enabled to ensure mid-block enabling works

    if(!listenerRunning)
        launchCmdListener();

    Timespec* begin = new Timespec();
    BlkClkPair pair(BlockKey(EZP_GET_TID, hashStr(blockName)), begin);

    pthread_mutex_lock(&lock);
    std::pair<Blk2Clk::iterator, bool> result = blocks.insert(pair);

    //We did not find the marker from before, so we inserted the new one
    if(result.second){
        pthread_mutex_unlock(&lock);

        //Get time in the very end to disturb the measurements the least possible
        clock_gettime(EZP_CLOCK,begin);
    }

    //We found a marker from before, so we didn't insert the new one
    else{
        delete begin;
        Timespec* target = result.first->second;
        pthread_mutex_unlock(&lock);

        //Get time in the very end to disturb the measurements the least possible
        clock_gettime(EZP_CLOCK,target);
    }
}

//This function is time critical!
void EasyPerformanceAnalyzer::endProfiling(const char* blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK, &end);

    if(!enabled)
        return;

    TID tid = EZP_GET_TID;
    BlockKey key(tid, hashStr(blockName));

    pthread_mutex_lock(&lock);
    Blk2Clk::iterator pairIt = blocks.find(key);
    if(pairIt == blocks.end()){
        pthread_mutex_unlock(&lock);

        EZP_PERR("EZP: Can't find %s, did you call EZP_START(\"%s\")?\n", blockName, blockName);
    }
    else{
        Timespec* begin = pairIt->second;
        pthread_mutex_unlock(&lock);

        EZP_PRINT("EZP: [%d]\t%s\t%6.2f ms\n", tid, blockName, getTimeDiff(begin,&end));
    }
}

//This function is time critical!
void EasyPerformanceAnalyzer::startProfilingSmooth(const char* blockName)
{
    //Record begin time even if not enabled to ensure mid-block enabling works

    if(!listenerRunning)
        launchCmdListener();

    SmoothMarker* begin = new SmoothMarker();

    Blk2SMarkerPair pair(BlockKey(EZP_GET_TID, hashStr(blockName)), begin);

    pthread_mutex_lock(&smoothLock);
    std::pair<Blk2SMarker::iterator, bool> result = smoothBlocks.insert(pair);

    //We did not find the marker from before, so we inserted the new one
    if(result.second){
        pthread_mutex_unlock(&smoothLock);

        //Get time in the very end to disturb the measurements the least possible
        clock_gettime(EZP_CLOCK,&(begin->beginTime));
    }

    //We found a marker from before, so we didn't insert the new one
    else{
        delete begin;
        SmoothMarker* target = result.first->second;
        pthread_mutex_unlock(&smoothLock);

        //Get time in the very end to disturb the measurements the least possible
        clock_gettime(EZP_CLOCK,&(target->beginTime));
    }
}

//This function is time critical!
void EasyPerformanceAnalyzer::endProfilingSmooth(const char* blockName, float sf)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);

    if(!enabled)
        return;

    TID tid = EZP_GET_TID;
    BlockKey key(tid, hashStr(blockName));

    pthread_mutex_lock(&smoothLock);
    Blk2SMarker::iterator pairIt = smoothBlocks.find(key);
    if(pairIt == smoothBlocks.end()){
        pthread_mutex_unlock(&smoothLock);

        EZP_PERR("EZP: Can't find %s, did you call EZP_START_SMOOTH(\"%s\")?\n", blockName, blockName);
    }
    else{
        float slice = sf*pairIt->second->lastSlice + (1.0f - sf)*getTimeDiff(&(pairIt->second->beginTime),&end);
        pairIt->second->lastSlice = slice;
        pthread_mutex_unlock(&smoothLock);

        EZP_PRINT("EZP: [%d]\t%s\t~%6.2f ms\n", tid, blockName, slice);
    }
}

//This function is time critical!
void EasyPerformanceAnalyzer::startProfilingOffline(const char* blockName)
{
    //Record begin time even if not enabled to ensure mid-block enabling works

    if(!listenerRunning)
        launchCmdListener();

    AggregateMarker* begin = new AggregateMarker();

    Blk2AMarkerPair pair(BlockKey(EZP_GET_TID, hashStr(blockName)), begin);

    pthread_mutex_lock(&offlineLock);
    std::pair<Blk2AMarker::iterator, bool> result = offlineBlocks.insert(pair);

    //We did not find the marker from before, so we inserted a new one
    if(result.second){
        pthread_mutex_unlock(&offlineLock);

        //Get time in the very end to disturb the measurements the least possible
        clock_gettime(EZP_CLOCK,&(begin->beginTime));
    }

    //We found a marker from before, so we didn't insert the new one
    else{
        delete begin;
        AggregateMarker* target = result.first->second;
        pthread_mutex_unlock(&offlineLock);

        //Get time in the very end to disturb the measurements the least possible
        clock_gettime(EZP_CLOCK,&(target->beginTime));
    }
}

//This function is time critical!
void EasyPerformanceAnalyzer::endProfilingOffline(const char* blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);

    if(!enabled)
        return;

    BlockKey key(EZP_GET_TID, hashStr(blockName));

    pthread_mutex_lock(&offlineLock);
    Blk2AMarker::iterator pairIt = offlineBlocks.find(key);
    if(pairIt == offlineBlocks.end()){
        pthread_mutex_unlock(&offlineLock);

        EZP_PERR("EZP: Can't find %s, did you call EZP_START_OFFLINE(\"%s\")?\n", blockName, blockName);
    }
    else{
        pairIt->second->numSamples++;
        pairIt->second->totalTime += getTimeDiff(&(pairIt->second->beginTime), &end);
        pthread_mutex_unlock(&offlineLock);
    }
}

//This function is not time critical
void EasyPerformanceAnalyzer::printOfflineProfiles()
{
    pthread_mutex_lock(&offlineLock);
    if(offlineBlocks.size() == 0){
        EZP_PERR("EZP: No offline block found; instrument some code first by wrapping it with EZP_START_OFFLINE() ... EZP_END_OFFLINE()\n");
        pthread_mutex_unlock(&offlineLock);
        return;
    }

    //Transfer offline profiles into sortable data structure and find max block name length
    std::vector<AggregateProfile> sortedProfiles(offlineBlocks.size());
    Blk2AMarker::iterator its = offlineBlocks.begin();
    std::vector<AggregateProfile>::iterator itt = sortedProfiles.begin();
    for(; its != offlineBlocks.end(); its++, itt++){
        itt->tid = its->first.tid;
        itt->blockName = its->first.blockName;
        itt->numSamples = its->second->numSamples;
        itt->averageTime = its->second->totalTime/itt->numSamples;
        if(isnanf(itt->averageTime))
            itt->averageTime = -1;
    }
    pthread_mutex_unlock(&offlineLock);

    //Sort for printing according to average time taken
    std::sort(sortedProfiles.begin(),sortedProfiles.end(),AggregateProfile::compareAvgTime);

    //Do the thread-wise printing
    EZP_PRINT("EZP: ===============================================================================\n");
    EZP_PRINT("EZP: Thread-wise analysis results\n");
    EZP_PRINT("EZP: -------------------------------------------------------------------------------\n");
    EZP_PRINT("EZP: Thread ID    Name    Average(ms)         Total(ms)           Calls\n");
    EZP_PRINT("EZP: -------------------------------------------------------------------------------\n");
    char cbuf[5];
    for(std::vector<AggregateProfile>::iterator it = sortedProfiles.begin(); it != sortedProfiles.end(); it++){
        unhashStr(it->blockName,cbuf);
        if(it->numSamples == 0)
            EZP_PRINT("EZP: %9d    %4s    EZP_END_OFFLINE(\"%s\") was not present or was not enabled\n",
                    it->tid, cbuf, cbuf);
        else
            EZP_PRINT("EZP: %9d    %4s    %-16.2f    %-16.2f    %-10d\n",
                    it->tid, cbuf, it->averageTime, it->averageTime*it->numSamples, it->numSamples);
    }

    //Sort according to block name for summing
    std::sort(sortedProfiles.begin(),sortedProfiles.end(),AggregateProfile::compareBlockName);

    //Sum profiles coming from different threads
    std::vector<SummedProfile> totalProfiles;
    std::vector<AggregateProfile>::iterator itS = sortedProfiles.begin();
    totalProfiles.push_back(SummedProfile(itS->blockName, itS->averageTime*itS->numSamples, itS->numSamples));
    itS++;
    for(;itS!=sortedProfiles.end();itS++)
        if(itS->blockName == totalProfiles.back().blockName){
            totalProfiles.back().totalTime += itS->averageTime*itS->numSamples;
            totalProfiles.back().numSamples += itS->numSamples;
        }
        else
            totalProfiles.push_back(SummedProfile(itS->blockName, itS->averageTime*itS->numSamples, itS->numSamples));

    //Sort summed profiles according to average time
    std::sort(totalProfiles.begin(),totalProfiles.end(),SummedProfile::compare);

    //Print summed profiles
    EZP_PRINT("EZP: ===============================================================================\n");
    EZP_PRINT("EZP: Anaylsis results summed across threads\n");
    EZP_PRINT("EZP: -------------------------------------------------------------------------------\n");
    EZP_PRINT("EZP: Name    Average(ms)         Total(ms)           Calls\n");
    EZP_PRINT("EZP: -------------------------------------------------------------------------------\n");
    for(std::vector<SummedProfile>::iterator it = totalProfiles.begin(); it != totalProfiles.end(); it++){
        unhashStr(it->blockName,cbuf);
        if(it->numSamples == 0)
            EZP_PRINT("EZP: %4s    EZP_END_OFFLINE(\"%s\") was not present or was not enabled\n",
                    cbuf, cbuf);
        else
            EZP_PRINT("EZP: %4s    %-16.2f    %-16.2f    %-10d\n",
                    cbuf, it->totalTime/it->numSamples, it->totalTime, it->numSamples);
    }
    EZP_PRINT("EZP: ===============================================================================\n");
}

//This function is not time critical
void EasyPerformanceAnalyzer::clearOfflineProfiles()
{
    //We will tolerate the memory leak caused by not freeing the one AggregateMarker per instrumentation block
    //In order to free them, pthread lock must be held until after the clock measurement to check for null pointer in endProfilingOffline()
    //The disturbance in the measurements is not worth fixing the memory leak

    pthread_mutex_lock(&offlineLock);
    offlineBlocks.clear();
    pthread_mutex_unlock(&offlineLock);
}

//This function is time critical!
inline float EasyPerformanceAnalyzer::getTimeDiff(const Timespec* begin, const Timespec* end)
{
    return (float)(end->tv_sec - begin->tv_sec)*1000.0f + (float)(end->tv_nsec - begin->tv_nsec)/1000000.0f;
}

//This function is time critical!
inline unsigned int EasyPerformanceAnalyzer::hashStr(const char* str)
{
    unsigned int result = 0;

    result += str[0];
    if(str[1] == '\0')
        return result << 24;
    result = result << 8;

    result += str[1];
    if(str[2] == '\0')
        return result << 16;
    result = result << 8;

    result += str[2];
    if(str[3] == '\0')
        return result << 8;
    result = result << 8;

    result += str[3];
    if(str[4] != '\0')
        EZP_PERR("EZP: Block name %s is too long, truncating to 4 characters\n", str);
    return result;
}

//This function is time critical!
inline void EasyPerformanceAnalyzer::unhashStr(unsigned int hash, char* output)
{
    output[4] = '\0';

    output[3] = hash;
    hash = hash >> 8;

    output[2] = hash;
    hash = hash >> 8;

    output[1] = hash;
    hash = hash >> 8;

    output[0] = hash;
}

//This function is (mostly) not time critical
void EasyPerformanceAnalyzer::launchCmdListener()
{
    pthread_mutex_lock(&listenerLauncherLock);

    if(listenerRunning){ //Extra guard agains other threads entering this function at the same time
        pthread_mutex_unlock(&listenerLauncherLock);
        return;
    }

    struct sockaddr_un acceptorAddr;
    int acceptorFD;

    if((acceptorFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        EZP_PERR("EZP: socket() error: %s\n",strerror(errno));
        return;
    }

    memset(&acceptorAddr, 0, sizeof(acceptorAddr));
    acceptorAddr.sun_family = AF_UNIX;
    strncpy(acceptorAddr.sun_path, cmdSocketName, sizeof(acceptorAddr.sun_path) - 1);

    unlink(cmdSocketName);
    if(bind(acceptorFD, (struct sockaddr*)&acceptorAddr, sizeof(acceptorAddr)) == -1){
        EZP_PERR("EZP: bind() error: %s\n",strerror(errno));
        EZP_PERR("EZP: Make sure that this is the only instrumented process running on this machine.\n");
        EZP_PERR("EZP: There is no multiple EZP session functionality yet...\n");
        exit(-1);
    }
    if(listen(acceptorFD, 5) == -1){
        EZP_PERR("EZP: listen() error: %s\n",strerror(errno));
        exit(-1);
    }

    pthread_attr_t* listenerAttr = new pthread_attr_t();
    pthread_attr_init(listenerAttr);
    pthread_attr_setdetachstate(listenerAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setguardsize(listenerAttr, 0);
    pthread_attr_setstacksize(listenerAttr, PTHREAD_STACK_MIN); //This should be plenty
    int ret = pthread_create(&cmdListener, listenerAttr, &EasyPerformanceAnalyzer::listenCmd, (void*)(new int(acceptorFD)));
    if(!ret)
        listenerRunning = true;
    else
        EZP_PERR("EZP: pthread_create() error: %s\n", strerror(ret));

    pthread_mutex_unlock(&listenerLauncherLock);
}

//This function is not time critical
void* EasyPerformanceAnalyzer::listenCmd(void* arg)
{
    char buf[2];
    int ret;
    int clientFD;
    int acceptorFD = *((int*)arg);
    EZP_PRINT("EZP: [%d]\tCommand listener running...\n",(unsigned int)EZP_GET_TID);

    while(true){
        if((clientFD = accept(acceptorFD, NULL, NULL)) == -1) {
            EZP_PERR("EZP: accept() error: %s\n",strerror(errno));
            continue;
        }

        ret = read(clientFD, buf, sizeof(buf));
        if(ret == 2){
            switch(buf[0]){
                case 'e':
                    enabled = true;
                    EZP_PRINT("EZP: Enabled instrumentation upon remote request.\n");
                    break;
                case 'd':
                    enabled = false;
                    EZP_PRINT("EZP: Disabled instrumentation upon remote request.\n");
                    break;
                case 'p':
                    printOfflineProfiles();
                    EZP_PRINT("EZP: Printed offline analyses upon remote request.\n");
                    break;
                case 'c':
                    clearOfflineProfiles();
                    EZP_PRINT("EZP: Cleared offline analysis history upon remote request.\n");
                    break;
                default:
                    EZP_PERR("EZP: Unknown command received: %c\n", buf[0]);
                    break;
            }
        }
        else if(ret == -1)
            EZP_PERR("EZP: read() error: %s\n", strerror(errno));
        else
            EZP_PERR("EZP: Received %d bytes in read(), must receive 2 bytes\n", ret);

        close(clientFD);
    }

    //This function should and does return only when the process leaves
}

}

