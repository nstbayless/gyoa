/*
 * Room.h
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#ifndef ROOM_H_
#define ROOM_H_

#include <string>
#include <map>
namespace cyoa {

namespace model {

struct id_type {
	// integer (increments global counter on room creation)
	unsigned int gid;

	// integer (chosen at random from 0 to MAXINT on room creation)
	unsigned int rid;

	bool operator==(const id_type& other) const {
		return other.gid==gid&&other.rid==rid;
	}
	bool operator<(const id_type& other) const {
		if (gid<other.gid)
			return true;
		if (gid==other.gid)
			return rid<other.rid;
		return false;
	}
};

using rm_id_t=id_type;
using opt_id_t=id_type;

struct option_t {
	//!the id of the room this option leads to
	id_type dst;

	//!(fluff) text describing nature of option
	std::string option_text;
};

struct room_t {
	//! (fluff) header for room
	std::string title="untitled room";

	//! (fluff) body text for room
	std::string body="blah blah blah";

	//! list of outedges from this room. (Invariant: empty if dead_end is true)
	std::map<opt_id_t,option_t> options;

	//! this room marks an "end" for the game.
	bool dead_end=false;

	//! if false, room will be loaded when accessed
	bool loaded=true;
};

}

}

#endif /* ROOM_H_ */
