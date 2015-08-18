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

struct MergeConflict {
	enum {
		STRING,
		ID,
		BOOL,
	} data_type;
	std::string description="";
	std::string common;
	std::string remote;
	std::string local;
	void* data_ptr;
};

/**synchronizes using git.
 * Presently invokes git via system calls; this shall be replaced with a library in the future.*/
class GitOps {
public:
	GitOps();
	~GitOps();

	//! Sets the directory which will be used to store the git repository.
	//! This should not be invoked after any the repository has already been initialized.
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

	//! clones git repo from upstream url to repo_dir
	void clone(context::context_t&);

	//! commits all staged edits with the given commit message.
	void commit(context::context_t&,std::string message);

	void fetch(context::context_t&);

	//! returns true if common history exists with most recently-fetched branch
	bool commonHistoryExists();

	//! Merges in fetched changes (from fetch()).
	//! updates data model in memory, so Operation.reload() is not necessary.
	//! returns pair (bool error?, list of conflicts for user to resolve,
	//! or empty list if mergy style is not MANUAL)
	std::pair<bool,std::vector<MergeConflict>> merge(merge_style,ops::Operation& ops,context::context_t&);

	//! pushes commits to origin.
	void push(context::context_t&);
private:
	//! tree for current revisions. Similar to git add --all.
	git_tree * setStaged(std::vector<std::string> paths);

	//! retrieves head commit, or nullptr. Don't forget to free.
	git_commit * getHead();

	//! retrieves last fetched commit.
	git_commit* getFetchCommit();

	//! retrieves latest common ancestor between fetch and head
	git_commit* getCommon();

	//! retrieves origin
	git_remote * getOrigin(context::context_t&);

	//! constructs a model matching the state of the world at the given commit
	model::world_t modelFromCommit(git_commit*);

	//! merges two strings
	void merge_string(std::string& result, std::string common, std::string remote, std::string local, merge_style,
			bool& error, std::vector<MergeConflict>& merge_list, std::string error_description);
	//! merges two id_types
	void merge_id(model::id_type& result, model::id_type common, model::id_type remote, model::id_type local,
			merge_style merge_style, bool& error, std::vector<MergeConflict>& merge_list, std::string error_description);
	//! merges two bools
	void merge_bool(bool& result, bool common, bool remote, bool local, merge_style, bool& error,
			std::vector<MergeConflict>& merge_list, std::string error_description);
private:
	std::string repo_dir;
	git_repository* repo=nullptr;
};

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_GITOPS_H_ */
