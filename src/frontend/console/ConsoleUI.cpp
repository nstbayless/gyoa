/*
 * ConsoleUI.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#include "ConsoleUI.h"

#include <cassert>
#include <cstdlib>
#include <map>
#include <utility>

#include "../../backend/git/GitOps.h"
#include "../../backend/id_parse.h"
#include "../../meta.h"


namespace gyoa {
namespace ui {

ConsoleUI::ConsoleUI() {
}

ConsoleUI::~ConsoleUI() {
}

void ConsoleUI::start() {
	clear();
	print("Welcome to " + meta::NAME_FULL+"!");
	print("Built: "+meta::BUILD_DATE +" at " + meta::BUILD_TIME);
	system("mkdir data 2> /dev/null");
	gitops::gitInit();
	print("\nloading world...");
	std::string def_path="data/";

	//! a download is recommended
	bool git_pull_reqd=true;
	//! user chose to create new world
	bool newly_created=false;
	char choice;

	if (model::directoryContainsModel(def_path)) {
		print("World found! Loading world...");
		am=model::loadModel(def_path);
		print("World title \"" + am->world.title+"\"");
		context=context::loadContext(def_path+"context.txt");
	} else {
		print("No world found. Creating new world instead...");
		if (model::directoryInUse(def_path)){
			print("Directory \"" + def_path + "\"already in use. Aborting.");
			goto exit;
		}
		am=model::makeModel(def_path);
	}

	//user selects mode:
	pick_mode:
	print("\nWould you like to [e]dit, [p]lay, or [q]uit?");

	//remote repository options:
	if (git_pull_reqd) {
		if (context.upstream_url.size()==0){
			if (newly_created)
				print("You may also [d]ownload an adventure from the internet.");
		}
		else
			print("You may also [d]ownload the latest changes to the adventure from the internet. (recommended.)");
	}
	choice = input();
	if (choice=='e') {
		mode=EDIT_ROOM;
	} else if (choice=='p') {
		mode=PLAY;
		clear();
		FixContextIfRoomIsNull();
		print_room();
	} else if (choice=='q') {
		mode=QUIT;
		goto exit;
	} else if (choice=='d'&&git_pull_reqd) {
		ops::saveAll(am);
		gitops::init(am);
		gitops::stageAndCommit(am,context,"pre-pull commit");
		pullAndMerge();
		git_pull_reqd=false;
		mode=META;
		goto pick_mode;
	} else if (choice=='d'&&git_pull_reqd&&context.upstream_url.size()==0&&newly_created) {
		gitops::init(am);
		print("Please enter a URL for the upstream repository, e.g. https://github.com/account/repo\nLeave blank to cancel.");
		std::string input = inputString();
		if (input.size()==0)
			goto pick_mode;
		context.upstream_url=input;
		context::saveContext(context,am->path+"context.txt");
		if (!tryFetch()) {
			print("Fatal error fetching from source.");
			goto exit;
		}
		gitops::merge(am,gitops::FORCE_REMOTE,context);
		print("Download successful.\n\nPress [h] for help.");
		mode=META;
		git_pull_reqd=false;
		goto pick_mode;
	} else {
		mode=META;
		goto pick_mode;
	}

	//user has fun:
	while (true) {
		if (mode==EDIT_ROOM)
			editCurrentRoom();
		if (mode==PLAY)
			playCurrentRoom();
		if (mode==QUIT){
			if (ops::savePending(am)) {
				print("Note: unsaved changes! Press [s] to save, [x] to confirm quit.");
				while (true) {
					switch (input()) {
					case 's':
						print(ops::saveAll(am));
					case 'x':
						goto exit;
					default:
						print("[s]ave or e[x]it");
						break;
					}
				}
			} else break;
		}
	}
	exit:
	print("Exiting game...");
	model::freeModel(am);
	gitops::gitShutdown();
	clear();
}

void ConsoleUI::editCurrentRoom() {
	assert(mode==EDIT_ROOM);
	if (FixContextIfRoomIsNull()) {
		print("World is empty. Creating initial room...");
		model::rm_id_t init = context.current_room = ops::makeRoom(am);
		ops::editStartRoom(am,init);
	}
	print_help();
	std::string s;
	while (true){
		assert(mode==EDIT_ROOM);
		auto id = context.current_room;
		auto& rm = model::getRoom(am,context.current_room);
		print ("Enter command. Press [h] for help.");
		switch (input()) {
		case 'q': mode=QUIT;									return;
		case 'p': mode=PLAY; print_room();						return;
		case 'b': context.current_room=am->world.first_room; print_help();
			continue;
		case 'j': id=inputRoom();
			if (!id.is_null()) {
				//if input, jump to room
				context.current_room=id;
				print_help();
			}
			id = context.current_room;
			break;
		case 'i':
			model::loadAllRooms(am);
			for (auto iter : am->world.rooms) {
				print("scenario id "+write_id(iter.first) +" (\""+iter.second.title+"\")");
			}
			break;
		// print room description:
		case 'r':
			print_room();
			break;
		case 'y':
			print("Old title: " + rm.title
							+ "\nEnter a new title (blank to cancel):");
			s=inputString();
			if (s.length())
				ops::editRoomTitle(am,id,s);
			print("\n ## " + rm.title + " ##\n\n");
			if (s.length()) {
				print("Title edited. (Don't forget to [s]ave.) Press [r] to review.\n");
			}
			break;
		case 't':
			s=edit_text(rm.body);
			if (s.length()) {
				ops::editRoomBody(am,id,s);
				//todo: compare to see if edited at all.
				print("body text edited. (Don't forget to [s]ave.). Press [r] to review.\n");
			}
			break;
		case 'd':
			if (rm.options.size()) {
				print("There are options available.\n"
						"Please remove all options with [o] then [x] before marking this as a dead end.");
				break;
			}
			ops::editRoomDeadEnd(am,id,!rm.dead_end);
			print("Dead End: " + std::string(((rm.dead_end) ? "Enabled" : "Disabled")) + ". (Don't forget to [s]ave.)\n");
			break;
		case 's':
			print(ops::saveAll(am));
			break;
		case 'h':
			print_help();
			break;
		case 'o':
			mode=EDIT_OPTIONS;
			print_help();
			editOptions();
			mode=EDIT_ROOM;
			clear();
			print_help();
			break;
		case 'g':
			mode=EDIT_GIT;
			print_help();
			editGit();
			mode=EDIT_ROOM;
			clear();
			print_help();
		default:
			continue;
		}
	}
}

void ConsoleUI::editOptions() {
	//this is a messy function
	assert(mode == EDIT_OPTIONS);

	//declaring these local variables in advance
	//because variables cannot be declared within a switch
	int it;
	std::string s;
	model::option_t opt;
	model::option_t opt_input;
	model::rm_id_t input_id;

	//repeatedly get input from user and execute, break if 'q' or 'e' input
	while (true) {
		assert(mode == EDIT_OPTIONS);
		auto id = context.current_room;
		auto& rm = model::getRoom(am,context.current_room);
		print ("Enter command. Press [h] for help, [e] to return to main edit menu.");
		switch (char c = input()) {
			case 'q':
			case 'e': return;
			case 'r': print(""); print_room(); break;
			case 's': print(ops::saveAll(am));
					break;
			case 'h': print_help(); break;

			//add new option to room:
			case 'a': print("Enter text for new option (leave blank to cancel)");
					s = inputString();
					if (s.length()) {
						//define option user is adding.
						//gid.is_null() means the option goes nowhere by default.
						opt.dst = model::opt_id_t::null();
						opt.option_text=s;

						//return here if user does not enter a valid option [c|l|e]
						user_failed:
						print("[c]reate scenario for option, [l]ink to existing scenario, or continue [e]diting?");
						switch(input()) {
							case 'd': //d is allowed as an alternative for c as when editing options below.
							case 'c':
								context.current_room=opt.dst=ops::makeRoom(am);
								ops::editRoomTitle(am,opt.dst,opt.option_text);
								mode=EDIT_ROOM;
								break;
							case 'l':
								//update opt.dst iff there is an input entry
								input_id = inputRoom();
								if (input_id.is_null()) {
									print("\nCancelled operation.");
									break;
								} else
									opt.dst=input_id;
							case 'e': print_help(); break;
							default: goto user_failed;
						}

						//option defined to user's tastes; add to room:
						ops::addOption(am,id,opt);
						print_help();
						if (mode==EDIT_ROOM)
							return;
					} else
						print("\nCancelled.");
					break;

			//remove an existing option:
			case 'x': print("remove which option? Enter number (0 to cancel)");
					it = input()-'0';
					if (it>0 && it<=9) {
						input_id=model::getOption(model::getRoom(am,id),it);

						if (input_id.is_null()) {
							//invalid option input:
							print("\nNo such option exists to remove.");
							break;
						}
						//valid option input:
						ops::removeOption(am,id,input_id);
						print("\nOption removed.");
						break;
					}
					break;

			default:
					it = c-'0';
					//edit an existing option:
					if (it>0 && it<=9) {
						input_id=model::getOption(rm,it);
						if (input_id.is_null()) {
							//invalid option input:
							print("\nNo such option exists to edit, but you can [a]dd it if you prefer.");
							break;
						}
						//valid option input:
						opt_input=rm.options[input_id];
						print("Current text: "+opt_input.option_text);
						if (opt_input.dst.is_null())
							print ("No Destination");
						else
							print("Destination: "+write_id(opt_input.dst));
						print("Edit [t]ext, [l]ink to existing destination, or create new [d]estination?");
						switch(input()) {
							case 't':
								opt.dst=opt_input.dst;
								opt.option_text=edit_text(opt_input.option_text);
								print("Text edited. Don't forget to [s]ave. Press [r] to review.\n");
								ops::editOption(am,id, input_id, opt);
								continue;
							case 'd':
								context.current_room=opt.dst=ops::makeRoom(am);
								ops::editOption(am,id, input_id, opt);
								ops::editRoomTitle(am,opt.dst,opt.option_text);
								mode=EDIT_ROOM;
								print_help();
								return;
							case 'l':
								ops::editOption(am,id, input_id, {inputRoom(), opt_input.option_text});
								print("\noption edited. Still editing room " + write_id(id) + "(\"" + rm.title + "\").\n");
								continue;
							default:
								print("\nCancelled.");
								continue;
						}
						print("\nInvalid option number.");
						break;
					}
					break;
		}
	}
}

void ConsoleUI::playCurrentRoom() {
	user_failed:
	assert(mode==PLAY);
	auto& rm = model::getRoom(am,context.current_room);
	if (FixContextIfRoomIsNull())
		return;
	switch(char c = input()){
	case 'q': mode=QUIT; 											break;
	case 'e': mode=EDIT_ROOM; 										break;
	case 'b': context.current_room=am->world.first_room; print_room(); 	break;
	case 'r': print(""); print_room();								break;
	case 'h': print_help();											break;
		default:
		if (c>='1'&&c<='9') {
			int choice = c-'0';
			//select choice-th option:
			for (auto iter : rm.options) {
				choice--;
				if (choice==0) {
					if (iter.second.dst.is_null()) { //no destination for given option
						print("This option's scenario has not been written.\n"
								"You may [c]reate a scenario for the option, "
								"[r]eturn to pick another option, "
								"[e]dit the current scenario, "
								"or [b]egin again from the start.");
						model::option_t opt_edit = iter.second;
						switch (input()) {
							case 'd': //alternative for 'c'
							case 'c':
									opt_edit.dst=ops::makeRoom(am);
									ops::editOption(am,context.current_room,iter.first,opt_edit);
									ops::editRoomTitle(am,opt_edit.dst,opt_edit.option_text);
									context.current_room=opt_edit.dst;
									mode=EDIT_ROOM;
										break;
							case 'r':
									print_room();
										return;
							case 'e':
									mode=EDIT_ROOM; return;
							case 'b':
									context.current_room=am->world.first_room; print_room(); return;
							default:
									print_room();
						}
					}
					else {
						context.current_room=iter.second.dst;
						context::saveContext(context,am->path+"context.txt");
						print_room();
					}
					return;
				}
			}
		}
		//user failed!
		print("Nope. Press [h] for help, or [r]eread the descriptive text above.\n");
		goto user_failed;
	}
	return;
}

bool ConsoleUI::FixContextIfRoomIsNull() {
	if (context.current_room.is_null()) {
		context.current_room = am->world.first_room;
		if (context.current_room.is_null()) {
			clear();
			print("World is empty. Switching to edit mode.");
			mode = EDIT_ROOM;
			return true;
		}
	}
	return false;
}

} /* namespace ui */
} /* namespace gyoa */
