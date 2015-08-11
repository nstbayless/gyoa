/*
 * World.h
 *
 *  Created on: Aug 9, 2015
 *      Author: n
 */

#ifndef BACKEND_MODEL_WORLD_H_
#define BACKEND_MODEL_WORLD_H_

#include <map>
#include <string>

#include "Room.h"

namespace cyoa {

namespace model {

//! Model root. contains list of rooms.
struct world_t {
	//! name of the cyoa world.
	std::string title;

	//! the next id_type::gid to be assigned on creation of a new room.
	unsigned int next_rm_gid;

	//!rm_id_t of starting room.
	id_type first_room;

	//! list of rooms, mapped from id_type to room_t.
	std::map<id_type,room_t> rooms;

	//! true if world_t information has been edited since last save
	bool edited=false;
};

}
}

#endif /* BACKEND_MODEL_WORLD_H_ */
