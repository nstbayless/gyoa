/*
 * ConsoleUI.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef FRONTEND_CONSOLE_CONSOLEUI_H_
#define FRONTEND_CONSOLE_CONSOLEUI_H_

#include <string>

#include "../../backend/model/Room.h"
#include "../../backend/model/World.h"
#include "../../backend/ops/Operation.h"

namespace cyoa {
namespace ui {

class ConsoleUI {
public:
	ConsoleUI();
	virtual ~ConsoleUI();

	//! starts ui loop
	void start();
private:
	//! prints message to user
	void print(std::string);

	//! prints help command to user
	void print_help();

	//! prints the current room.
	void print_room();

	//! saves all edited information.
	void save_all();

	//! clears console
	void clear();

	//! allows user to edit text by a system call to nano.
	//! input: original text. output: new text.
	std::string edit_text(std::string);

	//! gets input from user
	char input();

	//! gets string input from user
	std::string inputString();

	//! gets a rm_id_t from the user, or {-1,-1} if user cancels.
	model::rm_id_t inputRoom();

	//! allows user to edit model
	void editCurrentRoom();

	//! allows user to edit options of current scenario
	void editOptions();

	//! allows user to play game.
	void playCurrentRoom();
private:
	//! edits model
	model::world_t model;

	//! modifies model
	ops::Operation ops;

	enum {
		EDIT_ROOM, EDIT_OPTIONS, PLAY, META, QUIT
	} mode = META;

	model::rm_id_t current_room;
	std::map<model::rm_id_t,bool> room_edited;
	bool world_edited=false;

	struct {
		std::string text_editor="nano";
	} usr_prefs;

};

} /* namespace ui */
} /* namespace cyoa */

#endif /* FRONTEND_CONSOLE_CONSOLEUI_H_ */
