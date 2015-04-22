/*
 * Room.h
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#ifndef ROOM_H_
#define ROOM_H_

#include <string>
#include <vector>

typedef unsigned int room_id;

struct option_t {
	//the id of the room this option leads to:
	room_id dst;
	std::string option_text;
};

struct room_t {
	room_id id=-1;
	std::string title;
	std::string body;
	std::vector<option_t> options;
};

#endif /* ROOM_H_ */
