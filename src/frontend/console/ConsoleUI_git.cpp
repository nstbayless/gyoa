/*
 * ConsoleUI_git.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: n
 */

#include <string>
#include <cassert>

#include "ConsoleUI.h"

namespace gyoa { namespace ui {

void ConsoleUI::editGit() {
	assert(mode==EDIT_GIT);
	print_help();
	std::string s;
	while (true) {
		switch (input()) {
		case 'q':
		case 'e': mode=EDIT_ROOM; return;
		case 's':
			print(ops.saveAll()); continue;
		case 'm': //pull
			ops.gitOps.init();
			ops.saveAll();
			ops.gitOps.addAll();
			ops.gitOps.commit("pre-pull commit");
			ops.gitOps.pull();
			ops.reload();
			print("\nType [h] for help.");
			continue;
		case 'p': //push
			print("Enter a commit message, or leave blank for default");
			s=inputString();
			if (s.size()==0)
				s="Default commit message.";
			ops.saveAll();
			ops.gitOps.init();
			ops.gitOps.addAll();
			ops.gitOps.commit(s);
			ops.gitOps.pull();
			ops.gitOps.push();
			ops.reload();
			print("\nType [h] for help.");
			continue;
		case 'u': //set upstream
			print("Enter upstream repository url, e.g. https://github.com/account/gyoa");
			s=inputString();
			print("Upstream URL: " + s);
			ops.gitOps.init();
			ops.gitOps.setUpstream(s);
			print("Upstream repository set to "+ s);
			print("\nType [h] for help.");
			continue;
		case 'h':
			print_help();
			continue;
		default:
			print("No. Press [h] for help.");
			continue;
		}
	}
}
}}
