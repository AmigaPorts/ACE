/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_STREAM_H_
#define _ACE_TOOLS_COMMON_STREAM_H_

#include <istream>

namespace nStream {

std::istream &getAnyLine(std::istream &Source, std::string &szLine);

}

#endif _ACE_TOOLS_COMMON_STREAM_H_
