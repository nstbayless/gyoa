/*
 * Fileio.h
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>

namespace cyoa {
namespace model {
struct world_t;
struct id_type;
struct room_t;
} /* namespace model */

class FileIO {
public:
	FileIO();
	virtual ~FileIO();
	void writeRoomToFile(cyoa::model::room_t room, std::string filename) const;
	model::room_t loadRoom(std::string filename="") const;
	void writeWorldToFile(cyoa::model::world_t&, std::string filename);
	cyoa::model::world_t loadWorld(std::string filename);
};
}

#endif /* FILEIO_H_ */
