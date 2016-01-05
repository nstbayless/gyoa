/*
 * meta.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: n
 */

#include "gyoa/meta.h"

#include <string>

namespace gyoa {

const std::string meta::BUILD_DATE = 		__DATE__;
const std::string meta::BUILD_TIME = 		__TIME__;
const std::string meta::NAME_CONDENSED = 	"gyoa";
const std::string meta::NAME_FULL = 		"Git-Your-Own-Adventure";
const std::string meta::VERSION = 			"0.3";

const char* getBuildDate() {
	return gyoa::meta::BUILD_DATE.c_str();
}

const char* getBuildTime() {
	return gyoa::meta::BUILD_TIME.c_str();
}

}
