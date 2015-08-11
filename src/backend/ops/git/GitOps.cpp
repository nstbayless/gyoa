/*
 * GitOps.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#include "GitOps.h"

#include <cstdlib>
#include <string>
#include <cassert>

namespace cyoa {
namespace ops {

GitOps::GitOps() {
}

GitOps::~GitOps() {
}

void GitOps::setLocalRepoDirectory(std::string dir) {
	repo_dir=dir;
}

void GitOps::setUpstream(std::string upstream) {
	std::string cmd =
	std::string(std::string("cd "+repo_dir).c_str())+"; "+
	std::string("git checkout -b master >/dev/null 2>/dev/null; ") +
	std::string("git remote set-url origin " + upstream)+" >/dev/null 2>/dev/null; "+
	std::string(std::string("git remote add origin " + upstream).c_str())+" &>/dev/null 2>/dev/null; "+
	std::string("cd - >/dev/null");
	system(cmd.c_str());
}

void GitOps::pull() {
	system(std::string("cd "+repo_dir+"; git pull origin master; cd - >/dev/null").c_str());
}

void GitOps::addAll() {
	system(std::string("cd "+repo_dir + "; git add --all :/; cd - >/dev/null").c_str());
}

void GitOps::commit(std::string msg) {
	assert(msg.size()>0);
	assert(msg.find("\"")==std::string::npos);

	system(std::string("cd "+repo_dir + "; " + "git commit -m \"" + msg + "\"; cd - >/dev/null").c_str());
}

void GitOps::init(bool silent) {
	system(std::string("git init "+repo_dir + ((silent)?" > /dev/null":"")).c_str());
}

void GitOps::push() {
	system(std::string("cd "+repo_dir +"; git push origin master; cd - >/dev/null").c_str());
}

} /* namespace ops */
} /* namespace cyoa */
