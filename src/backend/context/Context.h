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
 * such as current room, room history, user prefs, setc.*/
struct context_t {
	//! username of user
	std::string user_name   ="gyoa_client";

	//! email address of user
	std::string user_email  ="?@?";

	//! URL of origin
	std::string upstream_url="";

	//! if this is non-empty, will be used as the last-common-commit for merging
	std::string common_commit_oid="";

	//! current room being viewed in play mode
	model::rm_id_t current_room=model::id_type::null();
};

} /* namespace context */
} /* namespace gyoa */

#endif /* BACKEND_CONTEXT_CONTEXT_H_ */
