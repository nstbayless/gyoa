/*
 * driver.cpp
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#include <string>

#include "FileIO.h"
#include "Room.h"

int main() {
	room_t r;
	r.body = "test test test";
	r.id = 0;
	r.title = "testtitle";
	FileIO fio;
	fio.writeRoomToFile(r);
	room_t rb = fio.loadRoom(0);
}
