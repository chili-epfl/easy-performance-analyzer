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
 * @file easy-profiler.cpp
 * @brief Simple instrumentation profiler
 * @author Ayberk Özgür
 * @date 2014-10-15
 */

#include"EasyProfiler.hpp"

namespace ezp{

///////////////////////////////////////////////////////////////////////////////
//Static members
///////////////////////////////////////////////////////////////////////////////

const char* EasyProfiler::androidTag = "EASYPROFILER";

Blk2Clk EasyProfiler::blocks(BlockKey::compare);
Blk2SMarker EasyProfiler::smoothBlocks(BlockKey::compare);
Blk2AMarker EasyProfiler::offlineBlocks(BlockKey::compare);

pthread_mutex_t EasyProfiler::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t EasyProfiler::smoothLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t EasyProfiler::offlineLock = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////

//This function is time critical!
void EasyProfiler::startProfiling(const char* blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    Timespec* begin = new Timespec();

    BlkClkPair pair(BlockKey(EZP_GET_TID, hashStr(blockName)), begin);

    pthread_mutex_lock(&lock);
    std::pair<Blk2Clk::iterator, bool> result = blocks.insert(pair);

    //We did not find the marker from before, so we inserted the new one
    if(result.second){
        pthread_mutex_unlock(&lock);

        clock_gettime(EZP_CLOCK,begin);
    }

    //We found a marker from before, so we didn't insert the new one
    else{
        delete begin;
        Timespec* target = result.first->second;
        pthread_mutex_unlock(&lock);

        clock_gettime(EZP_CLOCK,target);
    }
}

//This function is time critical!
void EasyProfiler::endProfiling(const char* blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK, &end);

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
void EasyProfiler::startProfilingSmooth(const char* blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    SmoothMarker* begin = new SmoothMarker();

    Blk2SMarkerPair pair(BlockKey(EZP_GET_TID, hashStr(blockName)), begin);

    pthread_mutex_lock(&smoothLock);
    std::pair<Blk2SMarker::iterator, bool> result = smoothBlocks.insert(pair);

    //We did not find the marker from before, so we inserted the new one
    if(result.second){
        pthread_mutex_unlock(&smoothLock);

        clock_gettime(EZP_CLOCK,&(begin->beginTime));
    }

    //We found a marker from before, so we didn't insert the new one
    else{
        delete begin;
        SmoothMarker* target = result.first->second;
        pthread_mutex_unlock(&smoothLock);

        clock_gettime(EZP_CLOCK,&(target->beginTime));
    }
}

//This function is time critical!
void EasyProfiler::endProfilingSmooth(const char* blockName, float sf)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);

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
void EasyProfiler::startProfilingOffline(const char* blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    AggregateMarker* begin = new AggregateMarker();

    Blk2AMarkerPair pair(BlockKey(EZP_GET_TID, hashStr(blockName)), begin);

    pthread_mutex_lock(&offlineLock);
    std::pair<Blk2AMarker::iterator, bool> result = offlineBlocks.insert(pair);

    //We did not find the marker from before, so we inserted a new one
    if(result.second){
        pthread_mutex_unlock(&offlineLock);

        clock_gettime(EZP_CLOCK,&(begin->beginTime));
    }

    //We found a marker from before, so we didn't insert the new one
    else{
        delete begin;
        AggregateMarker* target = result.first->second;
        pthread_mutex_unlock(&offlineLock);

        clock_gettime(EZP_CLOCK,&(target->beginTime));
    }
}

//This function is time critical!
void EasyProfiler::endProfilingOffline(const char* blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);

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
void EasyProfiler::printOfflineProfiles()
{
    pthread_mutex_lock(&offlineLock);
    if(offlineBlocks.size() == 0){
        EZP_OMNIPRINT("EZP ERROR: No offline profile found; profile some code first by wrapping it with EZP_START_OFFLINE() ... EZP_END_OFFLINE()\n");
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
    EZP_OMNIPRINT("Thread-wise profile results\n");
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
    EZP_OMNIPRINT("Summed results across threads\n");
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
void EasyProfiler::clearOfflineProfiles()
{
    pthread_mutex_lock(&offlineLock);
    //TODO: MEMORY LEAK: DELETE KEYS AND VALUES BEFORE CLEARING!!!!!!!!!!!!
    offlineBlocks.clear();
    pthread_mutex_unlock(&offlineLock);
}

//This function is time critical!
inline float EasyProfiler::getTimeDiff(const Timespec* begin, const Timespec* end)
{
    return (float)(end->tv_sec - begin->tv_sec)*1000.0f + (float)(end->tv_nsec - begin->tv_nsec)/1000000.0f;
}

//This function is time critical!
inline unsigned int EasyProfiler::hashStr(const char* str)
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
        EZP_OMNIPRINT("EZP ERROR: Block name %s is too long, truncating to 4 characters; consider shortening the name for better performance\n", str);
    return result;
}

//This function is time critical!
inline void EasyProfiler::unhashStr(unsigned int hash, char* output)
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

} /* namespace ezp */

