/*
 * ConsoleUI.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#include "ConsoleUI.h"

#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>

#include "../../backend/id_parse.h"

namespace cyoa {
namespace ui {

ConsoleUI::ConsoleUI() {
	ops.setModel(model);
}

ConsoleUI::~ConsoleUI() {

}

void ConsoleUI::start() {
	clear();
	print("loading world...");
	model=ops.loadWorld();
	if (ops.loadResult()) {
		print("world found! \""+model.title+'"');
	} else {
		print("No world found. Created new world instead.");
		world_edited=true;
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
	} else {
		mode=META;
		goto user_failed;
	}

	//user has fun:
	while (mode!=QUIT) {
		if (mode==EDIT_ROOM)
			editCurrentRoom();
		if (mode==PLAY)
			playCurrentRoom();
	}
	print("Exiting game...");
}

void ConsoleUI::print(std::string message) {
	std::cout<<message<<"\n";
}

char ConsoleUI::input() {
	std::string s = inputString();
	if (!s.length())
		return ' ';
	return tolower(s[0]);
}

std::string ConsoleUI::inputString() {
	using namespace std;
	std::string s;
	getline(cin,s);
	return s;
}

std::string ConsoleUI::edit_text(std::string original) {
	using namespace std;
	string file = "/tmp/cyoa_edit.tmp";
	ofstream out;
	out.open (file);
	out << original;
	out.close();

	std::string command = "nano "+file;
	system(command.c_str());

	std::ifstream in(file);
	std::stringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

void ConsoleUI::clear() {
	system("clear");
}

void ConsoleUI::editCurrentRoom() {
	assert(mode==EDIT_ROOM);
	print_help();
	auto id = current_room;
	auto& rm = ops.loadRoom(current_room);
	std::string s;
	while (true){
		switch (input()) {
		case 'q': mode=QUIT;						return;
		case 'p': mode=PLAY;						return;
		case 'b': current_room=model.first_room;	continue;
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
			break;
		default:
			print("No. Press [h] for help.");
			continue;
		}
	}
}

void ConsoleUI::editOptions() {
	assert(mode == EDIT_OPTIONS);
	int it;
	std::string s;
	model::option_t opt;
	auto id = current_room;
	auto& rm = ops.loadRoom(current_room);
	while (true) {
		switch (char c = input()) {
			case 'q':
			case 'e': return;
			case 'r': print_room(); break;
			case 's': save_all();
					break;
			case 'h': print_help(); break;
			case 'a': print("Enter text for new option (leave blank to cancel)");
			s = inputString();
			if (s.length()) {
				opt.dst.gid=-1;
				opt.option_text=s;
				user_failed:
				print("[c]reate scenario for option or continue [e]diting?");
				switch(input()) {
					case 'd':
					case 'c': current_room=opt.dst=ops.makeRoom();
					ops.save(id);
					break;
					//todo: link room
					case 'e': print("\nReturned to edit mode. Press [h] for help."); break;
					default: goto user_failed;
				}
				ops.addOption(id,opt);
				room_edited[id]=true;
				print_room();
			} else
				print("Cancelled.");
			break;
			case 'x': print("remove which option? Enter number (0 to cancel)");
			it = input()-'0';
			if (it>0 && it<=9) {
				for (auto iter : rm.options) {
					it-=1;
					if (it==0)
					ops.removeOption(id,iter.first);
					room_edited[id]=true;
				}
			}
			break;
			default:
			it = c-'0';
			if (it>0 && it<=9) {
				for (auto iter : rm.options) {
					it-=1;
					if (it==0) {
						print("Current text: "+iter.second.option_text);
						if (iter.second.dst.gid==-1)
							print ("No Destination");
						else
							print("Destination: "+write_id(iter.second.dst));
						print("Edit [t]ext, [l]ink to existing destination, or create new [d]estination?");
						switch(input()) {
							case 't':
								opt.dst=iter.second.dst;
								opt.option_text=iter.second.option_text;
								ops.editOption(id,iter.first, opt);
								break;
							case 'd':
								current_room=opt.dst=ops.makeRoom();
								room_edited[id]=true;
								room_edited[opt.dst]=true;
								print_room();
								break;
							case 'l':
								//todo link room
							default:
								break;
						}
					}
				}
			} else
			print("Press [h] for help.");
		}}
}

void ConsoleUI::print_help() {
	clear();
	switch (mode) {
	case PLAY:
		print("## HELP MENU: Play Mode ##\n"
				"[q]   exit program\n"
				"[e]   switch to edit mode\n"
				"[b]   start over\n"
				"[r]   reread scenario description\n"
				"[h]   view this screen\n"
				"[1-9] select option");
		break;
	case EDIT_ROOM:
		print("## HELP MENU: Edit Mode ##\n"
				"[q]   exit program\n"
				"[p]   switch to play mode\n"
				"[b]   edit the starting room\n"
				"[r]   read scenario description\n"
				"[y]   edit header title\n"
				"[t]   edit text\n"
				"[o]   edit options\n"
				"[d]   toggle dead end\n"
				"[s]   save all\n"
				"[h]   view this screen");
		break;
	case EDIT_OPTIONS:
		print("## HELP MENU: Edit Options Mode ##\n"
				"[q|e] return to previous screen\n"
				"[r]   read scenario description\n"
				"[a]   add option\n"
				"[x]   remove option\n"
				"[1-9] edit option\n"
				"[s]   save all\n"
				"[h]   view this screen");
		break;
	default:
		print("No help available.");
		break;
	}
	print("");
}

void ConsoleUI::print_room() {
	int opt_n = 1;
	auto id = current_room;
	auto& rm = ops.loadRoom(current_room);
	print("## " + rm.title + " ##");
	if (mode != PLAY)
		print("gid: " + write_id(id));
	print("\n#> " + rm.body + "\n");

	for (auto iter : rm.options) {
		assert(opt_n < 10);
		print("[" + std::to_string(opt_n) + "] > " + iter.second.option_text);
		opt_n++;
	}
	if (!rm.options.size()) {
		if (rm.dead_end)
			if (mode != PLAY)
				print("\n[dead end].");
			else
				print("\nYou've reached a dead end.");
		else if (mode == PLAY)
			print("\n> This is as far as anyone has written so far. "
					"You may [b]egin play again, [q]uit, or [e]xtend the story from this "
					"point on for other players to enjoy.");
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
	print("done");
}

void ConsoleUI::playCurrentRoom() {
	assert(mode==PLAY);
	auto& rm = ops.loadRoom(current_room);
	print("");
	print_room();
	print("\nPress [h] for help.\n");

	user_failed:
	switch(char c = input()){
	case 'q': mode=QUIT; 						break;
	case 'e': mode=EDIT_ROOM; 						break;
	case 'b': current_room=model.first_room; 	break;
	case 'r': 									break;
	case 'h': print_help();						break;
	default:
		if (c>='1'&&c<='9') {
			int choice = c-'0';
			//select choice-th option:
			for (auto iter : rm.options) {
				choice--;
				if (choice==0)
					current_room=iter.second.dst;
				return;
			}
			assert(false);
		} else
			//user failed!
			print("Nope. Press [h] for help, or [r]eread the descriptive text above.");
			goto user_failed;
	}
	return;
}

} /* namespace ui */
} /* namespace cyoa */
