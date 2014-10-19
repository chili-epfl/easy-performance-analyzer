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
 * @file compiler-optimization.cpp
 * @brief easy-profiler demo that shows effect of compiler optimizations on code speed
 * @author Ayberk Özgür
 * @date 2014-10-17
 */

#include<EasyProfiler.hpp>

int main(){
    int* y = new int;

    EZP_ENABLE

    for(int i=0;i<100;i++){

        EZP_START_SMOOTH()
        for(int j=0;j<10000000;j++){
            *y += 51;
            *y = *y % 101;
            *y += 28138481u;
            *y = 623415232 % *y;
        }
        EZP_END_SMOOTH()

    }
    return 0;
}

