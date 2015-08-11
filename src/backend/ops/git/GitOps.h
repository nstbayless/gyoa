/*
 * GitOps.h
 *
 *  Created on: Aug 10, 2015
 *      Author: n
 */

#ifndef BACKEND_OPS_GITOPS_H_
#define BACKEND_OPS_GITOPS_H_

#include <string>

namespace cyoa {
namespace ops {

/**synchronizes using git*/
class GitOps {
public:
	GitOps();
	virtual ~GitOps();
	void setLocalRepoDirectory(std::string);
	void init();
	void setUpstream(std::string upstream);
	void pull();
	void addAll();
	void commit(std::string message);
	void push();
private:
	std::string repo_dir;
};

} /* namespace ops */
} /* namespace cyoa */

#endif /* BACKEND_OPS_GITOPS_H_ */
