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
 * @file external-control.cpp
 * @brief easy-performance-analyzer demo that demonstrates the usage of ezp_control to enable analysis from outside the process
 * @author Ayberk Özgür
 * @date 2014-10-17
 */

#include<ezp.hpp>

int main(int argc, char** argv){
    int* y = new int;

    printf("%s: Instrumented code running, run `ezp_control --enable` to remotely enable instrumentation, `ezp_control --disable` to remotely disable instrumentation, `ezp_control --print` to print offline analysis information and `ezp_control --clear` to clear offline analysis history on demand.\n",argv[0]);

    while(true){

        EZP_START_OFFLINE("OFLN")
        for(int j=0;j<10000000;j++){
            *y += 51;
            *y = *y % 101;
        }
        EZP_END_OFFLINE("OFLN")

        EZP_START_SMOOTH("REAL")
        for(int j=0;j<10000000;j++){
            *y += 51;
            *y = *y % 101;
        }
        EZP_END_SMOOTH("REAL")

    }

    return 0;
}

