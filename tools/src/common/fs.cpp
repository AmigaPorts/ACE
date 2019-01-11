/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fs.h"
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nFs {

bool dirCreate(const std::string &szPath) {
	#if defined(_WIN32)
		auto Error = CreateDirectoryA(szPath.c_str(), 0);
	#else
		auto Error = mkdir(sPath.c_str(),0733);
	#endif

	return Error != 0;
}

bool isDir(const std::string &szPath) {
	struct stat info;

	if(stat( szPath.c_str(), &info ) != 0) {
			return false;
	}
	if(info.st_mode & S_IFDIR) {  // S_ISDIR() doesn't exist on my windows
			return true;
	}
	return false;
}

} // namespace nFs
