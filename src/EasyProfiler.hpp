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

///////////////////////////////////////////////////////////////////////////////
//Public API
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Sets the Logcat tag on Android
 */
#define EZP_SET_ANDROID_TAG(TAG) ezp::EasyProfiler::androidTag = TAG;

/**
 * @brief Alias for EasyProfiler::startProfiling()
 */
#define EZP_START(BLOCK_NAME) ezp::EasyProfiler::startProfiling(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfiling()
 */
#define EZP_END(BLOCK_NAME) ezp::EasyProfiler::endProfiling(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::startProfilingSmooth()
 */
#define EZP_START_SMOOTH(BLOCK_NAME) ezp::EasyProfiler::startProfilingSmooth(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfilingSmooth() with default smoothing factor
 */
#define EZP_END_SMOOTH(BLOCK_NAME) ezp::EasyProfiler::endProfilingSmooth(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfilingSmooth() with custom smoothing factor
 */
#define EZP_END_SMOOTH_FACTOR(BLOCK_NAME,SMOOTHING_FACTOR) ezp::EasyProfiler::endProfilingSmooth(BLOCK_NAME,SMOOTHING_FACTOR);

/**
 * @brief Alias for EasyProfiler::startProfilingOffline()
 */
#define EZP_START_OFFLINE(BLOCK_NAME) ezp::EasyProfiler::startProfilingOffline(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::endProfilingOffline()
 */
#define EZP_END_OFFLINE(BLOCK_NAME) ezp::EasyProfiler::endProfilingOffline(BLOCK_NAME);

/**
 * @brief Alias for EasyProfiler::printOfflineProfiles()
 */
#define EZP_PRINT_OFFLINE ezp::EasyProfiler::printOfflineProfiles();

/**
 * @brief Alias for EasyProfiler::clearOfflineProfiles()
 */
#define EZP_CLEAR_OFFLINE ezp::EasyProfiler::clearOfflineProfiles();

///////////////////////////////////////////////////////////////////////////////
//Private API
///////////////////////////////////////////////////////////////////////////////

#include<ctime>
#include<map>
#include<string>
#include<vector>
#include<algorithm>
#include<pthread.h>
#include<sys/syscall.h>
#include<unistd.h>

#ifdef ANDROID
#include<android/log.h>
#else
#include<cstdio>
#endif

namespace ezp{

#define EZP_CLOCK CLOCK_THREAD_CPUTIME_ID
#ifdef ANDROID
#define EZP_GET_TID gettid()
#else
#define EZP_GET_TID syscall(SYS_gettid)
#endif

typedef pid_t TID;

typedef struct timespec Timespec;

/**
 * @brief Key to reaching profile records
 */
struct BlockKey_t{
    TID tid;                ///< Thread ID of the block's caller
    unsigned int blockName; ///< Hash of name of the block

    /**
     * @brief Creates a new profile record key
     *
     * @param tid_ Thread ID of the block's caller
     * @param blockName_ Hash of the name of the block
     */
    BlockKey_t(TID tid_, unsigned int blockName_){
        tid = tid_;
        blockName = blockName_;
    }

    /**
     * @brief Compares two BlockKeys for sorting purposes, thread ID has priority
     *
     * @param one First compared key
     * @param two Second compared key
     *
     * @return Whether first key is ahead of second key
     */
    static bool compare(const struct BlockKey_t& one, const struct BlockKey_t& two){
        if(one.tid == two.tid)
            return one.blockName > two.blockName;
        else
            return one.tid > two.tid;
    }
};

/**
 * @brief Holds a smooth profile record
 */
struct SmoothMarker_t{
    Timespec beginTime; ///< When the most recent profile was started
    float lastSlice;    ///< Most recent smoothed time that this profile took, i.e history

    /**
     * @brief Creates a new smooth profile record with zero history
     */
    SmoothMarker_t(){ lastSlice = 0.0f; }
};

/**
 * @brief Holds the total amount of time a profile took in the past
 */
struct AggregateMarker_t{
    Timespec beginTime; ///< When the most recent profile was started
    float totalTime;    ///< Total time that this profile took in the past
    int numSamples;     ///< How many times this profile was done in the past

    /**
     * @brief Creates a new aggregate profile with zero history
     */
    AggregateMarker_t(){ totalTime = 0.0f; numSamples = 0; }
};

/**
 * @brief Brings AggregateMarker and BlockKey together in a sortable object
 */
struct AggregateProfile_t{
    TID tid;                ///< Thread ID
    unsigned int blockName; ///< Hash of the name of the profile
    float averageTime;      ///< Average time the profile took in the past
    int numSamples;         ///< How many times this profile was done in the past

    /**
     * @brief Compares two AggregateProfiles on their average times for sorting purposes
     *
     * @param one First compared profile
     * @param two Second compared profile
     *
     * @return Whether first took more average time than the second
     */
    static bool compareAvgTime(const struct AggregateProfile_t& one, const struct AggregateProfile_t& two)
    {
        return one.averageTime > two.averageTime;
    }

    /**
     * @brief Compares two AggregateProfiles on their block names for sorting purposes
     *
     * @param one First compared profile
     * @param two Second compared profile
     *
     * @return Whether first block name comes before the second
     */
    static bool compareBlockName(const struct AggregateProfile_t& one, const struct AggregateProfile_t& two)
    {
        return one.blockName < two.blockName;
    }
};

/**
 * @brief Sum of profiles coming from different threads
 */
struct SummedProfile_t{
    unsigned int blockName; ///< Hash of the name of the profile
    float totalTime;        ///< Total time this profile took
    int numSamples;         ///< Total number of times this profile was done

    /**
     * @brief Initializes a new summed profile
     *
     * @param blockName_ Hash of the name of the profile
     * @param totalTime_ Initial total time coming from a thread
     * @param numSamples_ Initial number of times this profile was done, coming from a thread
     */
    SummedProfile_t(unsigned int blockName_, float totalTime_, int numSamples_){
        blockName = blockName_;
        totalTime = totalTime_;
        numSamples = numSamples_;
    }

    /**
     * @brief Compares two SummedProfiles on their average times for sorting purposes
     *
     * @param one First compared profile
     * @param two Second compared profile
     *
     * @return Whether first took more average time than the second
     */
    static bool compare(const struct SummedProfile_t& one, const struct SummedProfile_t& two)
    {
        return one.totalTime/one.numSamples > two.totalTime/two.numSamples;
    }
};

typedef struct BlockKey_t BlockKey;
typedef bool (*BlockKeyComp)(const BlockKey&, const BlockKey&);
typedef struct SmoothMarker_t SmoothMarker;
typedef struct AggregateMarker_t AggregateMarker;
typedef struct AggregateProfile_t AggregateProfile;
typedef struct SummedProfile_t SummedProfile;
typedef std::map<BlockKey, Timespec*, BlockKeyComp> Blk2Clk;
typedef std::pair<BlockKey, Timespec*> BlkClkPair;
typedef std::map<BlockKey, SmoothMarker*, BlockKeyComp> Blk2SMarker;
typedef std::pair<BlockKey, SmoothMarker*> Blk2SMarkerPair;
typedef std::map<BlockKey, AggregateMarker*, BlockKeyComp> Blk2AMarker;
typedef std::pair<BlockKey, AggregateMarker*> Blk2AMarkerPair;

/**
 * @brief Simple instrumentation profiler that relies on CPU clocks
 */
class EasyProfiler{
public:

    /**
     * @brief Starts a named profile
     *
     * @param blockName Name of the profile, max 4 characters
     */
    static void startProfiling(const char* blockName = "NDEF");

    /**
     * @brief Ends the named profile, printing the running time of the block; it must have been started before
     *
     * @param blockName Name of the profile, max 4 characters
     */
    static void endProfiling(const char* blockName = "NDEF");

    /**
     * @brief Starts a named profile that is smoothed over time
     *
     * @param blockName Name of the profile, max 4 characters
     */
    static void startProfilingSmooth(const char* blockName = "NDEF");

    /**
     * @brief  Ends a named smoothed profile, printing the running time of the block; it must have been started before
     *
     * @param blockName Name of the profile, max 4 characters
     * @param smoothingFactor Coefficient of the history, between 0 and 1
     */
    static void endProfilingSmooth(const char* blockName = "NDEF", float smoothingFactor = 0.95f);

    /**
     * @brief Starts a named offline profile that keeps the total running time and number of calls
     *
     * @param blockName Name of the profile, max 4 characters
     */
    static void startProfilingOffline(const char* blockName = "NDEF");

    /**
     * @brief Ends a named offline profile, it must have been smoothed before
     *
     * @param blockName Name of the profile, max 4 characters
     */
    static void endProfilingOffline(const char* blockName = "NDEF");

    /**
     * @brief Prints all data of all offline profiles up to now
     */
    static void printOfflineProfiles();

    /**
     * @brief Clears the offline profile record
     */
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

    /**
     * @brief Hashes a string of maximum length 4 into an int uniquely
     *
     * @param str String of maximum length 4
     *
     * @return Unique hash of given string
     */
    static unsigned int hashStr(const char* str);

    /**
     * @brief Unhashes a string of length 4 from its unique hash
     *
     * @param hash Unique hash of length 4 string
     * @param output Preallocated buffer to write string to, must be at least 5 characters long
     */
    static void unhashStr(unsigned int hash, char* output);

    static Blk2Clk blocks;              ///< Names and beginning times of profile blocks
    static Blk2SMarker smoothBlocks;    ///< Names, beginning times and latest time slices of smoothed profile blocks
    static Blk2AMarker offlineBlocks;   ///< Names, beginning times, total times and number of samples of offline profile blocks

    static pthread_mutex_t lock;        ///< Locks normal profile record access
    static pthread_mutex_t smoothLock;  ///< Locks smooth profile record access
    static pthread_mutex_t offlineLock; ///< Locks offline profile record access
};

#ifdef ANDROID
#define EZP_OMNIPRINT(...) __android_log_print(ANDROID_LOG_INFO, EasyProfiler::androidTag.c_str(), __VA_ARGS__)
#else
#define EZP_OMNIPRINT(...) printf(__VA_ARGS__)
#endif

} /* namespace ezp */

#endif /* EASYPROFILER_HPP */

