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
 * @file ezp_control.cpp
 * @brief Command line controller for external EZP sessions
 * @author Ayberk Özgür
 * @version 1.0
 * @date 2014-10-30
 */

#include<iostream>
#include<getopt.h>

#include"ezp.hpp"

void printHelp(bool desc){
    using namespace std;
    cout << "Usage: ezp_control [OPTION]" << endl;
    if(desc)
        cout << "Controls an already running EZP session in an external process." << std::endl;
    cout << endl;
    cout << "  -e, --enable     Enables the EZP session present on this machine" << std::endl;
    cout << "  -d, --disable    Disables the EZP session present on this machine" << std::endl;
    cout << "  -h, --help       Displays this message" << std::endl;
}

int main(int argc, char** argv){
    struct option options[] = {
        {"enable",  no_argument,    NULL, 'e'},
        {"disable", no_argument,    NULL, 'd'},
        {"help",    no_argument,    NULL, 'h'}
    };

    int i = 0;
    while (true)
        switch(getopt_long(argc, argv, "edh", options, &i)){
            case 'e':
                EZP_FORCE_STDERR_ON
                EZP_ENABLE_EXTERNAL
                return 0;
            case 'd':
                EZP_FORCE_STDERR_ON
                EZP_DISABLE_EXTERNAL
                return 0;
            case 'h':
                printHelp(true);
                return 0;
            case -1:
                printHelp(false);
                return 0;
            case '?':
                return -1;
            default:
                return -1;
        }
}

