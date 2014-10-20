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

int main(){
    const int nsamples = 1000;
    const int nblocks = 1000;

    struct timespec t1,t2;
    char cbuf[5];
    unsigned long long int avgstarttimes[nblocks];
    unsigned long long int avgendtimes[nblocks];
    unsigned long long int minstarttime[nblocks];
    unsigned long long int minendtime[nblocks];
    unsigned long long int maxstarttime[nblocks];
    unsigned long long int maxendtime[nblocks];

    for(int b=0;b<nblocks;b++){
        avgstarttimes[b] = 0;
        avgendtimes[b] = 0;
        minstarttime[b] = UINT_MAX;
        minendtime[b] = UINT_MAX;
        maxstarttime[b] = 0;
        maxendtime[b] = 0;
    }

    EZP_ENABLE

    //Measure nblocks many blocks, nsamples times each
    unsigned int t;
    for(int i=0;i<nsamples;i++)
        for(int b=0;b<nblocks;b++){
            sprintf(cbuf,"%d",b);

            clock_gettime(CLK,&t1);
            EZP_SAMPLE_START(cbuf)
            clock_gettime(CLK,&t2);
            t = (t2.tv_sec - t1.tv_sec)*1000000000l + t2.tv_nsec - t1.tv_nsec;
            avgstarttimes[b] += t;
            if(t > maxstarttime[b])
                maxstarttime[b] = t;
            if(t < minstarttime[b])
                minstarttime[b] = t;

            clock_gettime(CLK,&t1);
            EZP_SAMPLE_END(cbuf)
            clock_gettime(CLK,&t2);
            t = (t2.tv_sec - t1.tv_sec)*1000000000l + t2.tv_nsec - t1.tv_nsec;
            avgendtimes[b] += t;
            if(t > maxendtime[b])
                maxendtime[b] = t;
            if(t < minendtime[b])
                minendtime[b] = t;
        }

    //Print statistics
    for(int b=0;b<nblocks;b++){
        avgstarttimes[b] /= nsamples;
        avgendtimes[b] /= nsamples;
        printf("Block %4d -- average start: %6lld ns, max start: %6lld ns, min start: %6lld ns, average end: %6lld ns, max end: %6lld ns, min end: %6lld ns\n",
                b, avgstarttimes[b], maxstarttime[b], minstarttime[b], avgendtimes[b], maxendtime[b], minendtime[b]);
    }

    printf("==============================================\n");

    //Print summed statistics
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
    printf("Overall -- average: %6lld ns, max: %6lld ns, min: %6lld ns\n",avgtime,maxtime,mintime);

    return 0;
}

