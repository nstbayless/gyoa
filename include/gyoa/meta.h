/*
 * meta.h
 *
 *  Created on: Aug 12, 2015
 *      Author: n
 */

#ifndef META_H_
#define META_H_

#include <string>

namespace gyoa {

struct meta {

static const std::string BUILD_DATE;
static const std::string BUILD_TIME;
static const std::string NAME_CONDENSED;
static const std::string NAME_FULL;
static const std::string VERSION;

};

extern "C" {

const char* getBuildDate();

const char* getBuildTime();
}

}
#endif /* META_H_ */
