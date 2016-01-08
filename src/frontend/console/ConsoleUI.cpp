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

#include "gyoa/GitOps.h"
#include "gyoa/id_parse.h"
#include "gyoa/meta.h"


namespace gyoa {
namespace ui {

ConsoleUI::ConsoleUI() {
}

ConsoleUI::~ConsoleUI() {
}

void ConsoleUI::start(std::string path) {
	clear();
	print("Welcome to " + meta::NAME_FULL+"!");
	print("Built: "+meta::BUILD_DATE +" at " + meta::BUILD_TIME);
	system(std::string("mkdir " + path+ " 2> /dev/null").c_str());
	gitops::libgitInit();
	print("\nloading world...");

	//! a download is recommended
	bool git_pull_reqd=true;
	//! user chose to create new world
	bool newly_created=false;
	char choice;

	if (model::directoryContainsModel(path)) {
		print("World found! Loading world...");
		am=model::loadModel(path.c_str());
		print("World title \"" + am->world.title+"\"");
		context=context::loadContext(path+"context.txt");
	} else {
		print("No world found. Creating new world instead...");
		if (model::directoryInUse(path)){
			print("Directory \"" + path + "\" already in use. Aborting.");
			//pause
			input(false);
			goto EXIT;
		}
		am=model::makeModel(path);
		newly_created=true;
	}

PICK_MODE:
	//user selects mode:
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

	//user selects mode
	if (choice=='e') {
		//edit mode
		mode=EDIT_ROOM;
	} else if (choice=='p') {
		//play mode
		mode=PLAY;
		clear();
		if (!FixContextIfRoomIsNull())
			print_room();
	} else if (choice=='q') {
		//quit
		mode=QUIT;
		goto EXIT;
	} else if (choice=='d'&&git_pull_reqd&&context.upstream_url.size()>0) {
		//download (routine)

		//save, initialize git, stage&commit, merge.
		ops::saveAll(am);
		gitops::initRepo(am);
		gitops::stageAndCommit(am,context,"pre-pull commit");
		pullAndMerge();

		//user selects a new option
		git_pull_reqd=false;
		mode=META;
		goto PICK_MODE;
	} else if (choice=='d'&&git_pull_reqd&&
			context.upstream_url.size()==0&&newly_created) {
		//download (first time)

		//initialize git repo if necessary
		gitops::initRepo(am);

		//user enters URL
		print("Please enter a URL for the upstream repository, e.g. "
				"https://github.com/account/repo\n"
				"Leave blank to cancel.");
		std::string input = inputString();

		//user cancels
		if (input.size()==0)
			goto PICK_MODE;

		//update user prefs with new upstream
		context.upstream_url=input;
		context::saveContext(context,am->path+"context.txt");

		//fetch commits from upstream
		if (!tryFetch()) {
			print("Fatal error fetching from source.");
			goto EXIT;
		}

		//merge upstream into local repo
		gitops::merge(am,gitops::FORCE_REMOTE,context);
		print("Download successful.\n\nPress [h] for help.");

		//user goes back to select a different option option
		mode=META;
		git_pull_reqd=false;
		goto PICK_MODE;
	} else {
		//try again
		mode=META;
		goto PICK_MODE;
	}

	//user has fun:
	while (true) {
		//present options based on mode
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
						input(false);
					case 'x':
						goto EXIT;
					default:
						print("[s]ave or e[x]it");
						break;
					}
				}
			} else break;
		}
	}

EXIT:
	print("Exiting game...");
	if (am)
		model::freeModel(am);
	gitops::libgitShutdown();
	clear();
}

void ConsoleUI::editCurrentRoom() {
	assert(mode==EDIT_ROOM);

	//no currently selected room
	if (FixContextIfRoomIsNull()) {
		print("World is empty. Creating initial room...");
		model::rm_id_t init = context.current_room = ops::makeRoom(am);
		ops::editStartRoom(am,init);
	}

	//display options to user
	print_help();

	while (true){
		assert(mode==EDIT_ROOM);
		auto id = context.current_room;
		auto& rm = model::getRoom(am,context.current_room);
		print ("Enter command. Press [h] for help.");

		//user selects option
		switch (input()) {
		case 'q': mode=QUIT;				return; //quit
		case 'p': mode=PLAY; print_room();	return; //switch to play mode
		case 'b': //return to starting room
			context.current_room=am->world.first_room; print_help();
			continue;
		case 'j': //jump to room
			//user selects target:
			id=inputRoom();
			if (!id.is_null()) {
				//if input, jump to room
				context.current_room=id;
				print_help();
			}
			id = context.current_room;
			break;
		case 'i': //display all rooms
			model::loadAllRooms(am);
			for (auto iter : am->world.rooms) {
				print("scenario id "+write_id(iter.first) +" (\""+iter.second.title+"\")");
			}
			break;
		case 'r': //display room
			print_room();
			break;
		case 'y': { //edit room title
			print(
					"Old title: " + rm.title
							+ "\nEnter a new title (blank to cancel):");
			std::string s = inputString();
			if (s.length())
				ops::editRoomTitle(am, id, s.c_str());
			print("\n ## " + rm.title + " ##\n\n");
			if (s.length()) {
				print(
						"Title edited. (Don't forget to [s]ave.) Press [r] to review.\n");
			}
		} break;
		case 't': { //edit room body text
			std::string s=edit_text(rm.body);
			if (s.length()) {
				ops::editRoomBody(am,id,s.c_str());
				//todo: compare to see if edited at all.
				print("body text edited. (Don't forget to [s]ave.). Press [r] to review.\n");
			}
		} break;
		case 'd': //set as dead-end
			//cannot set dead-end if options available
			if (rm.options.size()) {
				print(  "There are options available.\n"
						"Please remove all options manually with [o] then [x] "
						"before marking this as a dead end.");
				break;
			}
			ops::editRoomDeadEnd(am,id,!rm.dead_end);
			print("Dead End: " + std::string(((rm.dead_end) ? "Enabled" : "Disabled")) + ". (Don't forget to [s]ave.)\n");
			break;
		case 's': //save all changes
			print(ops::saveAll(am));
			break;
		case 'h': //display help
			print_help();
			break;
		case 'o': //edit options
			mode=EDIT_OPTIONS;
			print_help();
			editOptions();
			mode=EDIT_ROOM;
			clear();
			print_help();
			break;
		case 'g': //git options
			mode=EDIT_GIT;
			print_help();

			//displays more options...
			editGit();

			//return to edit mode (editGit() changes mode)
			mode=EDIT_ROOM;
			clear();
			print_help();
		default:
			continue;
		}
	}
}

void ConsoleUI::editOptions() {
	//this is a messy function by design
	assert(mode == EDIT_OPTIONS);

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
			case 'q': //return to edit_room mode
			case 'e': return;
			case 'r': //display room
				print(""); print_room(); break;
			case 's': //save all
				print(ops::saveAll(am)); break;
			case 'h': //display help
				print_help(); break;
			case 'a': //add new option to room
				print("Enter text for new option (leave blank to cancel)");
				s = inputString();
				if (s.length()) {
					//define option user is adding.
					//gid.is_null() means the option goes nowhere by default.
					opt = model::option_t();
					opt.dst = model::opt_id_t::null();
					opt.option_text=s;

					//return here if user does not enter a valid option [c|l|e]
				USER_FAILED:
					print("[c]reate scenario for option, [l]ink to existing scenario, or continue [e]diting?");
					switch(input()) {
						case 'd': //d is allowed as an alternative
								  //for c by convention
						case 'c': //create scenario (switch to edit_room mode)
							context.current_room=opt.dst=ops::makeRoom(am);
							ops::editRoomTitle(am,opt.dst,opt.option_text.c_str());
							mode=EDIT_ROOM;
							break;
						case 'l': //link to an existing room
							//update opt.dst iff there is an input entry
							input_id = inputRoom();
							if (input_id.is_null()) {
								print("\nCancelled operation.");
								break;
							} else
								opt.dst=input_id;
							break;
						case 'e': //continue editing
							print_help(); break;
						default:
							goto USER_FAILED;
					}

					//option is now defined to user's tastes; add to room
					ops::addOption(am,id,opt.option_text.c_str(),opt.dst);
					print_help();

					// return to room_edit func if mode was changed above
					if (mode==EDIT_ROOM)
						return;
				} else //no text entered for option
					print("\nCancelled.");
				break;
			case 'x': //remove an existing option:
				print("remove which option? Enter number (0 to cancel)");
				it = input()-'0';
				if (it>0 && it<=9) {
					input_id=model::getOptionID(am,id,it-1);

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
				if (it>0 && it<=9) { //edit an existing option:
					opt = model::option_t();
					input_id=model::getOptionID(am,context.current_room,it-1);
					if (input_id.is_null()) {
						//invalid option input
						print("\nNo such option exists to edit, but you can [a]dd it if you prefer.");
						break;
					}

					//valid option input
					opt_input=rm.options[input_id];

					//display option text
					print("Current text: "+opt_input.option_text);

					//display option target destination
					if (opt_input.dst.is_null())
						print ("No Destination");
					else
						print("Destination: "+write_id(opt_input.dst));
					print("Edit [t]ext, [l]ink to existing destination, or create new [d]estination?");
					switch(input()) {
						case 't': //edit option text
							opt.dst=opt_input.dst;
							opt.option_text=edit_text(opt_input.option_text);
							print("Text edited. Don't forget to [s]ave. Press [r] to review.\n");
							ops::editOption(am,id, input_id, opt);
							continue;
						case 'd': //create new destination for option
							context.current_room=opt.dst=ops::makeRoom(am);
							ops::editOption(am,id, input_id, opt);
							ops::editRoomTitle(am,opt.dst,opt.option_text.c_str());

							//return to edit mode (at new destination room)
							mode=EDIT_ROOM;
							print_help();
							return;
						case 'l': //link option to existing destination
							ops::editOption(am,id, input_id, {inputRoom(), opt_input.option_text});
							print("\noption edited. Still editing room " + write_id(id) + "(\"" + rm.title + "\").\n");
							continue;
						default:
							print("\nCancelled.");
							continue;
					}
					print("\nInvalid option number.");
					break;
				} else //non-numeric
					print("Unknown command");
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
	case 'q': mode=QUIT; 						break; //quit game
	case 'e': mode=EDIT_ROOM; 					break; //switch to edit mode
	case 'b': context.current_room=am->world.first_room;
			print_room(); 						break; //restart
	case 'r': print(""); print_room();			break; //display room text again
	case 'h': print_help();						break; //display help
	default:
		if (c>='1'&&c<='9') { //select option (and follow to room)
			int choice = c-'0';
			//select choice-th option:
			for (auto iter : rm.options) { //find desired option
				choice--;
				if (choice==0) { //found desired option
					if (iter.second.dst.is_null()) {
						//no destination for given option
						print("This option's scenario has not been written.\n"
								"You may [c]reate a scenario for the option, "
								"[r]eturn to pick another option, "
								"[e]dit the current scenario, "
								"or [b]egin again from the start.");
						model::option_t opt_edit = iter.second;
						switch (input()) {
							case 'd': //alternative for 'c' (by convention)
							case 'c': //create scenario
								opt_edit.dst=ops::makeRoom(am);
								ops::editOption(am,context.current_room,iter.first,opt_edit);
								ops::editRoomTitle(am,opt_edit.dst,opt_edit.option_text.c_str());
								context.current_room=opt_edit.dst;
								mode=EDIT_ROOM;
									break;
							case 'r': //pick another option
								print_room();
								return;
							case 'e': //drop into edit mode
								mode=EDIT_ROOM;
								return;
							case 'b': //restart
								context.current_room=am->world.first_room;
								print_room();
								return;
							default: //cancel
								print_room();
						}
						return;
					} else { //follow link
						context.current_room=iter.second.dst;
						context::saveContext(context,am->path+"context.txt");
						print_room();
					}
					return;
				}
			}
			//no option of given number
			print("Invalid option number");
		}
		//user selected improper option
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
