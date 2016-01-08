#ifndef FILEIO_PARSE_H_
#define FILEIO_PARSE_H_

#include <iostream>
#include <string>

#include "gyoa/Room.h"
#include <sstream>

extern "C" {

//TODO: put into .cpp
//parses id of the form int:int
gyoa::model::id_type parse_id(const char* s);

//converts id to string of the form int:int
std::string write_id(gyoa::model::id_type i);

}

#endif
