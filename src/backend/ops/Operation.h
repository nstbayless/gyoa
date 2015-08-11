/*
 * Operation.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef BACKEND_OPS_OPERATION_H_
#define BACKEND_OPS_OPERATION_H_

#include <string>

#include "../git/GitOps.h"

namespace gyoa {
namespace model {
struct room_t;
struct world_t;
struct id_type;
struct option_t;
} /* namespace model */
} /* namespace gyoa */

namespace gyoa {
namespace ops {

/** operates on model data in backend/model*/
class Operation {
	using rm_id_t=model::id_type;
	using opt_id_t=model::id_type;
public:
	GitOps gitOps;
public:
	Operation();
	~Operation(){}

	//! sets model object to operate on
	void setModel(model::world_t& model);

	//! set directory for file i/o (must end in path separator to be directory)
	void setDataDirectory(std::string);

	//! loads world object contained in world.txt in data directory.
	//! (does not set Operation model to be this world.)
	//! if world does not exist, world is created.
	model::world_t loadWorld();

	//! fully loads operation model, including all rooms. (Rooms can be unloaded ptr data otherwise)
	//! (set operation model with Operation::setModel())
	//! Not recommended to call unless number of rooms is known to be small.
	void fullyLoad();

	//! retrieves room from given id. If no such id maps to a room, an error is thrown.
	model::room_t& loadRoom(rm_id_t);

	//! returns true if world or room was loaded, false if generated
	//! because file not found or if already loaded.
	bool loadResult();

	//! creates a room, incrementing world_t::next_gid and generating random rid.
	//! room is added to model.
	rm_id_t makeRoom();

	//! edit title of room
	void editRoomTitle(rm_id_t, std::string title);

	//! edit body of room
	void editRoomBody(rm_id_t, std::string body);

	//! edit dead-end state of room.
	void editRoomDeadEnd(rm_id_t, bool dead_end);

	//! add option to room
	void addOption(rm_id_t, model::option_t);

	//! remove option from room
	void removeOption(rm_id_t, opt_id_t);

	//! edit room option
	void editOption(rm_id_t, opt_id_t, model::option_t);

	//! saves room to file. Returns name of file saved to.
	std::string saveRoom(rm_id_t);

	//! saves loaded world to file.
	void saveWorld();

	//! saves all edited rooms and world.txt if applicable.
	//! returns a string describing save result in human-readable form
	std::string saveAll();

	//! returns true if model information has been edited since last save
	bool savePending();
private:
	//! throws exception if model not loaded.
	void checkModel();

	//! determines the filename (including data_dir path) of room for room_id
	std::string rm_id_to_filename(rm_id_t);
private:
	//! directory containing model data:
	std::string data_dir="data/";

	//! pointer to model object containing all game data
	model::world_t* model=nullptr;

	//! true if object was loaded, false if generated because file not found
	//! applies to loadWorld, loadRoom.
	//! retrieved by objectLoaded();
	bool load_result;
};

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_OPERATION_H_ */
