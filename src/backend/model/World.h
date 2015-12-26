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

struct git_repository;

namespace gyoa {

namespace model {

//! Model root. contains list of rooms.
struct world_t {
	//! name of the gyoa world.
	std::string title="";

	//! the next id_type::gid to be assigned on creation of a new room.
	unsigned int next_rm_gid=0;

	//!rm_id_t of starting room.
	id_type first_room=rm_id_t::null();

	//! list of rooms, mapped from id_type to room_t.
	std::map<id_type,room_t> rooms;

	//! not part of model, used to efficiently save data
	bool edited=false;
};

/**
 * Contains a path, the world model for
 * the given path, git repo data, and
 * other data pertaining to
 * active instances of a model in use
 */
struct ActiveModel {
	ActiveModel(){}
	//! directory where model is saved/loaded
	std::string path;
	//! model instance including all unsaved changes
	model::world_t model;
	git_repository* repo=nullptr;
private:
//non-copyable:
	ActiveModel & operator=(const ActiveModel&) = delete;
	ActiveModel (const ActiveModel&) = delete;
};

//! creates a new model (with default parameters). Must be freed with freeModel()
ActiveModel* makeModel(std::string directory);

//! frees an instance of ActiveModel
void freeModel(ActiveModel*);

//! checks if a model exists at directory/world.txt
bool directoryContainsModel(std::string dir);

//! checks if any files are in the directory
bool directoryInUse(std::string dir);

//! loads the model from the given directory (creating a new model if no model is saved there)
//! should be freed with freeModel
ActiveModel* loadModel(std::string dir);

//! preloads all rooms (rather than loading them on-the-fly)
//! returns number of rooms loaded
int loadAllRooms(ActiveModel*);

//! returns true if a room exists with the given ID
bool roomExists(ActiveModel*,rm_id_t);

//! loads the room with the given id
//! should not be modified (will be const in future versions)
room_t& loadRoom(ActiveModel*,rm_id_t);

//! retrieves nth option by id from given room, where n is from 1 to 9.
//! returns opt_id_t::null() if no option found.
//! should not be modified (will be const in future versions)
opt_id_t getOption(room_t& room, int n);

//! returns path to the given room
std::string rm_id_to_filename(rm_id_t id, std::string root_dir);

}
}

#endif /* BACKEND_MODEL_WORLD_H_ */
