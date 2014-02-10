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
#ifndef _COMMON_UTILS_UTIL_HPP
#define _COMMON_UTILS_UTIL_HPP

/**
  * @file util.hpp
  * @date 22 Nov 2013
  *
  * License: GPL v3
  */

#include <string>
#include <functional>

namespace common { namespace utils {

bool hasSuffix(const std::string& str, const std::string& suffix);

std::string
getFileContents(const std::string& path);

void
putFileContents(const std::string& path, const std::string& contents);

typedef std::function<int (const std::string fullPath,
                           const std::string partialDirectoryPath)>
FileCallback;
typedef std::function<bool (const std::string partialPath)> DirectoryCallback;

static
DirectoryCallback ignore_directories = [](const std::string partialPath) {
    printf("Skipping directory %s\n", partialPath.c_str());
    return false;
};

int
foreachEntryInDirectory(const std::string& path,
        const FileCallback& fileCallback,
        const DirectoryCallback& directoryCallback = ignore_directories);

} }

#endif // _COMMON_UTILS_UTIL_HPP
