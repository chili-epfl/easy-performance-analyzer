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

std::string EasyProfiler::androidTag = "EASYPROFILER";

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
void EasyProfiler::startProfiling(const std::string& blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    Timespec* begin = new Timespec();

    BlkClkPair pair(BlockKey(EZP_GET_TID, blockName), begin);

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
void EasyProfiler::endProfiling(const std::string& blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK, &end);

    TID tid = EZP_GET_TID;
    BlockKey key(tid, blockName);

    pthread_mutex_lock(&lock);
    Blk2Clk::iterator pairIt = blocks.find(key);
    if(pairIt == blocks.end()){
        pthread_mutex_unlock(&lock);

        EZP_OMNIPRINT("EZP ERROR: Can't find %s, did you call EZP_START(\"%s\")?\n", blockName.c_str(), blockName.c_str());
    }
    else{
        Timespec* begin = pairIt->second;
        pthread_mutex_unlock(&lock);

        EZP_OMNIPRINT("[%d]\t%s\t%6.2f ms\n", tid, blockName.c_str(), getTimeDiff(begin,&end));
    }
}

//This function is time critical!
void EasyProfiler::startProfilingSmooth(const std::string& blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    SmoothMarker* begin = new SmoothMarker();

    Blk2SMarkerPair pair(BlockKey(EZP_GET_TID, blockName), begin);

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
void EasyProfiler::endProfilingSmooth(const std::string& blockName, float sf)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);

    TID tid = EZP_GET_TID;
    BlockKey key(tid, blockName);

    pthread_mutex_lock(&smoothLock);
    Blk2SMarker::iterator pairIt = smoothBlocks.find(key);
    if(pairIt == smoothBlocks.end()){
        pthread_mutex_unlock(&smoothLock);

        EZP_OMNIPRINT("EZP ERROR: Can't find %s, did you call EZP_START_SMOOTH(\"%s\")?\n", blockName.c_str(), blockName.c_str());
    }
    else{
        float slice = sf*pairIt->second->lastSlice + (1.0f - sf)*getTimeDiff(&(pairIt->second->beginTime),&end);
        pairIt->second->lastSlice = slice;
        pthread_mutex_unlock(&smoothLock);

        EZP_OMNIPRINT("[%d]\t%s\t%6.2f ms\n", tid, blockName.c_str(), slice);
    }
}

//This function is time critical!
void EasyProfiler::startProfilingOffline(const std::string& blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    AggregateMarker* begin = new AggregateMarker();

    Blk2AMarkerPair pair(BlockKey(EZP_GET_TID, blockName), begin);

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
void EasyProfiler::endProfilingOffline(const std::string& blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);

    BlockKey key(EZP_GET_TID, blockName);

    pthread_mutex_lock(&offlineLock);
    Blk2AMarker::iterator pairIt = offlineBlocks.find(key);
    if(pairIt == offlineBlocks.end()){
        pthread_mutex_unlock(&offlineLock);

        EZP_OMNIPRINT("EZP ERROR: Can't find %s, did you call EZP_START_OFFLINE(\"%s\")?\n", blockName.c_str(), blockName.c_str());
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
    int maxNameLen = 0;
    Blk2AMarker::iterator its = offlineBlocks.begin();
    std::vector<AggregateProfile>::iterator itt = sortedProfiles.begin();
    for(; its != offlineBlocks.end(); its++, itt++){
        if(its->first.blockName.length() > maxNameLen)
            maxNameLen = its->first.blockName.length();
        itt->tid = its->first.tid;
        itt->blockName = its->first.blockName;
        itt->numSamples = its->second->numSamples;
        itt->averageTime = its->second->totalTime/itt->numSamples;
    }
    pthread_mutex_unlock(&offlineLock);

    //Sort for printing according to average time taken
    std::sort(sortedProfiles.begin(),sortedProfiles.end(),AggregateProfile::compareAvgTime);

    //Do the thread-wise printing
    std::string spaces(maxNameLen - 4, ' ');
    std::string dashes(maxNameLen - 4, '-');
    std::string equals(maxNameLen - 4, '=');
    EZP_OMNIPRINT("%s=======================================================================\n", equals.c_str());
    EZP_OMNIPRINT("Thread-wise profile results\n");
    EZP_OMNIPRINT("-------------%s----------------------------------------------------------\n", dashes.c_str());
    EZP_OMNIPRINT("Thread ID    %sName    Average(ms)         Total(ms)           Calls     \n", spaces.c_str());
    EZP_OMNIPRINT("-------------%s----------------------------------------------------------\n", dashes.c_str());
    for(std::vector<AggregateProfile>::iterator it = sortedProfiles.begin(); it != sortedProfiles.end(); it++)
        EZP_OMNIPRINT("%9d    %*s    %-16.2f    %-16.2f    %-10d\n",
                it->tid, maxNameLen, it->blockName.c_str(), it->averageTime, it->averageTime*it->numSamples, it->numSamples);

    //Sort according to block name for summing
    std::sort(sortedProfiles.begin(),sortedProfiles.end(),AggregateProfile::compareBlockName);

    //Sum profiles coming from different threads
    std::vector<SummedProfile> totalProfiles;
    std::vector<AggregateProfile>::iterator itS = sortedProfiles.begin();
    totalProfiles.push_back(SummedProfile(itS->blockName, itS->averageTime*itS->numSamples, itS->numSamples));
    itS++;
    for(;itS!=sortedProfiles.end();itS++)
        if(itS->blockName.compare(totalProfiles.back().blockName) == 0){
            totalProfiles.back().totalTime += itS->averageTime*itS->numSamples;
            totalProfiles.back().numSamples += itS->numSamples;
        }
        else
            totalProfiles.push_back(SummedProfile(itS->blockName, itS->averageTime*itS->numSamples, itS->numSamples));

    //Sort summed profiles according to average time
    std::sort(totalProfiles.begin(),totalProfiles.end(),SummedProfile::compare);

    //Print summed profiles
    EZP_OMNIPRINT("%s=======================================================================\n", equals.c_str());
    EZP_OMNIPRINT("Summed results across threads\n");
    EZP_OMNIPRINT("%s-----------------------------------------------------------------------\n", dashes.c_str());
    EZP_OMNIPRINT("%sName    Average(ms)         Total(ms)           Calls                  \n", spaces.c_str());
    EZP_OMNIPRINT("%s-----------------------------------------------------------------------\n", dashes.c_str());
    for(std::vector<SummedProfile>::iterator it = totalProfiles.begin(); it != totalProfiles.end(); it++)
        EZP_OMNIPRINT("%*s    %-16.2f    %-16.2f    %-10d\n",
                maxNameLen, it->blockName.c_str(), it->totalTime/it->numSamples, it->totalTime, it->numSamples);
    EZP_OMNIPRINT("%s=======================================================================\n", equals.c_str());
}

//This function is not time critical
void EasyProfiler::clearOfflineProfiles()
{
    pthread_mutex_lock(&offlineLock);
    //TODO: MEMORY LEAK: DELETE KEYS AND VALUES BEFORE CLEARING!!!!!!!!!!!!
    offlineBlocks.clear();
    pthread_mutex_unlock(&offlineLock);
}

inline float EasyProfiler::getTimeDiff(const Timespec* begin, const Timespec* end)
{
    return (float)(end->tv_sec - begin->tv_sec)*1000.0f + (float)(end->tv_nsec - begin->tv_nsec)/1000000.0f;
}

} /* namespace ezp */

