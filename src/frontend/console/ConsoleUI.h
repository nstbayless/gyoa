/*
 * ConsoleUI.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef FRONTEND_CONSOLE_CONSOLEUI_H_
#define FRONTEND_CONSOLE_CONSOLEUI_H_

#include <string>

#include "gyoa/Context.h"
#include "gyoa/Room.h"
#include "gyoa/World.h"
#include "gyoa/Operation.h"

namespace gyoa {
namespace gitops {
struct push_cred;
}
namespace ui {

//! quick-and-dirty console-based user interface
class ConsoleUI {
public:
	ConsoleUI();
	virtual ~ConsoleUI();

	//! starts ui loop
	void start(std::string path="data/");
public:
	//! gets input from user
	char input(bool prompt=true);

	//! prints message to user
	void print(std::string);

private:
	//! prints help command to user
	void print_help();

	//! prints the current room.
	void print_room();

	//! clears console
	void clear();

	//! allows user to edit text by a system call to nano.
	//! input: original text. output: new text.
	std::string edit_text(std::string);

	//! gets string input from user
	std::string inputString(bool prompt=true);

	//! gets a rm_id_t from the user, or {-1,-1} if user cancels.
	model::rm_id_t inputRoom();

	//! allows user to edit model
	//! user makes changes to the current room viewed (context::current_room)
	void editCurrentRoom();

	//! allows user to edit options of current scenario
	void editOptions();

	//! allows user to synchronize data to external repository.
	void editGit();

	//! retrieves git authentication credentials from user
	//! returns 0 on success, 1 on abort.
	int getCredentials(gitops::push_cred&);

	//! pulls and merges, asks user for input if there are conflicts.
	//! All changes should be committed first.
	//! returns true if successful
	//! uses given creds, and updates them if modified by user
	bool pullAndMerge(gitops::push_cred* cred_=nullptr);

	//! first fetches without authentication. Then attempts
	//! by asking user for cred.
	//! overwrites cred with credentials attained if possible
	//! returns true if successful
	bool tryFetch(gitops::push_cred* cred=nullptr);

	//! allows user to play game.
	void playCurrentRoom();

	//! if context.current_room.is_null() then fix.
	//! returns true if switching mode.
	bool FixContextIfRoomIsNull();
private:
	//! model stores information about all rooms
	model::ActiveModel* am=nullptr;

	enum {
		EDIT_ROOM, EDIT_OPTIONS, EDIT_GIT, PLAY, META, QUIT
	} mode = META;

	context::context_t context;

	struct {
		//todo: allow user to edit
		std::string text_editor="nano";
	} usr_prefs;

};

} /* namespace ui */
} /* namespace gyoa */

#endif /* FRONTEND_CONSOLE_CONSOLEUI_H_ */
