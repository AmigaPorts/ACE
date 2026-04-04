// C++17 deprecation for codecvt still doesn't have substitute
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fs.h"
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#include <locale>
#include <codecvt>
#else
#include <dirent.h>
#endif

namespace nFs {

bool dirCreate(const std::string &szPath)
{
	#if defined(_WIN32)
		auto Error = _mkdir(szPath.c_str());
	#else
		auto Error = mkdir(szPath.c_str(),0733);
	#endif

	return Error != 0;
}

bool isDir(const std::string &szPath)
{
	struct stat info;

	if(stat( szPath.c_str(), &info ) != 0) {
			return false;
	}
	if(info.st_mode & S_IFDIR) {  // S_ISDIR() doesn't exist on my windows
			return true;
	}
	return false;
}

std::string getExt(const std::string &szPath)
{
	auto DotPos = szPath.find_last_of('.');
	if(DotPos == std::string::npos) {
		return "";
	}
	return szPath.substr(DotPos + 1);
}

std::string removeExt(const std::string &szPath)
{
	auto DotPos = szPath.find_last_of('.');
	if(DotPos == std::string::npos) {
		return "";
	}
	return szPath.substr(0, DotPos);
}

std::string getBaseName(const std::string &szPath)
{
	auto LastSlash = szPath.find_last_of('/');
	if(LastSlash == std::string::npos) {
		LastSlash = 0;
	}
	auto LastBackslash = szPath.find_last_of('\\');
	if(LastBackslash == std::string::npos) {
		LastBackslash = 0;
	}
	return szPath.substr(std::max(LastSlash, LastBackslash));
}

void iterateDirectory(const std::string &szPath, std::function<void(const std::string &)> onFile) {
#if defined(_WIN32)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(converter.from_bytes(szPath + "\\*").c_str(), &ffd);
	do {
		onFile(std::string(converter.to_bytes(reinterpret_cast<wchar_t*>(ffd.cFileName))));
	} while (FindNextFileW(hFind, &ffd) != 0);
#else
	DIR *pDir = opendir(szPath.c_str());
  if (pDir != nullptr) {
		dirent *pDirent;
    while ((pDirent = readdir(pDir)) != NULL) {
			onFile(std::string(pDirent->d_name));
    }
    closedir(pDir);
  }
#endif
}

} // namespace nFs
