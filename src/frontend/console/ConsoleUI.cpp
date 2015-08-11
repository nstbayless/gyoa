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
	print("loading world...");
	model=ops.loadWorld();
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
		mode=EDIT;
	} else if (choice=='p') {
		mode=PLAY;
	} else {
		mode=META;
		goto user_failed;
	}

	//user has fun:
	while (mode!=QUIT) {
		if (mode==EDIT)
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
	return tolower(inputString()[0]);
}

std::string ConsoleUI::inputString() {
	using namespace std;
	std::string s;
	getline(cin,s);
	return s;
}


void ConsoleUI::editCurrentRoom() {
	assert(mode==EDIT);
	print_help();
	auto id = current_room;
	auto& rm = ops.loadRoom(current_room);
	int opt_n;
	std::string s;
	while (true){
		switch (input()) {
		case 'q': mode=QUIT;						return;
		case 'p': mode=PLAY;						return;
		case 'b': current_room=model.first_room;	continue;
		// print room description:
		case 'r':
			opt_n=1;
			print("## " + rm.title + " ##");
			print("gid: "+write_id(id));
			print("\n#> " + rm.body + "\n");
			if (rm.options.size())
				for (auto iter : rm.options) {
					assert(opt_n < 10);
					print(
							"[" + std::to_string(opt_n) + "] > "
									+ iter.second.option_text);
					opt_n++;
				}
			else {
				if (rm.dead_end)
					print("[Dead End].");
			}
			break;
		case 'y':
			print("Old title: " + rm.title
							+ "\nEnter a new title (blank to cancel):");
			s=inputString();
			if (s.length())
				ops.editRoomTitle(id,s);
			print("\n ## " + rm.title + " ##\n\nTitle edited. (Don't forget to [s]ave.)\n");
			break;
		case 't':
			s=edit_text(rm.body);
			if (s.length())
				ops.editRoomBody(id,s);
			print("body text edited. (Don't forget to [s]ave.)\n");
			break;
		case 'd':
			ops.editRoomDeadEnd(id,!rm.dead_end);
			print("Dead End: " + (rm.dead_end) ? "Enabled" : "Disabled. (Don't forget to [s]ave.)\n");
			break;
		case 's':
			print("Saving room...");
			print("Room saved to " + ops.save(id)+"\n");
			break;
		case 'h':
			print_help();
			break;
		default:
			print("No. Press [h] for help.");
			continue;
		}
	}
}

void ConsoleUI::print_help() {
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
	case EDIT:
		print("## HELP MENU: Edit Mode ##\n"
				"[q]   exit program\n"
				"[p]   switch to play mode\n"
				"[b]   edit the starting room\n"
				"[r]   read scenario description\n"
				"[y]   edit header title\n"
				"[t]   edit text\n"
				"[o]   edit options\n"
				"[d]   toggle dead end\n"
				"[s]   save changes to room\n"
				"[h]   view this screen");
		break;
	default:
		print("No help available.");
		break;
	}
	print("");
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

void ConsoleUI::playCurrentRoom() {
	assert(mode==PLAY);
	auto& rm = ops.loadRoom(current_room);
	print("");
	print("## " + rm.title + " ##\n");
	print("#> " + rm.body+"\n");

	// increments for each option, numbers each option.
	int opt_n=1;

	if (rm.options.size())
		for (auto iter : rm.options) {
			assert(opt_n<10);
			print("[" + std::to_string(opt_n) + "] > " + iter.second.option_text);
			opt_n++;
		}
	else {
		if (rm.dead_end)
			print("You've reached a dead end.");
		else
			print("> This is as far as anyone has written so far. "
					"You may [b]egin play again, [q]uit, or [e]xtend the story from this "
					"point on for other players to enjoy.");
	}
	print("\nPress [h] for help.\n");

	user_failed:
	switch(char c = input()){
	case 'q': mode=QUIT; 						break;
	case 'e': mode=EDIT; 						break;
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
