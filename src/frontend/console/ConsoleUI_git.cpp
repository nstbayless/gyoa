/*
 * ConsoleUI_git.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: n
 */

#include <cassert>
#include <string>

#include "../../backend/git/GitOps.h"
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
			ops.gitOps->init();
			if (!ops.gitOps->commonHistoryExists()) {
				print("Pulling from new source; overwrite local data? [y/n].");
				if (input()=='y') {
					print("Confirmed. Overwriting local files...");
					ops.gitOps->pull();
					ops.clearModel();
					ops.gitOps->merge(ops::FORCE_REMOTE);
					print("Merge successful.\n\nPress [h] for help.");
					continue;
				} else {
					print("Aborted. Press [h] for help.");
					continue;
				}
			}
			ops.saveAll();
			ops.gitOps->addAll();
			ops.gitOps->commit("pre-pull commit");
			ops.gitOps->pull();
			ops.gitOps->merge(ops::MANUAL);
			print("\nType [h] for help.");
			continue;
		case 'p': //push
			print("Enter a commit message, or leave blank for default");
			s=inputString();
			if (s.size()==0)
				s="Default commit message.";
			ops.saveAll();
			ops.gitOps->init();
			ops.gitOps->addAll();
			ops.gitOps->commit(s);
			ops.gitOps->pull();
			ops.gitOps->merge(ops::USE_REMOTE);
			ops.gitOps->push();
			print("\nType [h] for help.");
			continue;
		case 'u': //set upstream
			print("Enter upstream repository url, e.g. https://github.com/account/gyoa\nleave blank to cancel.");
			s=inputString();
			if (!s.size()) {
				print("Cancelled.");
				continue;
			}
			print("Upstream URL: " + s);
			ops.gitOps->init();
			ops.gitOps->setUpstream(s);
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
