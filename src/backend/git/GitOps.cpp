/*
 * GitOps.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#include "GitOps.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace gyoa {
namespace ops {

gyoa::ops::GitOps::GitOps() {
}

gyoa::ops::GitOps::~GitOps() {
}

void gyoa::ops::GitOps::setLocalRepoDirectory(std::string dir) {
	repo_dir=dir;
}

std::string GitOps::getLocalRepoDirectory() {
	return repo_dir;
}

void gyoa::ops::GitOps::setUpstream(std::string upstream) {
	std::string cmd =
	std::string(std::string("cd "+repo_dir).c_str())+"; "+
	std::string("git checkout -b master >/dev/null 2>/dev/null; ") +
	std::string("git remote set-url origin " + upstream)+" >/dev/null 2>/dev/null; "+
	std::string(std::string("git remote add origin " + upstream).c_str())+" &>/dev/null; "+
	std::string("cd - >/dev/null");
	system(cmd.c_str());
}

void gyoa::ops::GitOps::pull() {
	system(std::string("cd "+repo_dir+"; git pull --no-commit origin master; cd - >/dev/null").c_str());
}

void gyoa::ops::GitOps::addAll() {
	system(std::string("cd "+repo_dir + "; git add --all :/; cd - >/dev/null").c_str());
}

void gyoa::ops::GitOps::commit(std::string msg) {
	assert(msg.size()>0);
	assert(msg.find("\"")==std::string::npos);

	system(std::string("cd "+repo_dir + "; " + "git commit -m \"" + msg + "\" &> /dev/null; cd - >/dev/null").c_str());
}

void gyoa::ops::GitOps::init(bool silent) {
	system(std::string("git init "+repo_dir + ((silent)?" &> /dev/null":"")+";"
			"cd " + repo_dir + "; "
			" git config user.name \"gyoa-client\"; "
			  " git config user.email \"asdf@asdf.com\";"
			  "cd - > /dev/null"
			).c_str());
}

void gyoa::ops::GitOps::push() {
	system(std::string("cd "+repo_dir +"; git push -u origin master; cd - >/dev/null").c_str());
}

const std::string TMP_GIT_FILE = "/tmp/gyoa_git_stats.txt";

int GitOps::commitCount() {
	system(std::string("cd " + repo_dir + "> /dev/null; git rev-list HEAD --count > " + TMP_GIT_FILE + " 2>/dev/null; cd - > /dev/null").c_str());
	std::ifstream t(TMP_GIT_FILE);
	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string content =buffer.str();
	if (content.size()==0)
		return 0;
	return atoi(content.c_str());
}

std::string GitOps::getUpstream() {
	//return "" if this isn't a git directory -- if git root directory is above

	system(std::string("mkdir -p " + repo_dir + " >/dev/null").c_str());

	system(std::string("cd " + repo_dir + " >/dev/null; git rev-parse --show-toplevel > " + TMP_GIT_FILE).c_str());
	std::ifstream t0(TMP_GIT_FILE);
	std::stringstream buffer0;
	buffer0 << t0.rdbuf();
	std::string content = buffer0.str();

	if (!content.substr(0,5).compare("fatal"))//not in git repo
		return "";

	system(std::string("cd " + repo_dir + " >/dev/null; pwd > " + TMP_GIT_FILE).c_str());
	std::ifstream t1(TMP_GIT_FILE);
	std::stringstream buffer1;
	buffer1 << t1.rdbuf();
	std::string pwd = buffer1.str();

	if (pwd.compare(content))
		return "";

	system(std::string("cd " + repo_dir + " >/dev/null; git config --get remote.origin.url > " + TMP_GIT_FILE + "; cd - > /dev/null").c_str());
	std::ifstream t2(TMP_GIT_FILE);
	std::stringstream buffer2;
	buffer2 << t2.rdbuf();
	content = buffer2.str();
	return content;
}

} /* namespace ops */
} /* namespace gyoa */
