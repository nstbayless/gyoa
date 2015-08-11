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
	ops.setModel(model);
}

ConsoleUI::~ConsoleUI() {

}

void ConsoleUI::start() {
	clear();
	system("mkdir data");
	print("loading world...");
	model=ops.loadWorld();
	if (ops.loadResult()) {
		print("world found! \""+model.title+'"');
	} else {
		print("No world found. Created new world instead.");
		world_edited=true;
		room_edited[model.first_room]=true;
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
			if (world_edited || room_edited.size()) {
				print(
						"Note: unsaved changes! Press [s] to save, [x] to confirm quit.");
				while (true) {
					switch (input()) {
					case 's':
						save_all();
					case 'x':
						goto exit;
					default:
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
	auto id = current_room;
	auto& rm = ops.loadRoom(current_room);
	std::string s;
	while (true){
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
				print("Title edited. (Don't forget to [s]ave.)\n");
				room_edited[id]=true;
			}
			break;
		case 't':
			s=edit_text(rm.body);
			if (s.length()) {
				ops.editRoomBody(id,s);
				//todo: compare to see if edited at all.
				print("body text edited. (Don't forget to [s]ave.)\n");
				room_edited[id]=true;
			}
			break;
		case 'd':
			ops.editRoomDeadEnd(id,!rm.dead_end);
			print("Dead End: " + (rm.dead_end) ? "Enabled" : "Disabled. (Don't forget to [s]ave.)\n");
			room_edited[id]=true;
			break;
		case 's':
			save_all();
			break;
		case 'h':
			print_help();
			break;
		case 'o':
			mode=EDIT_OPTIONS;
			print_help();
			editOptions();
			mode=EDIT_ROOM;
			print_help();
			break;
		default:
			print("No. Press [h] for help.");
			continue;
		}
	}
}

void ConsoleUI::editOptions() {
	//this is a messy function
	assert(mode == EDIT_OPTIONS);
	auto id = current_room;
	auto& rm = ops.loadRoom(current_room);

	//declaring these local variables in advance
	//because variables cannot be declared within a switch
	int it;
	std::string s;
	model::option_t opt;
	model::option_t opt_input;
	model::rm_id_t input_id;

	//repeatedly get input from user and execute, break if 'q' or 'e' input
	while (true) {
		switch (char c = input()) {
			case 'q':
			case 'e': return;
			case 'r': print_room(); break;
			case 's': save_all();
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
								room_edited[opt.dst]=true;
								world_edited=true;
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

						//option added: mark this room as being edited.
						room_edited[id]=true;
						print_room();
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
						room_edited[id]=true;
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
								opt.option_text=opt_input.option_text;
								ops.editOption(id, input_id, opt);
								break;
							case 'd':
								current_room=opt.dst=ops.makeRoom();
								ops.editOption(id, input_id, opt);
								room_edited[id]=true;
								room_edited[opt.dst]=true;
								world_edited=true;
								print_room();
								break;
							case 'l':
								ops.editOption(id, input_id, {inputRoom(), opt_input.option_text});
								break;
							default:
								print("\ncancelled.");
								continue;
						}
					}
					print("Invalid option. Press [h] for help.");
					break;
		}
	}
}

void ConsoleUI::save_all() {
	if (world_edited) {
		print("Saving world information...");;
		ops.saveWorld();
	}
	for (auto iter : room_edited) if (iter.second) {
		std::string s = ops.save(iter.first);
		print("saved scenario to " + s);
	}
	if (!world_edited&&room_edited.empty())
		print ("Nothing to save. Don't forget to commit.");
	print("Done. Don't forget to commit and push.");
	world_edited=false;
	room_edited.clear();
}

void ConsoleUI::playCurrentRoom() {
	assert(mode==PLAY);
	auto& rm = ops.loadRoom(current_room);

	user_failed:
	switch(char c = input()){
	case 'q': mode=QUIT; 									break;
	case 'e': mode=EDIT_ROOM; 								break;
	case 'b': current_room=model.first_room; print_room(); 	break;
	case 'r': print_room();									break;
	case 'h': print_help();									break;
	default:
		if (c>='1'&&c<='9') {
			int choice = c-'0';
			//select choice-th option:
			for (auto iter : rm.options) {
				choice--;
				if (choice==0) {
					if (iter.second.dst.gid==-1)//no destination for given option
						print("This option's scenario has not been written. Press [e] to enter edit mode, "
								"then [o] to edit options.");
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
