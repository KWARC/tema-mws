/*

Copyright (C) 2010-2013 KWARC Group <kwarc.info>

This file is part of MathWebSearch.

MathWebSearch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MathWebSearch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MathWebSearch.  If not, see <http://www.gnu.org/licenses/>.

*/
/** 
 *  @file xhtml2harvests.cpp
 *  @author Corneliu C Prodescu <cprodescu@gmail.com>
 *
 *  Harvest XHTML files given as command line arguments.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <vector>
#include <queue>
#include <set>
#include <fstream>
using namespace std;

#include "crawler/parser/MathParser.hpp"
using namespace crawler::parser;

#include "common/utils/FlagParser.hpp"
#include "common/utils/util.hpp"
using namespace common::utils;


int main(int argc, char *argv[])
{
    string outdir = ".";
    string root   = "";
    FILE* harvest = NULL;

    FlagParser::addFlag('O', "outdir",  FLAG_OPT, ARG_REQ);
    FlagParser::addFlag('R', "root",    FLAG_OPT, ARG_REQ);
    FlagParser::setMinNumParams(1);

    if (FlagParser::parse(argc, argv) != 0) {
        fprintf(stderr, "%s", FlagParser::getUsage().c_str());
        return EXIT_FAILURE;
    }

    if (FlagParser::hasArg('O')) outdir = FlagParser::getArg('O');
    if (FlagParser::hasArg('R')) root = FlagParser::getArg('R');


    string harvest_templ_str = outdir + "/harvest_XXXXXX.harvest";
    char* harvest_templ = strdup(harvest_templ_str.c_str());
    int harvest_fd = mkstemps(harvest_templ, /* suffixlen = */ 8);
    if (harvest_fd < 0) {
        perror("mkstemp");
        goto failure;
    }

    harvest = fdopen(harvest_fd, "w");
    if (harvest == NULL) {
        perror("fdopen");
        goto failure;
    }

    fputs("<?xml version=\"1.0\" ?>\n"
          "<mws:harvest xmlns:m=\"http://www.w3.org/1998/Math/MathML\"\n"
          "             xmlns:mws=\"http://search.mathweb.org/ns\">\n",
          harvest);
    {
        const vector<string>& files = FlagParser::getParams();
        for (const string& file : files) {
            string url = root + file;
            string content = getFileContents(file.c_str());
            vector<string> mathElements = getHarvestFromXhtml(content, url);
            if (mathElements.size() == 0) {
                fprintf(stderr, "%s: %s: Could not find any Content MathML\n",
                        argv[0], file.c_str());
                continue;
            }
            for (const string& mathElement : mathElements) {
                fputs(mathElement.c_str(), harvest);
            }
        }
    }
    fputs("</mws:harvest>\n", harvest);

    fclose(harvest);

    return EXIT_SUCCESS;

failure:
    if (harvest) fclose(harvest);

    return EXIT_FAILURE;
}
