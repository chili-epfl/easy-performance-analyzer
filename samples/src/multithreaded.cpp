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
 * @file multithreaded.cpp
 * @brief easy-profiler demo that shows multithreading support
 * @author Ayberk Özgür
 * @date 2014-10-19
 */

#include<ctime>
#include<pthread.h>

#include<EasyProfiler.hpp>

void* run(void* id){
    int* y = new int;

    EZP_START_OFFLINE("WHOLE_THING")
    for(int i=0;i<100;i++){

        EZP_START_OFFLINE("LOOP_1")
        for(int j=0;j<10000000;j++){
            *y += 28138481u;
            *y = 623415232 % *y;
        }
        EZP_END_OFFLINE("LOOP_1")

        EZP_START_OFFLINE("LOOP_2")
        for(int j=0;j<10000000;j++){
            *y += 51;
            *y = *y % 101;
        }
        EZP_END_OFFLINE("LOOP_2")

    }
    EZP_END_OFFLINE("WHOLE_THING")

    pthread_exit(NULL);
}

int main(){
    int N = 4;
    pthread_t threads[N];

    for(int i=0;i<N;i++)
        pthread_create(&threads[i],NULL,run,NULL);

    for(int i=0;i<N;i++)
        pthread_join(threads[i],NULL);

    EZP_PRINT_OFFLINE

    return 0;
}

