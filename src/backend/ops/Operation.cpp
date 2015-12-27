/*
 * Operation.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#include "Operation.h"

#include <cassert>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>

#include "../context/Context.h"
#include "../error.h"
#include "../id_parse.h"
#include "../model/Room.h"
#include "../model/World.h"
#include "../fileio/FileIO.h"

namespace gyoa {
namespace ops {

using rm_id_t=model::id_type;
using opt_id_t=model::id_type;

rm_id_t makeRoom(model::ActiveModel* activeModel) {
	model::world_t* model = &activeModel->world;
	assert(model);
	rm_id_t id;
	id.gid=model->next_rm_gid++;
	id.rid=random();
	//first room must be {0,0}:
	if (id.gid==0)
		id.rid=0;
	model->rooms[id]=model::room_t();

	//update model-edited information:
	model->rooms[id].edited=true;
	model->edited=true;

	return id;
}

void editRoomTitle(model::ActiveModel* activeModel,rm_id_t id, std::string title) {
	model::world_t* model = &activeModel->world;
	assert(model);

	//trim input:
	title.erase(title.find_last_not_of(" \n\r\t")+1);

	getRoom(activeModel,id).title=title;

	//update model-edited information:
	getRoom(activeModel,id).edited=true;
}

void editRoomBody(model::ActiveModel* activeModel,rm_id_t id, std::string body) {
	model::world_t* model = &activeModel->world;
	assert(model);

	//trim input:
	body.erase(body.find_last_not_of(" \n\r\t")+1);

	getRoom(activeModel,id).body=body;

	//update model-edited information:
	getRoom(activeModel,id).edited=true;
}

void addOption(model::ActiveModel* activeModel,rm_id_t id,
		model::option_t option) {
	model::world_t* model = &activeModel->world;
	assert(model);

	auto& options = getRoom(activeModel,id).options;
	//add option with gid = highest gid + 1.
	int max_opt_gid=0;
	//find highest gid
	for (auto iter : options)
		if (iter.first.gid>max_opt_gid)
			max_opt_gid=iter.first.gid;

	//trim input:
	option.option_text.erase(option.option_text.find_last_not_of(" \n\r\t")+1);

	//create id for option by
	int rnd_nr = random(); // narrowing allowed here from long to int
	options[{max_opt_gid+1, rnd_nr}]=option;

	//update model-edited information:
	getRoom(activeModel,id).edited=true;
}

void removeOption(model::ActiveModel* activeModel,rm_id_t rid, opt_id_t oid) {
	model::world_t* model = &activeModel->world;
	assert(model);
	getRoom(activeModel,rid).options.erase(oid);

	//update model-edited information:
	getRoom(activeModel,rid).edited=true;
}

void editOption(model::ActiveModel* activeModel,rm_id_t rid, opt_id_t oid,
		model::option_t option) {
	model::world_t* model = &activeModel->world;
	assert(model);

	getRoom(activeModel,rid).options[oid]=option;

	//update model-edited information:
	getRoom(activeModel,rid).edited=true;
}

void editRoomDeadEnd(model::ActiveModel* activeModel,rm_id_t id, bool dead_end) {
	model::world_t* model = &activeModel->world;
	assert(model);

	getRoom(activeModel,id).dead_end=dead_end;

	//update model-edited information:
	getRoom(activeModel,id).edited=true;
}


std::string saveRoom(model::ActiveModel* activeModel,rm_id_t id) {
	model::world_t* model = &activeModel->world;
	assert(model);
	assert(activeModel->path.length()>1);

	std::string file=rm_id_to_filename(id,activeModel->path);
	FileIO::writeRoomToFile(getRoom(activeModel,id),file);

	//reset model-edited information:
	getRoom(activeModel,id).edited=false;

	return file;
}

void saveWorld(model::ActiveModel* activeModel) {
	model::world_t* model = &activeModel->world;
	assert(model);
	assert(activeModel->path.length()>1);

	FileIO::writeWorldToFile(*model,activeModel->path+"world.txt");
	FileIO::writeGitignore(activeModel->path+".gitignore.txt");;

	//reset model-edited information:
	model->edited=false;
}

std::string saveAll(model::ActiveModel* activeModel,bool force) {
	model::world_t* model = &activeModel->world;
	assert(model);
	assert(activeModel->path.length()>1);

	std::string result="";
	//true if something is saved
	bool anything_saved=false;

	if (model->edited||force) {
		saveWorld(activeModel);
		result+="saved world file.\n";
		anything_saved=true;
	}

	for (auto iter : model->rooms)
		if (iter.second.edited||force) {
			result+="saved scenario " + write_id(iter.first) + " to " + saveRoom(activeModel,iter.first)+"\n";
			anything_saved=true;
		}

	if (anything_saved)
		result+="done.";
	else
		result+="nothing to be done.";

	return result;
}

bool savePending(model::ActiveModel* activeModel) {
	model::world_t* model = &activeModel->world;
	assert(model);

	if (model->edited)
		return true;
	for (auto iter : model->rooms)
		if (iter.second.edited)
			return true;
	return false;
}

void editStartRoom(model::ActiveModel* am, rm_id_t id) {
	assert(model::roomExists(am,id));
	am->world.first_room=id;
	am->world.edited=true;
}

} /* namespace ops */
} /* namespace gyoa */
