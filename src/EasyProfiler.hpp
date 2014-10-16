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
 * @brief Simple instrumentation profiler
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

typedef struct timespec Timespec;
typedef std::map<std::string,Timespec*> Str2Clk;
typedef std::pair<std::string,Timespec*> StrClkPair;

/**
 * @brief Simple instrumentation profiler that relies on CPU clocks
 */
class EasyProfiler{
public:

    /**
     * @brief Starts a named profile
     *
     * @param blockName Name of the profile
     */
    static void startProfiling(const std::string& blockName = "UNNAMED_BLOCK");

    /**
     * @brief Ends the named profile, it must have been started before
     *
     * @param blockName Name of the profile
     */
    static void endProfiling(const std::string& blockName = "UNNAMED_BLOCK");

    static std::string androidTag;  ///< Logcat tag on Android

private:

    /**
     * @brief Gets the time difference between two times
     *
     * @param begin Beginning time
     * @param end Ending time
     *
     * @return Difference in milliseconds, 2 digits precision after the decimal point
     */
    static float getTimeDiff(const Timespec* begin, const Timespec* end);

    static Str2Clk blocks;          ///< Names and beginning times of profile blocks
};

#ifdef ANDROID
#define EZP_OMNIPRINT(...) __android_log_print(ANDROID_LOG_INFO, EasyProfiler::androidTag, __VA_ARGS__)
#else
#define EZP_OMNIPRINT(...) printf(__VA_ARGS__)
#endif

/**
 * @brief Sets the Logcat tag on Android
 */
#define EZP_SET_ANDROID_TAG(TAG) EasyProfiler::androidTag = TAG;

/**
 * @brief Alias for EasyProfiler::startProfiling()
 */
#define EZP_START(BLOCK_NAME) EasyProfiler::startProfiling(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfiling()
 */
#define EZP_END(BLOCK_NAME) EasyProfiler::endProfiling(BLOCK_NAME);

#endif /* EASY_PROFILER_HPP */
