/*
 * Fileio.h
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace gyoa {
namespace context {
struct context_t;
} /* namespace context */
namespace model {
struct world_t;
struct id_type;
struct room_t;
} /* namespace model */

namespace FileIO {
//! collection of file IO methods.
//todo: should be a namespace not a class.

//! retrieves canonical paths to all files within the given directory (non-recursive)
std::vector<std::string> getAllFiles(std::string directory);
//! retrieves name of file from longer path.
std::string getFilename(std::string filepath);
void deletePath(std::string path);

void writeRoomToFile(gyoa::model::room_t room, std::string filename);
model::room_t loadRoom(std::string filename);
model::room_t loadRoomFromText(std::string text);
void writeWorldToFile(gyoa::model::world_t&, std::string filename);
gyoa::model::world_t loadWorld(std::string filename);
gyoa::model::world_t loadWorldFromText(std::string text);
void writeGitignore(std::string filename);
void writeContext(context::context_t, std::string filename);
context::context_t loadContext(std::string filename);
}
}

#endif /* FILEIO_H_ */
