/*
 * GitOpsTmp.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: n
 */

#include "GitOps.h"
#include <cassert>

namespace gyoa {
namespace ops {

GitOpsWithTmp::GitOpsWithTmp(Operation* parent) : tmp_ops(false), last_pull_ops(false){
	this->parent=parent;
	tmp.setLocalRepoDirectory("/tmp/gyoa_pull_tmp/");
}

GitOpsWithTmp::~GitOpsWithTmp() {
}

void GitOpsWithTmp::setLocalRepoDirectory(std::string dir) {
	data.setLocalRepoDirectory(dir);
	last_pull.setLocalRepoDirectory(dir+"/.pull_common");
}

void GitOpsWithTmp::setTmpPullDirectory(std::string dir) {
	tmp.setLocalRepoDirectory(dir);
	tmp_ops.setDataDirectory(dir);
}

void GitOpsWithTmp::init(bool silence) {
	data.init(silence);
	last_pull.init(true);
	tmp.init(true);
}

void GitOpsWithTmp::setUpstream(std::string upstream) {
	data.setUpstream(upstream);
	last_pull.setUpstream(upstream);
	tmp.setUpstream(upstream);
}

void GitOpsWithTmp::pull() {
	tmp.pull();
	tmp_ops.setModel(tmp_model);
	tmp_ops.loadAll();
}

void GitOpsWithTmp::addAll() {
	data.addAll();
}

void GitOpsWithTmp::commit(std::string message) {
	data.commit(message);
}

void GitOpsWithTmp::push() {
	data.push();
	last_pull.pull();
}

const std::string CONFLICT_START = "CONFLICT->>";
const std::string CONFLICT_SEPARATOR = "|<-remote : local->|";
const std::string CONFLICT_END = "<<-CONFLICT";

std::pair<bool,std::string> GitOpsWithTmp::merge(merge_style style) {
	last_pull_ops.setModel(last_pull_model);
	last_pull_ops.loadAll();
	bool err=false;
	std::string msg="";

	model::world_t& common=last_pull_model;
	model::world_t& remote=tmp_model;
	model::world_t& local=*parent->model;

	//1. merge tmp (remote) model into parent (local) model
	//2. save local model
	//3. add/commit this model
	//4. pull to last_pull

	//first, merge world:
	local.title=merge_string(common.title,remote.title,local.title,style,err,msg);

	local.first_room=merge_id(common.first_room,remote.first_room,local.first_room,style,err,msg);

	//set world next_gid to the larger of the two:
	local.next_rm_gid=std::max(local.next_rm_gid,remote.next_rm_gid);

	//now merge rooms:
	//only need to merge rooms that were in common; otherwise, they're new to both forks.
	for (auto iter_rm : common.rooms) {
		auto rm_id = iter_rm.first;

		//if room deleted in either fork, use non-deleted version.
		if (!remote.rooms.count(rm_id))
			continue;
		if (!local.rooms.count(rm_id)) {
			local.rooms[rm_id]=remote.rooms[rm_id];
			continue;
		}

		model::room_t& rm_common=iter_rm.second;//==common.rooms[rm_id]
		model::room_t& rm_remote=remote.rooms[rm_id];
		model::room_t& rm_local = local.rooms[rm_id];

		//merge room title
		rm_local.title=merge_string(rm_common.title,rm_remote.title,rm_local.title,style,err,msg);

		//merge room body
		rm_local.body=merge_string(rm_common.body,rm_remote.body,rm_local.body,style,err,msg);

		//merge room options
		//only need to merge options that were in rm_common; otherwise, they're new to both forks.
		for (auto iter_opt : rm_common.options) {
			auto opt_id=iter_opt.first;

			//if option deleted in either fork, use non-deleted version
			if (!rm_remote.options.count(opt_id))
				continue;
			if (!rm_local.options.count(opt_id)) {
				rm_local.options[opt_id]=rm_remote.options[opt_id];
				continue;
			}

			model::option_t& opt_common=iter_opt.second;//==rm_common.options[opt_id]
			model::option_t& opt_remote=rm_remote.options[opt_id];
			model::option_t& opt_local = rm_local.options[opt_id];

			//merge option text:
			opt_local.option_text=merge_string(opt_common.option_text,opt_remote.option_text,opt_local.option_text,style,err,msg);

			//merge option destination:
			opt_local.dst=merge_id(opt_common.dst,opt_remote.dst,opt_local.dst,style,err,msg);
		}

		//merge room dead-end flag (set to false if options remaining)
		if (rm_local.options.size())
			rm_local.dead_end=false;
		else
			rm_local.dead_end=merge_bool(rm_common.dead_end,rm_remote.dead_end,rm_local.dead_end,style,err,msg);
	}

	//save changes
	parent->saveAll();

	//add & commit
	data.addAll();
	data.commit("merge result " + std::string(((err)?"(successful)":"(conflict. needs manual review)")));

	//update last_pull.
	last_pull.pull();
	return std::pair<bool,std::string>(err,msg);
}

std::string GitOpsWithTmp::merge_string(std::string common, std::string remote, std::string local,
		merge_style style, bool& error, std::string& msg) const {
	//local and remote changed, but both the same:
		if (!remote.compare(local))
			return local;
		//local and remote changed, both different
		if (common.compare(remote) && common.compare(local)) {
			switch (style) {
			case USE_LOCAL:
				return local;
			case USE_REMOTE:
				return remote;
			case MANUAL:
				//todo: error, can't return manual result.
				error=true;
				msg+="Error merging text.";
				msg+='\n';
				return remote;
			}
		}
		//only remote changed:
		if (common.compare(remote))
			return remote;
		//only local changed:
		if (common.compare(local))
			return local;
		//neither changed:
		assert(!common.compare(local)&&!remote.compare(local));
		return common;
}

model::id_type GitOpsWithTmp::merge_id(model::id_type common, model::id_type remote, model::id_type local,
		merge_style style, bool& error, std::string& msg) const {
	//local and remote changed, but both the same:
	if (remote==local)
		return local;
	//local and remote changed, both different
	if (common!=remote && common!= local) {
		switch (style) {
		case USE_LOCAL:
			return local;
		case USE_REMOTE:
			return remote;
		case MANUAL:
			//todo: error, can't return manual result.
			error=true;
			msg+="Error merging id.";
			msg+='\n';
			return remote;
		}
	}
	//only remote changed:
	if (common!=remote)
		return remote;
	//only local changed:
	if (common!=local)
		return local;
	//neither changed:
	assert(common==local&&remote==local);
	return common;
}

bool GitOpsWithTmp::merge_bool(bool common, bool remote, bool local,
		merge_style style, bool& error, std::string& msg) const {
	//local and remote changed, but both the same:
	if (remote==local)
		return local;
	//local and remote changed, both different
	if (common!=remote && common!= local) {
		switch (style) {
		case USE_LOCAL:
			return local;
		case USE_REMOTE:
			return remote;
		case MANUAL:
			//todo: error, can't return manual result.
			error=true;
			msg+="Error merging bool.";
			msg+='\n';
			return remote;
		}
	}
	//only remote changed:
	if (common!=remote)
		return remote;
	//only local changed:
	if (common!=local)
		return local;
	//neither changed:
	assert(common==local&&remote==local);
	return common;
}

} /* namespace ops */
} /* namespace gyoa */
