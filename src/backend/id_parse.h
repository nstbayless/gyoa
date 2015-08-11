#ifndef FILEIO_PARSE_H_
#define FILEIO_PARSE_H_

#include <iostream>
#include <string>

#include "model/Room.h"

//parses id of the form int:int
inline cyoa::model::id_type parse_id(std::string s){
	cyoa::model::id_type i;
	char punctuation;
	std::stringstream ss;
	ss<<s;
	ss>>i.gid;
	ss>>punctuation;
	ss>>i.rid;
	return i;
}

//converts id to string of the form int:int
inline std::string write_id(cyoa::model::id_type i) {
	return std::to_string(i.gid)+":" + std::to_string(i.rid);
}

#endif
