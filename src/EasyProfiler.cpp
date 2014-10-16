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

Str2Clk EasyProfiler::blocks;

std::string EasyProfiler::androidTag = "EASYPROFILER";

void EasyProfiler::startProfiling(const std::string& blockName)
{
    Timespec* begin = new Timespec();
    blocks.insert(StrClkPair(blockName,begin));

    //Get time after insertion to bypass its execution time
    clock_gettime(CLOCK_THREAD_CPUTIME_ID,begin);
}

void EasyProfiler::endProfiling(const std::string& blockName)
{
    Timespec end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID,&end);
    Str2Clk::iterator pairIt = blocks.find(blockName);
    if(pairIt == blocks.end())
        EZP_OMNIPRINT("Can't find %s, did you call EZP_START() on %s?\n", blockName.c_str(), blockName.c_str());
    else{
        EZP_OMNIPRINT("%s took %.2f ms\n",blockName.c_str(),getTimeDiff(pairIt->second,&end));
        blocks.erase(pairIt);
        delete pairIt->second;
    }
}

inline float EasyProfiler::getTimeDiff(const Timespec* begin, const Timespec* end)
{
    return (float)(end->tv_sec - begin->tv_sec)*1000.0f + (float)((end->tv_nsec - begin->tv_nsec)/10000)/100.0f;
}

