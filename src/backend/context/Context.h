/*
 * Context.h
 *
 *  Created on: Aug 17, 2015
 *      Author: n
 */

#ifndef BACKEND_CONTEXT_CONTEXT_H_
#define BACKEND_CONTEXT_CONTEXT_H_

#include <string>

#include "../model/Room.h"

namespace gyoa {
namespace context {

/**stores context information for a gyoa session,
 * such as current room, room history, user prefs, etc.*/
struct context_t {
	//! username of user
	std::string user_name   ="gyoa_client";

	//! email address of user
	std::string user_email  ="?@?";

	//! URL of origin
	std::string upstream_url="";

	//! current room being viewed in play mode
	model::rm_id_t current_room=model::id_type::null();

	struct {
		bool do_not_store=false;
		std::string user_name="";
		std::string path_to_privkey="";
		std::string path_to_pubkey="";
		//password not stored by design
	} git_authentication_prefs;
};

//! loads user context information (which room currently in, etc.) from the given file
context_t loadContext(std::string path);

//! saves user context information (current room, username, etc.) to the given file
void saveContext(context_t, std::string path);

} /* namespace context */
} /* namespace gyoa */

#endif /* BACKEND_CONTEXT_CONTEXT_H_ */
