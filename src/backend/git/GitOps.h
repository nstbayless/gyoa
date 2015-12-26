/*
 * GitOps.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef BACKEND_OPS_GITOPS_H_
#define BACKEND_OPS_GITOPS_H_

#include <string>
#include <utility>
#include <vector>

struct git_tag;

namespace gyoa {
namespace model {
struct id_type;
struct world_t;
struct ActiveModel;
} /* namespace model */
namespace ops {
class Operation;
} /* namespace ops */
} /* namespace gyoa */

struct git_remote;

struct git_tree;

struct git_commit;

struct git_repository;

struct git_signature;

namespace gyoa {
namespace context {
struct context_t;
} /* namespace context */
namespace gitops {

//! method with which to resolve conflicts when merging
enum merge_style {
	USE_LOCAL,		//!< if a conflict exists, use local change
	USE_REMOTE,		//!< if a conflict exists, use remote change
	FORCE_LOCAL,	//!< don't check if conflict exists; use local version
	FORCE_REMOTE,	//!< don't check if conflict exists; use remote version
	MANUAL,			//!< alert user to error
	TRIAL_RUN,		//!< do not merge, do not update common history, error returned if conflict exists
};

//! Represents a single instance of a conflict between the server and client
struct MergeConflict {
	//! what type of data conflicted?
	enum {
		STRING,
		ID,
		BOOL,
	} data_type;
	//! description of the conflict
	std::string description="";

	//! [data] version from common ancestor commit
	std::string common;

	//! [data] version from remote (server) commit
	std::string remote;

	//! [data] version from local commit
	std::string local;

	//! pointer to data where change should be made.
	void* data_ptr;
};

//! returned by a call to GitOps::Merge
struct MergeResult {
	//! list of conflicts with pointers to data to be manually resolved
	std::vector<MergeConflict> conflicts;

	//! number of times a conflict was automatically resolved
	int resolved=0;

	//! number of changes made to the local model
	int changes=0;

	bool conflict_occurred(){
		return resolved>0||conflicts.size();
	}
};

struct push_cred {
	enum {
		USERNAME,	//!< just a username [a]
		PLAINTEXT,	//!< username [a] and password [b]
		SSH,		//!< path to private ssh key [c] and public [d] and username [a] and passphrase [b]
	} credtype;
	std::string username="";
	std::string pass="";
	std::string privkey="";
	std::string pubkey="";
};

push_cred make_push_cred_username(std::string username);
push_cred make_push_cred_plaintext(std::string username, std::string password);
push_cred make_push_cred_ssh(std::string path_to_privkey, std::string path_to_pubkey, std::string username, std::string passphrase);

void gitInit();
void gitShutdown();

//! returns true if the repo already is a repository (and is root of repo)
bool isRepo(gyoa::model::ActiveModel*);

//! initializes git repository in repo_dir
void open(gyoa::model::ActiveModel*);

//! initializes empty git repository in repo_dir
void init(gyoa::model::ActiveModel*);

//! erases git directory
void obliterate(gyoa::model::ActiveModel*);

//! clones git repo from upstream url to repo_dir
void clone(gyoa::model::ActiveModel*,context::context_t&);

//! commits all saved edits with the given commit message.
void stageAndCommit(gyoa::model::ActiveModel*,context::context_t&,std::string message);

//! fetches changes, but does not update world.
//! returns false if error, true on success
//! credentials used to authenticate connection
bool fetch(gyoa::model::ActiveModel*,context::context_t&, push_cred credentials);

//! fetches changes, but does not update world.
//! returns false if error, true on success
bool fetch(gyoa::model::ActiveModel*,context::context_t&);

//! returns true if common history exists with most recently-fetched branch
bool commonHistoryExists(gyoa::model::ActiveModel*,context::context_t&);

//! Merges in fetched changes (from fetch()).
//! updates data model in memory, so Operation.reload() is not necessary.
//! returns pair (bool error?, list of conflicts for user to resolve,
//! or empty list if mergy style is not MANUAL)
MergeResult merge(gyoa::model::ActiveModel*,merge_style,context::context_t&);

//! pushes commits to origin, returns true if successful
//! push_kill_callback is a function that returns 1 to cancel the push.
//! interrupt: completed_callback() called when git disconnects from server,
//! success is true if succesfully pushed.
//! varg: supplied to callbacks
bool push(gyoa::model::ActiveModel*,context::context_t&,push_cred credentials,bool (*push_kill_callback)(void*)=[](void*){return false;},void completed_callback(bool success,void*)=[](bool,void*){return;},void* varg=nullptr);

//=====================INTERNAL METHODS=====================//

/** pushes directly; provides no means for interrupt
 *  INTERNAL*/
bool push_direct(gyoa::model::ActiveModel*,context::context_t& context, push_cred credentials,bool* kill);

//! tree for current revisions. Similar to git add --all.
//! INTERNAL
git_tree * setStaged(gyoa::model::ActiveModel*,std::vector<std::string> paths);

//! retrieves head commit, or nullptr. Don't forget to free.
//! INTERNAL
git_commit * getHead(gyoa::model::ActiveModel*);

//! retrieves last fetched commit.
//! INTERNAL
git_commit* getFetchCommit(gyoa::model::ActiveModel*);

//! retrieves latest common ancestor between fetch and head
//! INTERNAL
git_commit* getCommon(gyoa::model::ActiveModel*,context::context_t&);

//! INTERNAL
git_tag* getTagFromName(gyoa::model::ActiveModel*,std::string name);

//! retrieves origin
//! INTERNAL
git_remote* getOrigin(gyoa::model::ActiveModel*,context::context_t&);

//! constructs a model matching the state of the world at the given commit
//! INTERNAL
model::world_t modelFromCommit(gyoa::model::ActiveModel*,git_commit*);

//! merges two strings
//! INTERNAL
void merge_string(std::string& result, std::string common, std::string remote, std::string local, merge_style,
		MergeResult& log, std::string error_description);

//! merges two id_types
//! INTERNAL
void merge_id(model::id_type& result, model::id_type common, model::id_type remote, model::id_type local,
		merge_style style, MergeResult& log, std::string error_description);

//! merges two bools
//! INTERNAL
void merge_bool(bool& result, bool common, bool remote, bool local, merge_style,
		MergeResult& log, std::string error_description);

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_GITOPS_H_ */
