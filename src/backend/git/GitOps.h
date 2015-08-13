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

#include "../model/World.h"
#include "../ops/Operation.h"

namespace gyoa {
namespace ops {

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

	//! initializes the git repository if it has not yet been initialized already.
	//! Also sets git username and email to default values
	void init(bool silence=true);

	//! Sets the upstream (origin) repository.
	void setUpstream(std::string upstream);

	//! Retrieves the URL for the upstream (origin) repository.
	std::string getUpstream();

	//! Retrieves number of commits in history, zero if no commits have been made.
	int commitCount();

	//! Pulls from origin, merging in the normal git way.
	void pull();

	//! stages all edits.
	void addAll();

	//! commits all staged edits with the given commit message.
	void commit(std::string message);

	//! pushes commits to origin.
	void push();
private:
	std::string repo_dir;
};

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

/** pulls to a tmp folder and last-pull folder, allows merging*/
class GitOpsWithTmp {
public:
	GitOpsWithTmp(Operation* parent);
	~GitOpsWithTmp();

	//! sets directory to be used for git repo, and automatically sets directory
	//! for which common history will be stored as a subdir of the given directory.
	//! ("common history" refers to the last commit shared by the local branch and
	//! the remote branch.)
	void setLocalRepoDirectory(std::string);

	//! sets directory to which remote branch will be pulled to.
	//! A default directory in /tmp/ will be used if this is not invoked.
	void setTmpPullDirectory(std::string);

	//! returns true if common history has any commits, meaning a pull has previously occurred
	bool commonHistoryExists();

	//! initializes git repo and common history repo.
	void init(bool silence=true);

	//! sets origin url
	void setUpstream(std::string upstream);

	//! Retrieves the URL for the upstream (origin) repository.
	std::string getUpstream();

	//! retrieves remote branch, does not modify existing local repo directory.
	void fetch();

	//! Merges in fetched changes (from fetch()).
	//! updates data model in memory, so Operation.reload() is not necessary.
	//! returns pair (bool error?, list of conflicts for user to resolve,
	//! or empty list if mergy style is not MANUAL)
	std::pair<bool,std::vector<MergeConflict>> merge(merge_style);

	//! stages all changes in local repo directory
	void addAll();

	//! commits all changes in local repo directory with given message
	void commit(std::string message);

	//! pushes all commits from local repo.
	void push();
private:
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
	//data Operation required for merging
	Operation* parent;
	Operation tmp_remote_ops;
	Operation common_history_ops;
	model::world_t tmp_remote_model;
	model::world_t common_history_model;
	GitOps local_data;
	GitOps tmp_remote;
	GitOps common_history;
};

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_GITOPS_H_ */
