/*
 * GitOps.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef BACKEND_OPS_GITOPS_H_
#define BACKEND_OPS_GITOPS_H_

#include <string>

#include "../model/World.h"
#include "../ops/Operation.h"

namespace gyoa {
namespace ops {

/**synchronizes using git*/
class GitOps {
public:
	GitOps();
	~GitOps();
	void setLocalRepoDirectory(std::string);
	void init(bool silence=true);
	void setUpstream(std::string upstream);
	void pull();
	void addAll();
	void commit(std::string message);
	void push();
private:
	std::string repo_dir;
};

/** pulls to a tmp folder and last-pull folder, allows merging*/
class GitOpsWithTmp {
	enum merge_style {
		USE_LOCAL,
		USE_REMOTE,
		MANUAL,
	};
public:
	GitOpsWithTmp(Operation* parent);
	~GitOpsWithTmp();
	void setLocalRepoDirectory(std::string);
	void setTmpPullDirectory(std::string);
	void init(bool silence=true);
	void setUpstream(std::string upstream);
	void pull();
	//returns pair (bool error?, string error message)
	std::pair<bool,std::string> merge(merge_style);
	void addAll();
	void commit(std::string message);
	void push();
private:
	//! merges two strings
	std::string merge_string(std::string common, std::string remote, std::string local, merge_style,
			bool& error, std::string& msg) const;
	//! merges two id_types
	model::id_type merge_id(model::id_type common, model::id_type remote, model::id_type local,
			merge_style merge_style, bool& error, std::string& msg) const;
	//! merges two bools
	bool merge_bool(bool common, bool remote, bool local, merge_style, bool& error, std::string& error_msg) const;
private:
	//data Operation required for merging
	Operation* parent;
	Operation tmp_ops;
	Operation last_pull_ops;
	model::world_t tmp_model;
	model::world_t last_pull_model;
	GitOps data;
	GitOps tmp;
	GitOps last_pull;
};

} /* namespace ops */
} /* namespace gyoa */

#endif /* BACKEND_OPS_GITOPS_H_ */
