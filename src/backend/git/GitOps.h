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
namespace ops {

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

/**synchronizes using git.
 * Presently invokes git via system calls; this shall be replaced with a library in the future.*/
class GitOps {
public:
	GitOps();
	~GitOps();

	//! Sets the directory which will be used to store the git repository.
	//! This should not be invoked after init has been called
	void setLocalRepoDirectory(std::string);

	//! Retrieves directory in which local git repo is stored.
	std::string getLocalRepoDirectory();

	//! sets the fetch/pull origin
	void setOrigin(context::context_t context);

	//! returns true if repo_dir already is a repository (and is root of repo)
	bool isRepo();

	//! initializes git repository in repo_dir
	void open();

	//! initializes empty git repository in repo_dir
	void init();

	//! deletes the git repository, including files
	void clear();

	//! clones git repo from upstream url to repo_dir
	void clone(context::context_t&);

	//! commits all staged edits with the given commit message.
	void commit(context::context_t&,std::string message);

	//! fetches changes, but does not update world.
	//! returns false if error, true on success
	//! credentials used to authenticate connection
	bool fetch(context::context_t&, push_cred credentials);

	//! fetches changes, but does not update world.
	//! returns false if error, true on success
	bool fetch(context::context_t&);

	//! returns true if common history exists with most recently-fetched branch
	bool commonHistoryExists(context::context_t&);

	//! Merges in fetched changes (from fetch()).
	//! updates data model in memory, so Operation.reload() is not necessary.
	//! returns pair (bool error?, list of conflicts for user to resolve,
	//! or empty list if mergy style is not MANUAL)
	MergeResult merge(merge_style,ops::Operation& ops,context::context_t&);

	//! pushes commits to origin, returns true if successful
	//! push_kill_callback is a function that returns 1 to cancel the push.
	//! interrupt: completed_callback() called when git disconnects from server,
	//! success is true if succesfully pushed.
	//! varg: supplied to callbacks
	bool push(context::context_t&,push_cred credentials,bool (*push_kill_callback)(void*)=[](void*){return false;},void completed_callback(bool success,void*)=[](bool,void*){return;},void* varg=nullptr);
private:
	/** pushes directly; provides no means for interrupt*/
	bool push_direct(context::context_t& context, push_cred credentials,bool* kill);
private:
	//! tree for current revisions. Similar to git add --all.
	git_tree * setStaged(std::vector<std::string> paths);

	//! retrieves head commit, or nullptr. Don't forget to free.
	git_commit * getHead();

	//! retrieves last fetched commit.
	git_commit* getFetchCommit();

	//! retrieves latest common ancestor between fetch and head
	git_commit* getCommon(context::context_t&);

	git_tag* getTagFromName(std::string name);

	//! retrieves origin
	git_remote* getOrigin(context::context_t&);

	//! constructs a model matching the state of the world at the given commit
	model::world_t modelFromCommit(git_commit*);

	//! merges two strings
	void merge_string(std::string& result, std::string common, std::string remote, std::string local, merge_style,
			MergeResult& log, std::string error_description);

	//! merges two id_types
	void merge_id(model::id_type& result, model::id_type common, model::id_type remote, model::id_type local,
			merge_style style, MergeResult& log, std::string error_description);

	//! merges two bools
	void merge_bool(bool& result, bool common, bool remote, bool local, merge_style,
			MergeResult& log, std::string error_description);
private:
	std::string repo_dir;
	git_repository* repo=nullptr;
};

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_GITOPS_H_ */
