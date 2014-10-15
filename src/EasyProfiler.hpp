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
 * @file easy-profiler.hpp
 * @brief 
 * @author Ayberk Özgür
 * @date 2014-10-15
 */

#ifndef EASY_PROFILER_HPP
#define EASY_PROFILER_HPP

#include<ctime>
#include<map>
#include<string>

#ifdef ANDROID
#include<android/log.h>
#else
#include<cstdio>
#endif

typedef std::map<std::string,clock_t> Str2Clk;
typedef std::pair<std::string,clock_t> StrClkPair;

/**
 * @brief 
 */
class EasyProfiler{
public:

    /**
     * @brief 
     *
     * @param blockName
     */
    static void startProfiling(const std::string& blockName = "UNNAMED_BLOCK");

    /**
     * @brief 
     *
     * @param blockName
     */
    static void endProfiling(const std::string& blockName = "UNNAMED_BLOCK");

    static std::string androidTag;  ///< 

private:

    static Str2Clk blocks;          ///< 

};

#ifdef ANDROID
#define EZP_OMNIPRINT(...) __android_log_print(ANDROID_LOG_INFO, EasyProfiler::androidTag, __VA_ARGS__)
#else
#define EZP_OMNIPRINT(...) printf(__VA_ARGS__)
#endif

#define EZP_START(BLOCK_NAME) EasyProfiler::startProfiling(BLOCK_NAME)
#define EZP_END(BLOCK_NAME) EasyProfiler::endProfiling(BLOCK_NAME)

#endif /* EASY_PROFILER_HPP */
