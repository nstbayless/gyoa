/*
 * id_parse.cpp
 *
 *  Created on: Jan 7, 2016
 *      Author: n
 */

#include "gyoa/id_parse.h"

gyoa::model::id_type parse_id(const char* s){
	gyoa::model::id_type i;
	char punctuation;
	std::stringstream ss;
	ss << s;
	ss >> i.gid;

	if (!ss)
		return gyoa::model::id_type::err();

	ss >> punctuation;

	if (!ss)
		return gyoa::model::id_type::err();

	ss >> i.rid;

	if (!ss)
		return gyoa::model::id_type::err();

	return i;
}

//converts id to string of the form int:int
std::string write_id(gyoa::model::id_type i) {
	return std::to_string(i.gid)+":" + std::to_string(i.rid);
}
