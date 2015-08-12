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
namespace gyoa {

namespace model {

//! stores an id for a room_t in a world_t or an option_t in a room_t
//! comprises an integer that increments for each id, and and a randomly-
//! generated integer to preclude collisions when committing to git.
//!
//! a gid of -1 is reserved for default return values/null id
struct id_type {
	// integer (increments global counter on room creation)
	unsigned int gid;

	// integer (chosen at random from 0 to MAXINT on room creation)
	unsigned int rid;

	bool operator==(const id_type& other) const {
		return other.gid==gid&&other.rid==rid;
	}

	bool operator!=(const id_type& other) const {
		return !(*this==other);
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
	std::string title="untitled scenario";

	//! (fluff) body text for room
	std::string body=
			"This is the body text, but nobody has written anything yet.\n"
			"If you'd like to edit it, press [t] from edit mode. "
			"If you're not in edit mode, press [e] to enter edit mode.";

	//! list of outedges from this room. (Invariant: empty if dead_end is true)
	std::map<opt_id_t,option_t> options;

	//! this room marks an "end" for the game.
	bool dead_end=false;

	//! if false, room will be loaded when accessed
	bool loaded=true;

	//! true if room has been edited since last save
	bool edited=false;
};

}

}

#endif /* ROOM_H_ */
