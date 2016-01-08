/*
 * World.cpp
 *
 *  Created on: Dec 26, 2015
 *      Author: n
 */

#include "gyoa/World.h"

#include "git2/repository.h"
#include <utility>
#include <iostream>
#include "../fileio/FileIO.h"

#include "gyoa/id_parse.h"
namespace gyoa {
namespace model {

ActiveModel* makeModel(std::string directory) {
	ActiveModel* am = new ActiveModel();
	am->path=FileIO::getCanonicalPath(directory);
	if (am->path.substr(am->path.length()-1).compare("/"))
		am->path += "/";
	am->repo=nullptr;
	return am;
}

void freeModel(ActiveModel* am) {
	if (am->repo)
		git_repository_free(am->repo);
	delete(am);
}

bool directoryContainsModel(std::string dir) {
	return FileIO::fileExists(dir+"world.txt");
}

bool directoryInUse(std::string dir) {
	return (!!FileIO::getAllFiles(dir).size());
}

ActiveModel* loadModel(const char* dir) {
	ActiveModel* am = new ActiveModel();
	am->path=FileIO::getCanonicalPath(dir);
	if (am->path.substr(am->path.length()-1).compare("/"))
		am->path+="/";
	am->world = FileIO::loadWorld(am->path+"world.txt");
	am->repo=nullptr;
	return am;
}

int loadAllRooms(ActiveModel* am) {
	int c;
	for (auto iter : am->world.rooms)
		if (!iter.second.loaded) {
			getRoom(am,iter.first);
			c++;
		}
	return c;
}

bool roomExists(ActiveModel* am, rm_id_t id) {
	return am->world.rooms.count(id);
}

room_t& getRoom(ActiveModel* am, rm_id_t id) {
	assert (!id.is_null());
	if (am->world.rooms.count(id))
		if (am->world.rooms[id].loaded)
			return am->world.rooms[id];
	am->world.rooms[id]=FileIO::loadRoom(rm_id_to_filename(id,am->path));
	return am->world.rooms[id];
}

const char* getRoomTitle(ActiveModel* am, rm_id_t id) {
	return getRoom(am,id).title.c_str();
}

const char* getRoomBody(ActiveModel* am, rm_id_t id) {
	return getRoom(am,id).body.c_str();
}

const int getOptionCount(ActiveModel* am, rm_id_t id) {
	return getRoom(am,id).options.size();
}

const char* getOptionDescription(ActiveModel* am, rm_id_t id, int o) {
	auto& room = getRoom(am,id);
	const char* text = room.options[getOptionID(am,id,o)].option_text.c_str();
	return text;
}

rm_id_t getOptionDestination(ActiveModel* am, rm_id_t id, int o) {
	return getRoom(am,id).options[getOptionID(am,id,o)].dst;
}

opt_id_t getOptionID(ActiveModel* am,rm_id_t id, int N) {
	{
		auto& room = getRoom(am,id);
		int n=N;
		for (auto iter : room.options) {
			if (n == 0) {
				return iter.first;
			}
			n -= 1;
		}
		return opt_id_t::null();
	}
}

std::string rm_id_to_filename(rm_id_t id, std::string data_dir) {
	return data_dir+"rm_"+write_id(id)+".txt";
}
}
}
