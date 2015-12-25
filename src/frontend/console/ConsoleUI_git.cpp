/*
 * ConsoleUI_git.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: n
 */

#include <unistd.h>
#include <cassert>
#include <string>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../backend/git/GitOps.h"
#include "../../backend/id_parse.h"
#include "../../backend/model/Room.h"
#include "ConsoleUI.h"

namespace gyoa { namespace ui {

void ConsoleUI::editGit() {
	assert(mode==EDIT_GIT);
	print_help();
	std::string s;
	ops::push_cred cred;
	while (true) {
		switch (input()) {
		case 'q':
		case 'e': mode=EDIT_ROOM; return;
		case 's':
			print(ops.saveAll()); continue;
		case 'd': //pull
			ops.gitOps.init();
			ops.gitOps.fetch(context);
			//if new source:
			if (!ops.gitOps.commonHistoryExists(context)) {
				print("Pulling from new source; overwrite local data? [y/n].");
				if (input()=='y') {
					print("Confirmed. Overwriting local files...");
					ops.clearModel();
					ops.gitOps.clear();
					ops.saveContext(context);
					ops.gitOps.fetch(context);
					ops.gitOps.merge(ops::FORCE_REMOTE,ops,context);
					print("Download successful.\n\nPress [h] for help.");
					continue;
				} else {
					print("Aborted. Press [h] for help.");
					continue;
				}
			}

			//not a new source, routine pull:
			if (ops.savePending()) {
				print("Saved changes pending. Save changes now [s], or [a]bort?");
				switch (input()) {
				case 's':
					break; //goes to ops.saveAll() below
				default:
					print("aborted.");
					continue;
				}
			}
			ops.saveAll();
			ops.gitOps.commit(context,"pre-pull commit");
			pullAndMerge();
			print("\nType [h] for help.");
			continue;
		case 'u': //push
			if (ops.savePending()) {
				print("Saved changes pending. Save changes now [s], or [a]bort?");
				switch (input()) {
				case 's':
					break; //goes to ops.saveAll() below
				default:
					print("aborted.");
					continue;
				}
			}
			ops.saveAll();
			print("Enter a commit message, or leave blank for default");
			s=inputString();
			if (s.size()==0)
				s="Default commit message.";
			ops.gitOps.init();
			ops.gitOps.commit(context,s);
			print("Merging before upload...");
			if (pullAndMerge()) {
				//get credentials before push:
				if (!context.upstream_url.substr(0, 4).compare("http")) {
					//using https
					print("Please enter username for " + context.upstream_url);
					std::string username = inputString();
					print("Please enter password");
					std::string password = getpass("> ");
					cred = ops::make_push_cred_plaintext(username, password);
				} else {
					//todo: ssh
					assert(false);
				}
				//push:
				print ("Enter to kill");
				ops.gitOps.push(context, cred, [](void* varg)->bool {
					ConsoleUI* ui = (ConsoleUI*)varg;
					while (true) {
						ui->input();
						return true;
					}
				}, [](bool success,void* varg) {
					ConsoleUI* ui = (ConsoleUI*)varg;
					if (success)
						ui->print("push successful! Press [enter] to continue.");
					else
						ui->print("push failure");
				},this);
			}
			else
				print("Merge failed, not uploading local changes.");
			print("\nType [h] for help.");
			continue;
		case 'o': //set upstream
			print("Enter upstream repository url, e.g. https://github.com/account/gyoa\nleave blank to cancel.");
			s=inputString();
			if (!s.size()) {
				print("Cancelled.");
				continue;
			}
			ops.gitOps.init();
			context.upstream_url=s;
			ops.saveContext(context);
			ops.gitOps.setOrigin(context);
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

const std::string CONFLICT_START = "CONFLICT->>";
const std::string CONFLICT_SEPARATOR = "|<-remote : local->|";
const std::string CONFLICT_END = "<<-CONFLICT";

bool ConsoleUI::pullAndMerge() {
	assert(!ops.savePending());
	ops.gitOps.fetch(context);
	ops::merge_style style = ops::MANUAL;
	if (ops.gitOps.merge(ops::TRIAL_RUN,ops,context).conflict_occurred()) {
		print(	"Conflicts exist between your version and the remote version. How should this be handled?\n"
						"[a]   abort, nothing is changed.\n"
						"[m]   handle conflicts manually (this might take you a while).\n"
						"[l]   use local version in all cases.\n"
						"[w]   use remote version in all cases. (Recommended.)\n");
		switch (input()) {
		case 'm':
			print("Manual merge... expect prompts.");
			style = ops::MANUAL;
			break;
		case 'l':
			print("Using local versions to resolve conflicts. Very assertive.");
			style = ops::USE_LOCAL;
			break;
		case 'w':
			print("Using remote versions to resolve conflicts. How thoughtful.");
			style = ops::USE_REMOTE;
			break;
		default:
			print("aborted.");
			return false;
		}
	}

	auto result = ops.gitOps.merge(style,ops,context);

	print(std::to_string(result.resolved) + " conflicts resolved.");
	print(std::to_string(result.changes) + " changes made in all.");
	if (style==ops::MANUAL&&result.conflicts.size())
		print(std::to_string(result.conflicts.size())+" conflicts unresolved.");
	for (auto conflict : result.conflicts) {
		user_failed:
		print("\nConflict: " + conflict.description);
		print("\n"
				"[l]   use local revision\n"
				"[w]   use remote revision\n"
				"[c]   revert to previous version\n"
				"[m]   edit manually\n");
		switch(input()) {
		case 'l':
			switch (conflict.data_type) {
			case ops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=conflict.local;
				break;
			case ops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = parse_id(conflict.local);
				break;
			case ops::MergeConflict::BOOL:
				*((bool*) conflict.data_ptr) = conflict.local[0]-'0';
				break;
			}
			print("using local revision.");
			continue;
		case 'w':
			switch (conflict.data_type) {
			case ops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=conflict.remote;
				break;
			case ops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = parse_id(conflict.remote);
				break;
			case ops::MergeConflict::BOOL:
				*((bool*) conflict.data_ptr) = conflict.remote[0]-'0';
				break;
			}
			print("using remote revision.");
			continue;
		case 'c':
			switch (conflict.data_type) {
			case ops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=conflict.common;
				break;
			case ops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = parse_id(conflict.common);
				break;
			case ops::MergeConflict::BOOL:
				*((bool*) conflict.data_ptr) = conflict.common[0]-'0';
				break;
			}
			print("using latest common ancestor.");
			continue;
		case 'm':
			print("merging manually.");
			switch (conflict.data_type) {
			case ops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=edit_text(CONFLICT_START + conflict.remote + CONFLICT_SEPARATOR + conflict.local + CONFLICT_END);
				break;
			case ops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = inputRoom();
				break;
			case ops::MergeConflict::BOOL:
				user_failed_bool:
				print("Should room be a dead end? [y/n]");
				switch (input()) {
				case 'y':
					*((bool*) conflict.data_ptr) = true;
					print("Dead end enabled.");
					break;
				case 'n':
					print("Dead end disabled.");
					break;
				default:
					goto user_failed_bool;
				}
				break;
			}
			continue;
		default:
			print("Not a valid input. ");
			goto user_failed;
		}
	}
	print("Merge complete.");
	ops.saveAll(true);
	return true;
}

}

}
