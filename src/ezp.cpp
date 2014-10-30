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
//Static members
///////////////////////////////////////////////////////////////////////////////

const char* EasyPerformanceAnalyzer::androidTag = "EZP";
const char* EasyPerformanceAnalyzer::cmdSocketName = "\0ezp_socket";

pthread_t EasyPerformanceAnalyzer::cmdListener;

bool EasyPerformanceAnalyzer::listenerRunning = false;
bool EasyPerformanceAnalyzer::enabled = false;

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
void EasyPerformanceAnalyzer::cmdExternal(bool enabled)
{
    struct sockaddr_un addr;
    int fd;

    if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        //perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, cmdSocketName, sizeof(addr.sun_path) - 1);

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        //perror("connect error");
        exit(-1);
    }

    if(enabled)
        write(fd,"e",2);
    else
        write(fd,"d",2);

    close(fd);
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

        EZP_OMNIPRINT("EZP ERROR: Can't find %s, did you call EZP_START(\"%s\")?\n", blockName, blockName);
    }
    else{
        Timespec* begin = pairIt->second;
        pthread_mutex_unlock(&lock);

        EZP_OMNIPRINT("[%d]\t%s\t%6.2f ms\n", tid, blockName, getTimeDiff(begin,&end));
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

        EZP_OMNIPRINT("EZP ERROR: Can't find %s, did you call EZP_START_SMOOTH(\"%s\")?\n", blockName, blockName);
    }
    else{
        float slice = sf*pairIt->second->lastSlice + (1.0f - sf)*getTimeDiff(&(pairIt->second->beginTime),&end);
        pairIt->second->lastSlice = slice;
        pthread_mutex_unlock(&smoothLock);

        EZP_OMNIPRINT("[%d]\t%s\t%6.2f ms\n", tid, blockName, slice);
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

        EZP_OMNIPRINT("EZP ERROR: Can't find %s, did you call EZP_START_OFFLINE(\"%s\")?\n", blockName, blockName);
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
    if(!enabled)
        return;

    pthread_mutex_lock(&offlineLock);
    if(offlineBlocks.size() == 0){
        EZP_OMNIPRINT("EZP ERROR: No offline block found; instrument some code first by wrapping it with EZP_START_OFFLINE() ... EZP_END_OFFLINE()\n");
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
    }
    pthread_mutex_unlock(&offlineLock);

    //Sort for printing according to average time taken
    std::sort(sortedProfiles.begin(),sortedProfiles.end(),AggregateProfile::compareAvgTime);

    //Do the thread-wise printing
    EZP_OMNIPRINT("=======================================================================\n");
    EZP_OMNIPRINT("Thread-wise analysis results\n");
    EZP_OMNIPRINT("-----------------------------------------------------------------------\n");
    EZP_OMNIPRINT("Thread ID    Name    Average(ms)         Total(ms)           Calls     \n");
    EZP_OMNIPRINT("-----------------------------------------------------------------------\n");
    char cbuf[5];
    for(std::vector<AggregateProfile>::iterator it = sortedProfiles.begin(); it != sortedProfiles.end(); it++){
        unhashStr(it->blockName,cbuf);
        EZP_OMNIPRINT("%9d    %4s    %-16.2f    %-16.2f    %-10d\n",
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
    EZP_OMNIPRINT("=======================================================================\n");
    EZP_OMNIPRINT("Anaylsis results summed across threads\n");
    EZP_OMNIPRINT("-----------------------------------------------------------------------\n");
    EZP_OMNIPRINT("Name    Average(ms)         Total(ms)           Calls                  \n");
    EZP_OMNIPRINT("-----------------------------------------------------------------------\n");
    for(std::vector<SummedProfile>::iterator it = totalProfiles.begin(); it != totalProfiles.end(); it++){
        unhashStr(it->blockName,cbuf);
        EZP_OMNIPRINT("%4s    %-16.2f    %-16.2f    %-10d\n",
                cbuf, it->totalTime/it->numSamples, it->totalTime, it->numSamples);
    }
    EZP_OMNIPRINT("=======================================================================\n");
}

//This function is not time critical
void EasyPerformanceAnalyzer::clearOfflineProfiles()
{
    if(!enabled)
        return;

    pthread_mutex_lock(&offlineLock);
    //TODO: MEMORY LEAK: DELETE KEYS AND VALUES BEFORE CLEARING!!!!!!!!!!!!
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
        EZP_OMNIPRINT("EZP ERROR: Block name %s is too long, truncating to 4 characters\n", str);
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
inline void EasyPerformanceAnalyzer::launchCmdListener()
{
    pthread_mutex_lock(&listenerLauncherLock);

    if(listenerRunning){ //Extra guard agains other threads entering this function at the same time
        pthread_mutex_unlock(&listenerLauncherLock);
        return;
    }

    struct sockaddr_un acceptorAddr;
    int acceptorFD;

    if((acceptorFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        //perror("socket error");
        exit(-1);
    }

    memset(&acceptorAddr, 0, sizeof(acceptorAddr));
    acceptorAddr.sun_family = AF_UNIX;
    strncpy(acceptorAddr.sun_path, cmdSocketName, sizeof(acceptorAddr.sun_path) - 1);

    unlink(cmdSocketName);
    if(bind(acceptorFD, (struct sockaddr*)&acceptorAddr, sizeof(acceptorAddr)) == -1){
        //perror("bind error");
        exit(-1);
    }
    if(listen(acceptorFD, 5) == -1){
        //perror("listen error");
        exit(-1);
    }

    pthread_attr_t* listenerAttr = new pthread_attr_t();
    pthread_attr_init(listenerAttr);
    pthread_attr_setdetachstate(listenerAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setguardsize(listenerAttr, 0);
    pthread_attr_setstacksize(listenerAttr, PTHREAD_STACK_MIN); //This should be plenty
    if(!pthread_create(&cmdListener, listenerAttr, &EasyPerformanceAnalyzer::listenCmd, (void*)(new int(acceptorFD))))
        listenerRunning = true;
    else
        EZP_OMNIPRINT("Could not launch thread\n");

    pthread_mutex_unlock(&listenerLauncherLock);
}

//This function is not time critical
void* EasyPerformanceAnalyzer::listenCmd(void* arg)
{
    char buf[100];
    int cl,rc;
    int acceptorFD = *((int*)arg);
    EZP_OMNIPRINT("Listening to commands...\n");

    while(true){
        if ( (cl = accept(acceptorFD, NULL, NULL)) == -1) {
            //perror("accept error");
            continue;
        }

        while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
            EZP_OMNIPRINT("read %u bytes: %.*s\n", rc, rc, buf);
        }
        if (rc == -1) {
            //perror("read");
            exit(-1);
        }
        else if (rc == 0) {
            EZP_OMNIPRINT("EOF\n");
            close(cl);
        }
    }

    //This function does and should return only when the process leaves
}

}

