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

#include "../../backend/id_parse.h"
#include "../../backend/model/Read.h"

namespace cyoa {
namespace ui {

ConsoleUI::ConsoleUI() {
}

ConsoleUI::~ConsoleUI() {

}

void ConsoleUI::start() {
	clear();
	system("mkdir data");
	print("loading world...");
	ops.setModel(model=ops.loadWorld());
	if (ops.loadResult()) {
		print("world found! \""+model.title+'"');
	} else {
		print("No world found. Created new world instead.");
	}

	current_room=model.first_room;

	//user selects mode:
	user_failed:
	print("Would you like to [e]dit or [p]lay?");
	char choice = input();
	if (choice=='e') {
		mode=EDIT_ROOM;
	} else if (choice=='p') {
		mode=PLAY;
		clear();
		print_room();
	} else {
		mode=META;
		goto user_failed;
	}

	//user has fun:
	while (true) {
		if (mode==EDIT_ROOM)
			editCurrentRoom();
		if (mode==PLAY)
			playCurrentRoom();
		if (mode==QUIT){
			if (ops.savePending()) {
				print(
						"Note: unsaved changes! Press [s] to save, [x] to confirm quit.");
				while (true) {
					switch (input()) {
					case 's':
						print(ops.saveAll());
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
	clear();
}

void ConsoleUI::editCurrentRoom() {
	assert(mode==EDIT_ROOM);
	print_help();
	std::string s;
	while (true){
		auto id = current_room;
		auto& rm = ops.loadRoom(current_room);

		switch (input()) {
		case 'q': mode=QUIT;									return;
		case 'p': mode=PLAY; print_room();						return;
		case 'b': current_room=model.first_room; print_help();	continue;
		case 'j': id=inputRoom();
			if (id.gid!=-1) {
				//if input, jump to room
				current_room=id;
				print_help();
			}
			id = current_room;
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
				ops.editRoomTitle(id,s);
			print("\n ## " + rm.title + " ##\n\n");
			if (s.length()) {
				print("Title edited. (Don't forget to [s]ave.) Press [r] to review.\n");
			}
			break;
		case 't':
			s=edit_text(rm.body);
			if (s.length()) {
				ops.editRoomBody(id,s);
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
			ops.editRoomDeadEnd(id,!rm.dead_end);
			print("Dead End: " + std::string(((rm.dead_end) ? "Enabled" : "Disabled")) + ". (Don't forget to [s]ave.)\n");
			break;
		case 's':
			print(ops.saveAll());
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
			print("No. Press [h] for help.");
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
		auto id = current_room;
		auto& rm = ops.loadRoom(current_room);

		switch (char c = input()) {
			case 'q':
			case 'e': return;
			case 'r': print(""); print_room(); break;
			case 's': print(ops.saveAll());
					break;
			case 'h': print_help(); break;

			//add new option to room:
			case 'a': print("Enter text for new option (leave blank to cancel)");
					s = inputString();
					if (s.length()) {
						//define option user is adding.
						//gid=-1 means the option goes nowhere by default.
						opt.dst.gid=-1;
						opt.option_text=s;

						//return here if user does not enter a valid option [c|l|e]
						user_failed:
						print("[c]reate scenario for option, [l]ink to existing scenario, or continue [e]diting?");
						switch(input()) {
							case 'd': //d is allowed as an alternative for c as when editing options below.
							case 'c': current_room=opt.dst=ops.makeRoom();
								mode=EDIT_ROOM;
								break;
							case 'l':
								//update opt.dst iff input.gid!=-1 (indicating no input entry)
								input_id = inputRoom();
								if (input_id.gid==-1) {
									print("\nCancelled operation. Press [h] for help.");
									break;
								} else
									opt.dst=input_id;
							case 'e': print_help(); break;
							default: goto user_failed;
						}

						//option defined to user's tastes; add to room:
						ops.addOption(id,opt);
						print_help();
						print("\nPress [h] for help.");
					} else
						print("\nCancelled. Press [h] for help.");
					break;

			//remove an existing option:
			case 'x': print("remove which option? Enter number (0 to cancel)");
					it = input()-'0';
					if (it>0 && it<=9) {
						input_id=model::getOption(rm,it);

						if (input_id.gid==-1) {
							//invalid option input:
							print("\nNo such option exists to remove.");
							break;
						}
						//valid option input:
						ops.removeOption(id,input_id);
						print("\nOption removed. Press [h] for help.");
						break;
					}
					break;

			default:
					it = c-'0';
					//edit an existing option:
					if (it>0 && it<=9) {
						input_id=model::getOption(rm,it);
						if (input_id.gid==-1) {
							//invalid option input:
							print("\nNo such option exists to edit, but you can [a]dd it if you prefer.");
							break;
						}
						//valid option input:
						opt_input=rm.options[input_id];
						print("Current text: "+opt_input.option_text);
						if (opt_input.dst.gid==-1)
							print ("No Destination");
						else
							print("Destination: "+write_id(opt_input.dst));
						print("Edit [t]ext, [l]ink to existing destination, or create new [d]estination?");
						switch(input()) {
							case 't':
								opt.dst=opt_input.dst;
								opt.option_text=edit_text(opt_input.option_text);
								print("Text edited. Don't forget to [s]ave. Press [r] to review.\n");
								ops.editOption(id, input_id, opt);
								continue;
							case 'd':
								current_room=opt.dst=ops.makeRoom();
								ops.editOption(id, input_id, opt);
								mode=EDIT_ROOM;
								print_help();
								return;
							case 'l':
								ops.editOption(id, input_id, {inputRoom(), opt_input.option_text});
								print("\noption edited. Still editing room " + write_id(id) + "(\"" + rm.title + "\"). Press [h] for help.\n");
								continue;
							default:
								print("\nCancelled.");
								continue;
						}
						print("\nInvalid option number. Press [h] for help.");
						break;
					}
					print("\nPress [h] for help.");
					break;
		}
	}
}


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
			ops.gitOps.pull();
			print("\nType [h] for help.");
			continue;
		case 'p': //push
			print("Enter a commit message, or leave blank for default");
			s=inputString();
			if (s.size()==0)
				s="Default commit message.";
			ops.gitOps.init();
			ops.gitOps.addAll();
			ops.gitOps.commit(s);
			ops.gitOps.push();
			print("\nType [h] for help.");
			continue;
		case 'u': //set upstream
			print("Enter upstream repository url, e.g. https://github.com/account/gyoa");
			s=inputString();
			print("Upstream URL: " + s);
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

void ConsoleUI::playCurrentRoom() {
	user_failed:
	assert(mode==PLAY);
	auto& rm = ops.loadRoom(current_room);
	switch(char c = input()){
	case 'q': mode=QUIT; 									break;
	case 'e': mode=EDIT_ROOM; 								break;
	case 'b': current_room=model.first_room; print_room(); 	break;
	case 'r': print(""); print_room();						break;
	case 'h': print_help();									break;
		default:
		if (c>='1'&&c<='9') {
			int choice = c-'0';
			//select choice-th option:
			for (auto iter : rm.options) {
				choice--;
				if (choice==0) {
					if (iter.second.dst.gid==-1) { //no destination for given option
						print("This option's scenario has not been written.\n"
								"You may [c]reate a scenario for the option, "
								"[r]eturn to pick another option, "
								"[e]dit the current scenario, "
								"or [b]egin again from the start.");
						model::option_t opt_edit = iter.second;
						switch (input()) {
							case 'd': //alternative for 'c'
							case 'c':
									opt_edit.dst=ops.makeRoom();
									ops.editOption(current_room,iter.first,opt_edit);
									current_room=opt_edit.dst;
									mode=EDIT_ROOM;
										break;
							case 'r':
									print_room();
										return;
							case 'e':
									mode=EDIT_ROOM; return;
							case 'b':
									current_room=model.first_room; print_room(); return;
							default:
									print_room();
						}
					}
					else {
						current_room=iter.second.dst;
						print_room();
					}
					return;
				}
			}
		}
		//user failed!
		print("Nope. Press [h] for help, or [r]eread the descriptive text above.");
		goto user_failed;
	}
	return;
}

} /* namespace ui */
} /* namespace cyoa */
