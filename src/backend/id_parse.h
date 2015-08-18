#ifndef FILEIO_PARSE_H_
#define FILEIO_PARSE_H_

#include <iostream>
#include <string>

#include "model/Room.h"
#include <sstream>

//parses id of the form int:int
inline gyoa::model::id_type parse_id(std::string s){
	gyoa::model::id_type i;
	char punctuation;
	std::stringstream ss;
	ss << s;
	ss >> i.gid;

	// Invalid rooms are stored with int -1.
	// We have leftover signed 2^31-1 == 0x7FFF FFFF in legacy files.
	// They indicate the null room. !ss means overflow. Right now, no number
	// in existing files produces overflow -- we don't have 2^32 - 1 anywhere.
	if (!ss || i.gid == 0x7FFFFFFF)
		return gyoa::model::id_type::null();

	ss >> punctuation;
	ss >> i.rid;

	return ss ? i : gyoa::model::id_type::null();
}

//converts id to string of the form int:int
inline std::string write_id(gyoa::model::id_type i) {
	return std::to_string(i.gid)+":" + std::to_string(i.rid);
}

#endif
