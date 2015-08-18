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

class GitNotRepo: public std::exception {
public:
	GitNotRepo(std::string path) {
		err = "Not a git repository or can't open: " + path;
	}
	virtual const char* what() const throw () {
		return err.c_str();
	}

	std::string err;
};

class GitInitFail: public std::exception {
public:
	GitInitFail(std::string path) {
		err = "Could not initialize repository: " + path;
	}
	virtual const char* what() const throw () {
		return err.c_str();
	}

	std::string err;
};

class GitCloneFail: public std::exception {
public:
	GitCloneFail(std::string path,std::string url) {
		err = "Could not clone repository: " + path + " from " + url;
	}
	virtual const char* what() const throw () {
		return err.c_str();
	}

	std::string err;
};

#endif /* BACKEND_MODEL_ERROR_H_ */
