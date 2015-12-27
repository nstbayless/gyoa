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
	gitops::push_cred cred;
	while (true) {
		switch (input()) {
		case 'q':
		case 'e': mode=EDIT_ROOM; return;
		case 's':
			print(ops::saveAll(am)); continue;
		case 'd': //pull
			gitops::init(am);
			if (!tryFetch()){
				print("Error fetching from source. Aborting pull.");
				continue;
			}
			//if new source:
			if (!gitops::commonHistoryExists(am,context)) {
				print("Pulling from new source; overwrite local data? [y/n].");
				if (input()=='y') {
					print("Confirmed. Overwriting local files...");
					std::string path=am->path;
					model::freeModel(am);
					am=model::makeModel(path);
					gitops::obliterate(am);
					gitops::init(am);
					context::saveContext(context,am->path+"context.txt");
					print("fetching from " + context.upstream_url);
					if (!tryFetch()) {
						print("Error fetching from source. Aborting pull.");
						continue;
					}
					gitops::merge(am,gitops::FORCE_REMOTE,context);
					print("Download successful.\n\nPress [h] for help.");
					continue;
				} else {
					print("Aborted. Press [h] for help.");
					continue;
				}
			}

			//not a new source; routine pull
			if (ops::savePending(am)) {
				print("Saved changes pending. Save changes now [s], or [a]bort?");
				switch (input()) {
				case 's':
					break; //goes to ops::saveAll() below
				default:
					print("aborted.");
					continue;
				}
			}
			ops::saveAll(am);
			gitops::stageAndCommit(am,context,"pre-pull commit");
			pullAndMerge();
			print("\nType [h] for help.");
			continue;
		case 'u': //push
			if (context.upstream_url.length()<2) {
				print("no rem[o]te repository set.");
				continue;
			}
			if (ops::savePending(am)) {
				print("Saved changes pending. Save changes now [s], or [a]bort?");
				switch (input()) {
				case 's':
					break; //goes to ops::saveAll() below
				default:
					print("aborted.");
					continue;
				}
			}
			ops::saveAll(am);
			print("Enter a commit message, or leave blank for default");
			s=inputString();
			if (s.size()==0)
				s="Default commit message.";
			gitops::init(am);
			gitops::stageAndCommit(am,context,s);
			print("Merging before upload...");
			if (pullAndMerge(&cred)) {
				if (cred.credtype==gitops::push_cred::NONE)
					if (getCredentials(cred)) {
						print("aborting push");
						continue;
					}
				//push:
				print ("Connecting. Press enter to kill");
				gitops::push(am,context, cred, 4, [](void* varg)->bool {
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
						ui->print("push failure. " + gitops::gitError() + "\nPress [enter] to continue.");
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
			gitops::init(am);
			context.upstream_url=s;
			context::saveContext(context,am->path+"context.txt");
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

int ConsoleUI::getCredentials(gitops::push_cred& cred) {
	std::string username="";
	std::string pass="";
	if (context.upstream_url.find("@")==std::string::npos) {
		print("HTTPS connection.");
		//retrieve existing credentials:
		if (!context.git_authentication_prefs.do_not_store&&
				context.git_authentication_prefs.user_name.length()){
			print("Press [y] to retrieve username");
			if (input()=='y') {
				username=context.git_authentication_prefs.user_name;
			}
		}
		if (username.length() == 0) {
			print("Please enter username for " + context.upstream_url
							+ " (blank to abort)");
			username = inputString();
			if (username.length() == 0) {
				return -1;
			}
		}
		print("Please enter password");
		std::string password = getpass("> ");
		cred = gitops::make_push_cred_plaintext(username, password);
		//offer to store creds:
		if (!context.git_authentication_prefs.do_not_store
				&& !context.git_authentication_prefs.user_name.length()) {
			print("Would you like to store your username for future use? [y/n]");
			if (input() == 'y')
				context.git_authentication_prefs.user_name=username;
			else {
				print("stop asking? [y/n]");
				if (input()=='y')
					context.git_authentication_prefs.do_not_store=true;
			}
			context::saveContext(context,am->path+"context.txt");
		}
		return 0;
	} else {
		print("SSH connection.");
		std::string privkey_path;
		std::string pubkey_path;
		if (!context.git_authentication_prefs.do_not_store&&
				context.git_authentication_prefs.path_to_privkey.length()) {
			print("Press [y] to retrieve ssh keys");
			if (input() == 'y') {
				privkey_path=context.git_authentication_prefs.path_to_privkey;
				pubkey_path=context.git_authentication_prefs.path_to_pubkey;
				print("Please enter passphrase");
				pass = getpass("> ");
				cred = gitops::make_push_cred_ssh(privkey_path, pubkey_path, "",
						pass);
				return 0;
			}
		}
		print("Press [k] to authenticate with an SSH key,\n"
				"or [p] to perform plaintext authentication");
		switch(input()){
		case 'k':{
			print("Please enter the path to your private key (blank to abort)");
			privkey_path=inputString();
			if (privkey_path.length()==0)
				return -1;
			print("Please enter the path to your public key (blank to abort)");
			pubkey_path = inputString();
			if (pubkey_path.length() == 0)
				return -1;
			print("Please enter passphrase");
			pass = getpass("> ");
			cred = gitops::make_push_cred_ssh(privkey_path, pubkey_path, "",
					pass);
			//offer to store creds:
			if (!context.git_authentication_prefs.do_not_store
					&& !context.git_authentication_prefs.user_name.length()) {
				print(
						"Would you like to store "
						"the paths to your keys for future use? [y/n]\n"
						"(Your passphrase will not be stored)");
				if (input() == 'y') {
					context.git_authentication_prefs.path_to_privkey = privkey_path;
					context.git_authentication_prefs.path_to_pubkey = pubkey_path;
				}
				else {
					print("stop asking? [y/n]");
					if (input() == 'y')
						context.git_authentication_prefs.do_not_store = true;
				}
				context::saveContext(context,am->path+"context.txt");
			}
			return 0;
		}
		case 'p':{
			print("Please enter password");
			pass = getpass("> ");
			cred = gitops::make_push_cred_plaintext("", pass);
			return 0;
		}
		default:
			return -1;
		}
	}
}

bool ConsoleUI::tryFetch(gitops::push_cred* cred_) {
	assert(context.upstream_url.length()>1);
	if (!gitops::fetch(am,context,4)) {
		print("Authentication required to download world.");
		gitops::push_cred cred;
		if (getCredentials(cred))
			return false;
		if (cred_)
			*cred_=cred;
		return gitops::fetch(am,context,cred,4);
	}
	return true;
}

const std::string CONFLICT_START = "CONFLICT->>";
const std::string CONFLICT_SEPARATOR = "|<-remote : local->|";
const std::string CONFLICT_END = "<<-CONFLICT";

bool ConsoleUI::pullAndMerge(gitops::push_cred* cred) {
	assert(!ops::savePending(am));
	if (!tryFetch(cred)) {
		print("Error fetching from source. Aborting pull.");
		return false;
	}
	gitops::merge_style style = gitops::MANUAL;
	if (gitops::merge(am,gitops::TRIAL_RUN,context).conflict_occurred()) {
		print(	"Conflicts exist between your version and the remote version. How should this be handled?\n"
						"[a]   abort, nothing is changed.\n"
						"[m]   handle conflicts manually (this might take you a while).\n"
						"[l]   use local version in all cases.\n"
						"[w]   use remote version in all cases. (Recommended.)\n");
		switch (input()) {
		case 'm':
			print("Manual merge... expect prompts.");
			style = gitops::MANUAL;
			break;
		case 'l':
			print("Using local versions to resolve conflicts. Very assertive.");
			style = gitops::USE_LOCAL;
			break;
		case 'w':
			print("Using remote versions to resolve conflicts. How thoughtful.");
			style = gitops::USE_REMOTE;
			break;
		default:
			print("aborted.");
			return false;
		}
	}

	auto result = gitops::merge(am,style,context);

	print(std::to_string(result.resolved) + " conflicts resolved.");
	print(std::to_string(result.changes) + " changes made in all.");
	if (style==gitops::MANUAL&&result.conflicts.size())
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
			case gitops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=conflict.local;
				break;
			case gitops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = parse_id(conflict.local);
				break;
			case gitops::MergeConflict::BOOL:
				*((bool*) conflict.data_ptr) = conflict.local[0]-'0';
				break;
			}
			print("using local revision.");
			continue;
		case 'w':
			switch (conflict.data_type) {
			case gitops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=conflict.remote;
				break;
			case gitops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = parse_id(conflict.remote);
				break;
			case gitops::MergeConflict::BOOL:
				*((bool*) conflict.data_ptr) = conflict.remote[0]-'0';
				break;
			}
			print("using remote revision.");
			continue;
		case 'c':
			switch (conflict.data_type) {
			case gitops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=conflict.common;
				break;
			case gitops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = parse_id(conflict.common);
				break;
			case gitops::MergeConflict::BOOL:
				*((bool*) conflict.data_ptr) = conflict.common[0]-'0';
				break;
			}
			print("using latest common ancestor.");
			continue;
		case 'm':
			print("merging manually.");
			switch (conflict.data_type) {
			case gitops::MergeConflict::STRING:
				*((std::string*)conflict.data_ptr)=edit_text(CONFLICT_START + conflict.remote + CONFLICT_SEPARATOR + conflict.local + CONFLICT_END);
				break;
			case gitops::MergeConflict::ID:
				*((model::id_type*) conflict.data_ptr) = inputRoom();
				break;
			case gitops::MergeConflict::BOOL:
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
	ops::saveAll(am,true);
	return true;
}

}

}
