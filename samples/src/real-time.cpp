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
 * @file real-time.cpp
 * @brief easy-profiler demo that shows basic real-time usage
 * @author Ayberk Özgür
 * @date 2014-10-17
 */

#include<EasyProfiler.hpp>

int main(){
    int* y = new int;

    EZP_START("WHOLE_THING")
    for(int i=0;i<100;i++){

        EZP_START("LOOP_1")
        for(int j=0;j<10000000;j++){
            *y += 28138481u;
            *y = 623415232 % *y;
        }
        EZP_END("LOOP_1")

        EZP_START_SMOOTH("LOOP_2")
        for(int j=0;j<10000000;j++){
            *y += 51;
            *y = *y % 101;
        }
        EZP_END_SMOOTH("LOOP_2")

    }
    EZP_END("WHOLE_THING")

    return 0;
}

