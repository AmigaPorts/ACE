/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_EXCEPTION_H_
#define _ACE_TOOLS_COMMON_EXCEPTION_H_

#include <exception>
#include <string>
#include "logging.h"

void exceptionHandle(
	const std::exception &Ex, const std::string &szOperation
);

#endif // _ACE_TOOLS_COMMON_EXCEPTION_H_
