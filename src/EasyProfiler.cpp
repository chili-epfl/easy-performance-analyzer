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

///////////////////////////////////////////////////////////////////////////////
//Static members
///////////////////////////////////////////////////////////////////////////////

Str2Clk EasyProfiler::blocks;
Str2SMarker EasyProfiler::smoothBlocks;
std::string EasyProfiler::androidTag = "EASYPROFILER";

///////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////

void EasyProfiler::startProfiling(const std::string& blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    Timespec* begin = new Timespec();
    blocks.insert(StrClkPair(blockName,begin));
    clock_gettime(EZP_CLOCK,begin);
}

void EasyProfiler::endProfiling(const std::string& blockName)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);
    Str2Clk::iterator pairIt = blocks.find(blockName);
    if(pairIt == blocks.end())
        EZP_OMNIPRINT("Can't find %s, did you call EZP_START() on %s?\n", blockName.c_str(), blockName.c_str());
    else{
        EZP_OMNIPRINT("%s took %.2f ms\n",blockName.c_str(),getTimeDiff(pairIt->second,&end));
        blocks.erase(pairIt);
        delete pairIt->second;
    }
}

void EasyProfiler::startProfilingSmooth(const std::string& blockName)
{
    //Get time in the very end to disturb the measurements the least possible
    SmoothMarker* begin = new SmoothMarker();
    Str2SMarker::iterator pairIt = smoothBlocks.find(blockName);
    if(pairIt == smoothBlocks.end()){
        smoothBlocks.insert(Str2SMarkerPair(blockName,begin));
        clock_gettime(EZP_CLOCK,&(begin->beginTime));
    }
    else
        clock_gettime(EZP_CLOCK,&(pairIt->second->beginTime));
}

void EasyProfiler::endProfilingSmooth(const std::string& blockName, float sf)
{
    Timespec end;
    clock_gettime(EZP_CLOCK,&end);
    Str2SMarker::iterator pairIt = smoothBlocks.find(blockName);
    if(pairIt == smoothBlocks.end())
        EZP_OMNIPRINT("Can't find %s, did you call EZP_START_SMOOTH() on %s?\n", blockName.c_str(), blockName.c_str());
    else{
        pairIt->second->lastSlice = sf*pairIt->second->lastSlice + (1.0f - sf)*getTimeDiff(&pairIt->second->beginTime,&end);
        EZP_OMNIPRINT("%s took %.2f ms\n",blockName.c_str(),pairIt->second->lastSlice);
    }
}

inline float EasyProfiler::getTimeDiff(const Timespec* begin, const Timespec* end)
{
    return (float)(end->tv_sec - begin->tv_sec)*1000.0f + (float)(end->tv_nsec - begin->tv_nsec)/1000000.0f;
}

