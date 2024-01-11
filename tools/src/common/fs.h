/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// C++17 has <filesystem> but it ain't working on GCC7 so we need something else

#ifndef _ACE_TOOLS_COMMON_FS_H_
#define _ACE_TOOLS_COMMON_FS_H_

#include <string>

namespace nFs {

bool dirCreate(const std::string &szPath);

bool isDir(const std::string &szPath);

std::string getExt(const std::string &szPath);

std::string removeExt(const std::string &szPath);

std::string getBaseName(const std::string &szPath);

} // namespace nFs

#endif // _ACE_TOOLS_COMMON_FS_H_
