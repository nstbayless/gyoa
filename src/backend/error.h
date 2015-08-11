/*
 * error.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef BACKEND_MODEL_ERROR_H_
#define BACKEND_MODEL_ERROR_H_

#include <exception>
#include <iostream>

class NoModelException: public std::exception {
	virtual const char* what() const throw () {
		return "No Model Loaded";
	}
};

class FileNotFoundException: public std::exception {
public:
	FileNotFoundException(std::string file) {
		err = "File not Found: " + file;
	}
	virtual const char* what() const throw () {
		return err.c_str();
	}

	std::string err;
};

#endif /* BACKEND_MODEL_ERROR_H_ */
