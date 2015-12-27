/*
 * Context.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: n
 */

#include "../fileio/FileIO.h"
#include "../error.h"

#include "Context.h"

namespace gyoa {
namespace context {

context_t loadContext(std::string data_dir) {
	try {
	return FileIO::loadContext(data_dir+"context.txt");
	} catch (FileNotFoundException& e) {
		return context::context_t();
	}
}

void saveContext(context_t context, std::string data_dir) {
	FileIO::writeContext(context,data_dir+"context.txt");
}

} /* namespace context */
} /* namespace gyoa */
