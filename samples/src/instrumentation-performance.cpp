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
 * @file instrumentation-performance.cpp
 * @brief easy-performance-analyzer demo that shows the performance of the instrumentation calls themselves
 * @author Ayberk Özgür
 * @date 2014-10-19
 */

#include<ctime>
#include<cstdio>
#include<climits>

#include<ezp.hpp>

#define CLK CLOCK_THREAD_CPUTIME_ID

#if defined(EZP_SAMPLE_REALTIME)
    #define EZP_SAMPLE_START EZP_START
    #define EZP_SAMPLE_END  EZP_END
#elif defined(EZP_SAMPLE_SMOOTHED)
    #define EZP_SAMPLE_START EZP_START_SMOOTH
    #define EZP_SAMPLE_END EZP_END_SMOOTH
#else
    #define EZP_SAMPLE_START EZP_START_OFFLINE
    #define EZP_SAMPLE_END EZP_END_OFFLINE
#endif

#define EZP_SAMPLE_MEASURE_BEGIN    clock_gettime(CLK,&t1);
#define EZP_SAMPLE_MEASURE_END      clock_gettime(CLK,&t2); t = (t2.tv_sec - t1.tv_sec)*1000000000LL + t2.tv_nsec - t1.tv_nsec;
#define EZP_META_MEASURE_BEGIN      clock_gettime(CLK,&mt1);
#define EZP_META_MEASURE_END        clock_gettime(CLK,&mt2); mt = (mt2.tv_sec - mt1.tv_sec)*1000000000LL + mt2.tv_nsec - mt1.tv_nsec;

const int nsamples = 1000;
const int nblocks = 1000;
const int omsamples = 10;
const int dsamples = 10;

struct timespec t1,t2,mt1,mt2;
char cbuf[5];
unsigned long long int avgstarttimes[nblocks];
unsigned long long int avgendtimes[nblocks];
unsigned long long int minstarttime[nblocks];
unsigned long long int minendtime[nblocks];
unsigned long long int maxstarttime[nblocks];
unsigned long long int maxendtime[nblocks];
unsigned long long int t,mt;
unsigned long long int overhead = UINT_MAX;

inline void measureAndDiscard(const char* argv_0){
    for(int s=0;s<dsamples;s++){
        for(int i=0;i<nsamples;i++)
            for(int b=0;b<nblocks;b++){
                sprintf(cbuf,"%d",b);

                EZP_SAMPLE_MEASURE_BEGIN
                EZP_SAMPLE_START(cbuf)
                EZP_SAMPLE_MEASURE_END

                EZP_SAMPLE_MEASURE_BEGIN
                EZP_SAMPLE_END(cbuf)
                EZP_SAMPLE_MEASURE_END

            }

        printf("%s: Intermediate measurement no.%d, discarding\n", argv_0, s);
    }
}

inline void measure(){
    for(int i=0;i<nsamples;i++)
        for(int b=0;b<nblocks;b++){
            sprintf(cbuf,"%d",b);

            EZP_SAMPLE_MEASURE_BEGIN
            EZP_SAMPLE_START(cbuf)
            EZP_SAMPLE_MEASURE_END

            if(t < overhead)
                t = 0;
            else
                t -= overhead;
            avgstarttimes[b] += t;
            if(t > maxstarttime[b])
                maxstarttime[b] = t;
            if(t < minstarttime[b])
                minstarttime[b] = t;

            EZP_SAMPLE_MEASURE_BEGIN
            EZP_SAMPLE_END(cbuf)
            EZP_SAMPLE_MEASURE_END

            if(t < overhead)
                t = 0;
            else
                t -= overhead;
            avgendtimes[b] += t;
            if(t > maxendtime[b])
                maxendtime[b] = t;
            if(t < minendtime[b])
                minendtime[b] = t;
        }
}

__attribute__((optimize("unroll-loops"))) inline unsigned long long int getMeasurementOverhead(){

    //We assume that placing one clock_gettime() before and one clock_gettime()
    //after the measured block will incur approximately one clock_gettime()
    //overhead, so one clock_gettime() overhead will be subtracted from
    //measurements

    EZP_META_MEASURE_BEGIN

    for(int i=0;i<nsamples*nblocks;i++){
        clock_gettime(CLK,&t1);
        clock_gettime(CLK,&t2);
        clock_gettime(CLK,&t1);
        clock_gettime(CLK,&t2);
    }

    EZP_META_MEASURE_END

    return mt/4/(nsamples*nblocks);
}

int main(int argc, char** argv){
    for(int b=0;b<nblocks;b++){
        avgstarttimes[b] = 0;
        avgendtimes[b] = 0;
        minstarttime[b] = UINT_MAX;
        minendtime[b] = UINT_MAX;
        maxstarttime[b] = 0;
        maxendtime[b] = 0;
    }

    EZP_ENABLE

    //
    //Measure measurement overhead
    //

    printf("%s: Measuring measurement overhead, will collect %d samples and take minimum\n", argv[0], omsamples);

    //Taking minimum of 10 overhead measurements
    for(int i=0;i<omsamples;i++){
        unsigned long long int p = getMeasurementOverhead();
        printf("%s: Intermediate overhead measurement no.%d: %lld ns\n", argv[0], i, p);
        if(p < overhead)
            overhead = p;
    }

    printf("%s: Measurement overhead is measured as %lld ns\n", argv[0], overhead);

    //
    //Measure instrumentation performance
    //

    printf("%s: Beginning instrumentation performance measurement, will sample and discard %d sets first\n", argv[0], dsamples);

    measureAndDiscard(argv[0]);

    measure();

    //Print statistics
    printf("%s: ==============================================\n", argv[0]);
    for(int b=0;b<nblocks;b++){
        avgstarttimes[b] /= nsamples;
        avgendtimes[b] /= nsamples;
        printf("%s: Block %4d -- average start: %6lld ns, max start: %6lld ns, min start: %6lld ns, average end: %6lld ns, max end: %6lld ns, min end: %6lld ns\n",
                argv[0], b, avgstarttimes[b], maxstarttime[b], minstarttime[b], avgendtimes[b], maxendtime[b], minendtime[b]);
    }

    //Print summed statistics
    printf("%s: ==============================================\n", argv[0]);
    unsigned long long int maxtime = 0, mintime = UINT_MAX;
    unsigned long long int avgtime = 0;
    for(int b=0;b<nblocks;b++){
        if(maxstarttime[b] > maxtime)
            maxtime = maxstarttime[b];
        if(maxendtime[b] > maxtime)
            maxtime = maxendtime[b];
        if(minstarttime[b] < mintime)
            mintime = minstarttime[b];
        if(minendtime[b] < mintime)
            mintime = minendtime[b];
        avgtime += avgendtimes[b] + avgstarttimes[b];
    }
    avgtime /= 2*nblocks;
    printf("%s: Overall -- average: %6lld ns, max: %6lld ns, min: %6lld ns\n", argv[0], avgtime, maxtime, mintime);

    return 0;
}

