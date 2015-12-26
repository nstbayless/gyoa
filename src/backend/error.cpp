/*
 * error.cpp
 *
 *  Created on: Dec 26, 2015
 *      Author: n
 */

#include "model/World.h"
#include "error.h"

void checkModelDirectory(gyoa::model::ActiveModel* am) {
	if (!am->path.length()) {
		throw DirectoryInvalidException(am->path);
	}
}
