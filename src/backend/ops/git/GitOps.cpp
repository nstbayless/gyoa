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
	system(std::string("cd "+repo_dir).c_str());
	system("git remote rm origin");
	system(std::string("git remote add origin " + upstream).c_str());
	system("git branch --set-upstream origin/master master");
	system("cd -");
}

void GitOps::pull() {
	system(std::string("cd "+repo_dir).c_str());
	system("git pull origin master");
	system("cd -");
}

void GitOps::addAll() {
	system(std::string("cd "+repo_dir).c_str());
	system("git add --all :/");
	system("cd -");
}

void GitOps::commit(std::string msg) {
	assert(msg.size()>0);
	assert(msg.find("\"")==std::string::npos);

	system(std::string("cd "+repo_dir).c_str());
	system(std::string("git commit -m \"" + msg + "\"").c_str());
	system("cd -");
}

void GitOps::init() {
	system(std::string("git init "+repo_dir).c_str());
}

void GitOps::push() {
	system(std::string("cd "+repo_dir).c_str());
	system(std::string("git push origin master ").c_str());
	system(std::string("cd -").c_str());
}

} /* namespace ops */
} /* namespace cyoa */
