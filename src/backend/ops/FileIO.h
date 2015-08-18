/*
 * Fileio.h
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>

namespace gyoa {
namespace model {
struct world_t;
struct id_type;
struct room_t;
} /* namespace model */


//! collection of file IO methods.
//todo: should be a namespace not a class.
class FileIO {
public:
	FileIO();
	virtual ~FileIO();
	void writeRoomToFile(gyoa::model::room_t room, std::string filename) const;
	model::room_t loadRoom(std::string filename) const;
	model::room_t loadRoomFromText(std::string text) const;
	void writeWorldToFile(gyoa::model::world_t&, std::string filename);
	gyoa::model::world_t loadWorld(std::string filename);
	gyoa::model::world_t loadWorldFromText(std::string text);
};
}

#endif /* FILEIO_H_ */
