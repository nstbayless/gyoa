/*
 * Operation.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef BACKEND_OPS_OPERATION_H_
#define BACKEND_OPS_OPERATION_H_

#include <string>
#include "gyoa/Context.h"

namespace gyoa {
namespace model {
struct room_t;
struct world_t;
struct id_type;
struct option_t;
struct ActiveModel;
} /* namespace model */

namespace ops {
	using rm_id_t=model::id_type;
	using opt_id_t=model::id_type;

extern "C" {
	//! creates a room, incrementing world_t::next_gid and generating random rid.
	//! room is added to model.
	rm_id_t makeRoom(model::ActiveModel*);

	//! changes the first room to point to the given room.
	void editStartRoom(model::ActiveModel*,rm_id_t first_room);

	//! edit title of room
	void editRoomTitle(model::ActiveModel*,rm_id_t, const char* title);

	//! edit body of room
	void editRoomBody(model::ActiveModel*,rm_id_t, const char* body);

	//! edit dead-end state of room.
	void editRoomDeadEnd(model::ActiveModel*,rm_id_t, bool dead_end);

	//! add option to room
	opt_id_t addOption(model::ActiveModel*,rm_id_t, const char* text, rm_id_t dst);

	//! remove option from room
	void removeOption(model::ActiveModel*,rm_id_t, opt_id_t);

	//! edit room option
	void editOption(model::ActiveModel*,rm_id_t, opt_id_t, model::option_t replace);

	//! edits option description
	void editOptionDescription(model::ActiveModel*,rm_id_t,opt_id_t, const char* newdescription);

	//! edits option destination
	void editOptionDestination(model::ActiveModel*,rm_id_t,opt_id_t, rm_id_t newdestination);

	//! saves room to file. Returns name of file saved to.
	std::string saveRoom(model::ActiveModel*,rm_id_t);

	//! saves world data to file world.txt
	void saveWorld(model::ActiveModel*);

	//! saves all edited rooms and world.txt if applicable.
	//! returns a string describing save result in human-readable form
	//! if full is true, saves everything regardless of edit flag.
	const char* saveAll(model::ActiveModel*,bool full);

	//! returns true if model information has been edited since last save
	bool savePending(model::ActiveModel*);
}

//! saves all edited rooms and world.txt if applicable.
//! returns a string describing save result in human-readable form
std::string saveAll(model::ActiveModel*);

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_OPERATION_H_ */
