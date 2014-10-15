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
 * @brief
 * @author Ayberk Özgür
 * @date 2014-10-15
 */

#include"EasyProfiler.hpp"

Str2Clk EasyProfiler::blocks;

std::string EasyProfiler::androidTag = "EASYPROFILER";

void EasyProfiler::startProfiling(const std::string& blockName)
{
    blocks.insert(StrClkPair(blockName,clock()));
}

void EasyProfiler::endProfiling(const std::string& blockName)
{
    clock_t endt = clock();
    Str2Clk::const_iterator pairIt = blocks.find(blockName);
    if(pairIt == blocks.end())
        EZP_OMNIPRINT("Can't find %s, did you call startProfiling() on %s?", blockName.c_str(), blockName.c_str());
    else{
        endt = endt - pairIt->second;
        EZP_OMNIPRINT("%s took %.2f ms",blockName.c_str(),(float)endt/CLOCKS_PER_SEC*1000);
    }
}

