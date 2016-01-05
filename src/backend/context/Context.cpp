/*
 * Context.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: n
 */

#include "../fileio/FileIO.h"
#include "gyoa/error.h"

#include "gyoa/Context.h"

namespace gyoa {
namespace context {

context_t loadContext(std::string data_dir) {
	try {
	return FileIO::loadContext(data_dir);
	} catch (FileNotFoundException& e) {
		return context::context_t();
	}
}

void saveContext(context_t context, std::string path) {
	FileIO::writeContext(context,path);
}

} /* namespace context */
} /* namespace gyoa */
