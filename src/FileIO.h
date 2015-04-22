/*
 * Fileio.h
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>

#include "Room.h"

class FileIO {
	std::string data_path = "./data";
public:
	FileIO();
	virtual ~FileIO();
	void writeRoomToFile(room_t room, std::string filename="") const;
	room_t loadRoom(std::string filename="") const;
	room_t loadRoom(room_id id) const;
};

#endif /* FILEIO_H_ */
