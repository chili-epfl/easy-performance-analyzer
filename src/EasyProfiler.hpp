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

#ifndef EASYPROFILER_HPP
#define EASYPROFILER_HPP

#include<ctime>
#include<map>
#include<string>
#include<vector>
#include<algorithm>

#ifdef ANDROID
#include<android/log.h>
#else
#include<cstdio>
#endif

#define EZP_CLOCK CLOCK_THREAD_CPUTIME_ID

typedef struct timespec Timespec;

typedef std::map<std::string,Timespec*> Str2Clk;
typedef std::pair<std::string,Timespec*> StrClkPair;

typedef struct SmoothMarker_
{
    Timespec beginTime;
    float lastSlice;

    SmoothMarker_(){ lastSlice = 0.0f; }
}
SmoothMarker;

typedef std::map<std::string,SmoothMarker*> Str2SMarker;
typedef std::pair<std::string,SmoothMarker*> Str2SMarkerPair;

typedef struct AggregateMarker_
{
    Timespec beginTime;
    float totalTime;
    int numSamples;

    AggregateMarker_(){ totalTime = 0.0f; numSamples = 0; }
}
AggregateMarker;

typedef std::map<std::string,AggregateMarker*> Str2AMarker;
typedef std::pair<std::string,AggregateMarker*> Str2AMarkerPair;

typedef struct AggregateProfile_
{
    std::string blockName;
    float averageTime;
    int numSamples;

    static bool compare(const struct AggregateProfile_& one, const struct AggregateProfile_& two){
        return one.averageTime > two.averageTime;
    }
}
AggregateProfile;

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

    /**
     * @brief Starts a named profile that is smoothed over time
     *
     * @param blockName Name of the profile
     */
    static void startProfilingSmooth(const std::string& blockName = "UNNAMED_BLOCK");

    /**
     * @brief  Ends a named smoothed profile, it must have been started before
     *
     * @param blockName Name of the profile
     * @param smoothingFactor Coefficient of the history, between 0 and 1
     */
    static void endProfilingSmooth(const std::string& blockName = "UNNAMED_BLOCK", float smoothingFactor = 0.95f);


    static void startProfilingOffline(const std::string& blockName = "UNNAMED_BLOCK");

    static void endProfilingOffline(const std::string& blockName = "UNNAMED_BLOCK");

    static void printOfflineProfiles();

    static void clearOfflineProfiles();

    static std::string androidTag;      ///< Logcat tag on Android

private:

    /**
     * @brief Gets the time difference between two times
     *
     * @param begin Beginning time
     * @param end Ending time
     *
     * @return Difference in milliseconds
     */
    static float getTimeDiff(const Timespec* begin, const Timespec* end);

    static Str2Clk blocks;              ///< Names and beginning times of profile blocks
    static Str2SMarker smoothBlocks;    ///< Names, beginning times and latest time slices of smoothed profile blocks
    static Str2AMarker offlineBlocks;   ///< Names, beginning times, total times and number of samples of offline profile blocks
};

#ifdef ANDROID
#define EZP_OMNIPRINT(...) __android_log_print(ANDROID_LOG_INFO, EasyProfiler::androidTag.c_str(), __VA_ARGS__)
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

/**
 * @brief Alias for EasyProfiler::startProfilingSmooth()
 */
#define EZP_START_SMOOTH(BLOCK_NAME) EasyProfiler::startProfilingSmooth(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfilingSmooth() with default smoothing factor
 */
#define EZP_END_SMOOTH(BLOCK_NAME) EasyProfiler::endProfilingSmooth(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfilingSmooth() with custom smoothing factor
 */
#define EZP_END_SMOOTH_FACTOR(BLOCK_NAME,SMOOTHING_FACTOR) EasyProfiler::endProfilingSmooth(BLOCK_NAME,SMOOTHING_FACTOR);


#define EZP_START_OFFLINE(BLOCK_NAME) EasyProfiler::startProfilingOffline(BLOCK_NAME);
#define EZP_END_OFFLINE(BLOCK_NAME) EasyProfiler::endProfilingOffline(BLOCK_NAME);
#define EZP_PRINT_OFFLINE EasyProfiler::printOfflineProfiles();
#define EZP_CLEAR_OFFLINE EasyProfiler::clearOfflineProfiles();

#endif /* EASYPROFILER_HPP */
