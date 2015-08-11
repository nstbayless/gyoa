/*
 * ConsoleUI_io.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: n
 */

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../backend/id_parse.h"
#include "../../backend/model/Room.h"
#include "ConsoleUI.h"

namespace cyoa {
namespace ui {

model::rm_id_t ConsoleUI::inputRoom() {
	ops.fullyLoad();
	try_again:
	print("Enter an id, e.g. 3 or 4:32948395, or leave blank to cancel");

	std::string s = inputString();
	auto f = s.find(':');
	if (f==std::string::npos&&!s.empty()){
		//find room with given gidr
		int gid = atoi(s.c_str());
		std::vector<model::rm_id_t> matches;
		for (auto iter : model.rooms){
			if (iter.first.gid==gid)
				matches.push_back(iter.first);
		}
		if (matches.empty()) {
			print("No matches for gid " + std::to_string(gid) + ":*  of " + std::to_string(model.rooms.size()) + " possible results.");
			goto try_again;
		}
		if (matches.size()==1) {
			print("Match found: " + write_id(matches[0])+" (\"" + model.rooms[matches[0]].title + "\")");
			print("Confirm? [y]:");
			if (input()=='y') {
				print("selected id " + write_id(matches[0]));
				return matches[0];
			}
			goto try_again;
		}
		print(std::to_string(matches.size()) + " matches found: \n");
		for (auto iter : matches) {
			print(write_id(iter) + " (\"" + model.rooms[iter].title + "\")");
		}
		print("\nPlease enter complete a:b id.");
		goto try_again;
	}

	if (f!=std::string::npos) {
		auto id = parse_id(s);
		if (model.rooms.count(id)) {
			print("selected id " + write_id(id));
			return id;
		}
		else {
			print("Invalid ID.");
			goto try_again;
		}
	}

	print("cancelled or incorrect input. Returning to previous screen.");
	//default (cancel) value:
	return {-1,-1};
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
	string file = "/tmp/gyoa_edit.tmp";
	ofstream out;
	out.open (file);
	out << original;
	out.close();

	std::string command = "nano "+file;
	system(command.c_str());

	std::ifstream in(file);
	std::stringstream buffer;
	buffer << in.rdbuf();

	std::remove(file.c_str());

	return buffer.str();
}

void ConsoleUI::clear() {
	system("clear");
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
					"[b]   edit the opening scenario\n"
					"[r]   read scenario description\n"
					"[y]   edit header title\n"
					"[t]   edit text\n"
					"[o]   edit options\n"
					"[d]   toggle dead end\n"
					"[j]   jump to a scenario by id\n"
					"[s]   save all\n"
					"[g]   for synchronization options (git).\n"
					"[h]   view this screen");
			print("\nediting scenario "+write_id(current_room) + " (\"" + ops.loadRoom(current_room).title +"\")");
			if (ops.savePending())
				print("\nalert: un[s]aved changes.");
			break;
	case EDIT_OPTIONS:
			print("## HELP MENU: Edit Options Mode ##\n"
					"[q|e] return to edit mode\n"
					"[r]   read scenario description\n"
					"[a]   add option\n"
					"[x]   remove option\n"
					"[1-9] edit option\n"
					"[s]   save all\n"
					"[h]   view this screen");
			break;
	case EDIT_GIT:
			print("## HELP MENU: Git Synchronization Mode ##\n"
					"[q|e] return to edit mode\n"
					"[s]   save all changes (doesn't synchronize)\n"
					"[m]   download/merge from repository\n"
					"[p]   upload to repository (git add + commit + push)\n"
					"[u]   set upstream repository\n"
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
	if (mode==PLAY)
		clear();
	print("## " + rm.title + " ##");
	if (mode != PLAY)
		print("gid: " + write_id(id));
	print("\n#> " + rm.body + "\n");

	for (auto iter : rm.options) {
		assert(opt_n < 10);
		if (mode==PLAY)
			print("[" + std::to_string(opt_n) + ((iter.second.dst.gid==-1)?"]*> ":"] > ") + iter.second.option_text);
		else {
			print(" "+std::to_string(opt_n)+": "+ iter.second.option_text);
			if (iter.second.dst.gid==-1)//no destination
				print("\t(no destination)");
			else
				print("\tdestination: scenario id " + write_id(iter.second.dst) + " (\"" + ops.loadRoom(iter.second.dst).title + "\")\n");
		}
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
	} else if (mode==PLAY)
		print("\nPress [h] for help.\n");
}

}}
