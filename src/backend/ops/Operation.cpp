/*
 * Operation.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#include "Operation.h"

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <map>
#include <utility>

#include "../error.h"
#include "../git/GitOps.h"
#include "../id_parse.h"
#include "../model/Room.h"
#include "FileIO.h"

namespace gyoa {
namespace ops {

using rm_id_t=model::id_type;
using opt_id_t=model::id_type;

Operation::Operation(bool use_git) {
	using namespace std;
	srand(time(nullptr));
	if (use_git) {
		gitOps=new GitOpsWithTmp(this);
		gitOps->setLocalRepoDirectory(data_dir);
	}
}

Operation::~Operation() {
	if (gitOps)
		delete(gitOps);
}


void Operation::setModel(model::world_t& model) {
	this->model=&model;
}

void Operation::setDataDirectory(std::string dir) {
	this->data_dir=dir;
	if (gitOps)
		gitOps->setLocalRepoDirectory(data_dir);
}

model::world_t Operation::loadWorld(bool autogen) {
	FileIO io;
	try {
	load_result=true;
	return io.loadWorld(data_dir+"world.txt");
	} catch (FileNotFoundException e) {
		load_result=false;
		//no world exists; create one:
		model::world_t world;
		if (autogen){
			world.title="untitled world";
			world.first_room={0,0};
			world.next_rm_gid=1;
			world.rooms[{0,0}]=model::room_t();
			//make sure starting data is saved:
			world.rooms[{0,0}].edited=true;
			world.edited=true;
		}
		return world;
	}
}

void Operation::loadAll() {
	reload();
	for (auto iter : model->rooms)
		loadRoom(iter.first);
}

void Operation::loadAllUnloaded() {
	for (auto iter : model->rooms)
		if (!iter.second.loaded)
			loadRoom(iter.first);
}

model::room_t& Operation::loadRoom(rm_id_t id) {
	checkModel();
	load_result=false;
	//check if room already loaded in model:
	if (model->rooms.count(id))
		if (model->rooms[id].loaded)
			return model->rooms[id];
	load_result=true;
	//not found in model: load room from disk
	FileIO io;
	model->rooms[id]=io.loadRoom(rm_id_to_filename(id));
	return model->rooms[id];
}

rm_id_t Operation::makeRoom() {
	checkModel();
	rm_id_t id;
	id.gid=model->next_rm_gid++;
	id.rid=random();
	//first room must be {0,0}:
	if (id.gid==0)
		id.rid=0;
	model->rooms[id]=model::room_t();

	//update model-edited information:
	loadRoom(id).edited=true;
	model->edited=true;

	return id;
}

void Operation::editRoomTitle(rm_id_t id, std::string title) {
	//trim input:
	title.erase(title.find_last_not_of(" \n\r\t")+1);

	loadRoom(id).title=title;

	//update model-edited information:
	loadRoom(id).edited=true;
}

void Operation::editRoomBody(rm_id_t id, std::string body) {
	//trim input:
	body.erase(body.find_last_not_of(" \n\r\t")+1);

	loadRoom(id).body=body;

	//update model-edited information:
	loadRoom(id).edited=true;
}

void Operation::addOption(rm_id_t id,
		model::option_t option) {
	auto& options = loadRoom(id).options;
	//add option with gid = highest gid + 1.
	unsigned int max_opt_gid=0;
	//find highest gid
	for (auto iter : options)
		if (iter.first.gid>max_opt_gid)
			max_opt_gid=iter.first.gid;

	//trim input:
	option.option_text.erase(option.option_text.find_last_not_of(" \n\r\t")+1);

	//create id for option by
	options[{max_opt_gid+1,random()}]=option;

	//update model-edited information:
	loadRoom(id).edited=true;
}

void Operation::removeOption(rm_id_t rid, opt_id_t oid) {
	loadRoom(rid).options.erase(oid);

	//update model-edited information:
	loadRoom(rid).edited=true;
}

void Operation::editOption(rm_id_t rid, opt_id_t oid,
		model::option_t option) {
	loadRoom(rid).options[oid]=option;

	//update model-edited information:
	loadRoom(rid).edited=true;
}

void gyoa::ops::Operation::editRoomDeadEnd(rm_id_t id, bool dead_end) {
	loadRoom(id).dead_end=dead_end;

	//update model-edited information:
	loadRoom(id).edited=true;
}


std::string Operation::saveRoom(rm_id_t id) {
	checkModel();
	FileIO io;
	std::string file=rm_id_to_filename(id);
	io.writeRoomToFile(loadRoom(id),file);

	//reset model-edited information:
	loadRoom(id).edited=false;

	return file;
}

void Operation::saveWorld() {
	FileIO io;
	io.writeWorldToFile(*model,data_dir+"world.txt");

	//reset model-edited information:
	model->edited=false;
}

std::string Operation::saveAll(bool force) {
	std::string result="";
	//true if something is saved
	bool anything_saved=false;

	if (model->edited||force) {
		saveWorld();
		result+="saved world file.\n";
		anything_saved=true;
	}

	for (auto iter : model->rooms)
		if (iter.second.edited||force) {
			result+="saved scenario " + write_id(iter.first) + " to " + saveRoom(iter.first)+"\n";
			anything_saved=true;
		}

	if (anything_saved)
		result+="done.";
	else
		result+="nothing to be done.";

	return result;
}

void Operation::checkModel() {
	if (!model)
		throw NoModelException();
}

bool Operation::loadResult() {
	return load_result;
}

bool Operation::savePending() {
	if (model->edited)
		return true;
	for (auto iter : model->rooms)
		if (iter.second.edited)
			return true;
	return false;
}

void Operation::clearModel() {
	*model=model::world_t();
}

std::string Operation::rm_id_to_filename(rm_id_t id) {
	return data_dir+"rm_"+write_id(id)+".txt";
}

void gyoa::ops::Operation::reload() {
	assert(!savePending());
	assert(model!=nullptr);
	*model=loadWorld();
}

} /* namespace ops */
} /* namespace gyoa */
