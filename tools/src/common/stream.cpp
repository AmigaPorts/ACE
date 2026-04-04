/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stream.h"

namespace nStream {

std::istream &getAnyLine(std::istream &Source, std::string &szLine)
{
	szLine.clear();
	char c;
	while(Source.good()) {
		Source.read(&c, 1);
		if(!Source.good()) {
			break;
		}

		if(c == '\n' || c == '\r') {
			break;
		}
		szLine += c;
	}

	return Source;
}

}
