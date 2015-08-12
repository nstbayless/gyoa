/*
 * GitOpsTmp.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: n
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../id_parse.h"
#include "../model/Room.h"
#include "GitOps.h"

namespace gyoa {
namespace ops {

std::string TMP_PULL_DIR="/tmp/gyoa_pull_tmp/";

GitOpsWithTmp::GitOpsWithTmp(Operation* parent) : tmp_remote_ops(false), common_history_ops(false){
	this->parent=parent;
	tmp_remote.setLocalRepoDirectory(TMP_PULL_DIR);
	tmp_remote_ops.setDataDirectory(TMP_PULL_DIR);
}

GitOpsWithTmp::~GitOpsWithTmp() {
}

void GitOpsWithTmp::setLocalRepoDirectory(std::string dir) {
	local_data.setLocalRepoDirectory(dir);
	common_history_ops.setDataDirectory(dir+".pull_common/");
	common_history.setLocalRepoDirectory(dir+".pull_common/");
}

void GitOpsWithTmp::setTmpPullDirectory(std::string dir) {
	tmp_remote.setLocalRepoDirectory(dir);
	tmp_remote_ops.setDataDirectory(dir);
}

void GitOpsWithTmp::init(bool silence) {
	local_data.init(silence);
	common_history.init(true);

	system(std::string("cd " + local_data.getLocalRepoDirectory() + "; echo .pull_common/* > .gitignore").c_str());
}

void GitOpsWithTmp::setUpstream(std::string upstream) {
	local_data.setUpstream(upstream);
	common_history.setUpstream(upstream);
}

void GitOpsWithTmp::fetch() {
	tmp_remote.init();
	tmp_remote.setUpstream(local_data.getUpstream());
	tmp_remote.pull();
	tmp_remote_ops.setModel(tmp_remote_model);
	tmp_remote_ops.loadAll();
	system(std::string("rm -rf " + TMP_PULL_DIR).c_str());
	//pulls files to repo but does not modify files in memory
}

void GitOpsWithTmp::addAll() {
	local_data.addAll();
}

void GitOpsWithTmp::commit(std::string message) {
	local_data.commit(message);
}

void GitOpsWithTmp::push() {
	local_data.push();
	common_history.pull();
}

std::pair<bool,std::vector<MergeConflict>> GitOpsWithTmp::merge(merge_style style) {
	local_data.pull();
	common_history_ops.setModel(common_history_model=model::world_t());
	common_history_ops.loadAll();
	bool err=false;
	std::vector<MergeConflict> merge_list;

	model::world_t& common=common_history_model;
	model::world_t& remote=tmp_remote_model;
	model::world_t& local=*parent->model;

	//1. merge tmp (remote) model into parent (local) model
	//2. save local model
	//3. add/commit this model
	//4. pull to last_pull

	//first, merge world:
	merge_string(local.title, common.title,remote.title,local.title,style,err,merge_list, "World title");

	merge_id(local.first_room,common.first_room,remote.first_room,local.first_room,style,err,merge_list,"ID of opening scenario");

	//set world next_gid to the larger of the two:
	local.next_rm_gid=std::max(local.next_rm_gid,remote.next_rm_gid);

	//now merge rooms:
	//only need to merge rooms that were in common; otherwise, they're new to both forks.
	//them add options just in remote
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
		merge_string(rm_local.title,rm_common.title,rm_remote.title,rm_local.title,style,err,merge_list,"Title of scenario id " + write_id(rm_id));

		//merge room body
		merge_string(rm_local.body,rm_common.body,rm_remote.body,rm_local.body,style,err,merge_list,"Body text for scenario id " + write_id(rm_id));

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
			merge_string(opt_local.option_text,opt_common.option_text,
					opt_remote.option_text,opt_local.option_text,style,err,merge_list,"Option text for scenario id " + write_id(rm_id));

			//merge option destination:
			merge_id(opt_local.dst,opt_common.dst,opt_remote.dst,opt_local.dst,style,err,merge_list,
					"Destination scenario id from an option in scenario id " + write_id(rm_id) + ", titled \"" + opt_local.option_text + "\" locally, \""
					+ opt_remote.option_text + "\" on remote, and previously titled \"" + opt_common.option_text + "\"");
		}

		//add options unique to remote
		for (auto iter_opt : rm_remote.options)
				if (!rm_common.options.count(iter_opt.first))
					rm_local.options[iter_opt.first]=iter_opt.second;

		//merge room dead-end flag (set to false if options remaining)
		if (rm_local.options.size())
			rm_local.dead_end=false;
		else
			merge_bool(rm_local.dead_end,rm_common.dead_end,rm_remote.dead_end,rm_local.dead_end,
					style,err,merge_list,"dead-end flag for scenario id " + write_id(rm_id));
	}
	for (auto iter_rm : remote.rooms)
		if (!common.rooms.count(iter_rm.first))
			local.rooms[iter_rm.first]=iter_rm.second;

	if (style==TRIAL_RUN)
		return {err,merge_list};

	//save changes
	parent->saveAll(true);

	//add & commit
	local_data.addAll();
	local_data.commit("merge result " + std::string(((err)?"(successful)":"(conflict. needs manual review)")));

	//update last_pull.
	common_history.pull();
	return {err,merge_list};
}

void GitOpsWithTmp::merge_string(std::string& result, std::string common, std::string remote, std::string local, merge_style style,
			bool& error, std::vector<MergeConflict>& merge_list, std::string error_description) {
	if (style==FORCE_LOCAL) {
		result = local;
		return;
	}
	if (style==FORCE_REMOTE) {
		result = remote;
		return;
	}
	//local and remote changed, but both the same:
	if (!remote.compare(local))
		result = local;
	//local and remote changed, both different
	if (common.compare(remote) && common.compare(local)) {
		switch (style) {
		case USE_LOCAL:
			result = local;
			return;
		case USE_REMOTE:
			result = remote;
			return;
		case TRIAL_RUN:
			error=true;
			result = local;
			return;
		case MANUAL:
			MergeConflict mc;
			mc.data_type=MergeConflict::STRING;
			mc.description=error_description;
			mc.description+="\noriginal version: "+common;
			mc.description+="\nlocal revision: "+local;
			mc.description+="\nremote revision: "+remote;
			mc.common=common;
			mc.remote=remote;
			mc.local=local;
			mc.data_ptr=&result;
			merge_list.push_back(mc);
			return;
		}
	}
	//only remote changed:
	if (common.compare(remote)) {
		result = remote;
		return;
	}
	//only local changed:
	if (common.compare(local)) {
		result = local;
		return;
	}
	//neither changed:
	assert(!common.compare(local)&&!remote.compare(local));
	result = common;
}

void GitOpsWithTmp::merge_id(model::id_type& result, model::id_type common, model::id_type remote, model::id_type local,
		merge_style style, bool& error, std::vector<MergeConflict>& merge_list, std::string error_description) {
	if (style==FORCE_LOCAL) {
		result=local;
		return;
	}
	if (style==FORCE_REMOTE) {
		result=remote;
		return;
	}
	//local and remote changed, but both the same:
	if (remote==local)
		result=local;
	//local and remote changed, both different
	if (common!=remote && common!= local) {
		switch (style) {
		case USE_LOCAL:
			result = local;
			return;
		case USE_REMOTE:
			result = remote;
			return;
		case TRIAL_RUN:
			error = true;
			result = local;
			return;
		case MANUAL:
			//todo: error, can't return manual result.
			error = true;
			MergeConflict mc;
			mc.data_type=MergeConflict::ID;
			mc.description=error_description;
			mc.description+="\noriginal version: "+write_id(common);
			mc.description+="\nlocal revision: "+write_id(local);
			mc.description+="\nremote revision: "+write_id(remote);
			mc.common=write_id(common);
			mc.remote=write_id(remote);
			mc.local=write_id(local);
			mc.data_ptr=&result;
			merge_list.push_back(mc);
			result= remote;
		}
	}
	//only remote changed:
	if (common!=remote) {
		result= remote;
		return;
	}
	//only local changed:
	if (common!=local) {
		result= local;
		return;
	}
	//neither changed:
	assert(common==local&&remote==local);
	result= common;
}

void GitOpsWithTmp::merge_bool(bool& result, bool common, bool remote, bool local, merge_style style, bool& error,
			std::vector<MergeConflict>& merge_list, std::string error_description) {
	if (style == FORCE_LOCAL) {
		result = local;
		return;
	}
	if (style == FORCE_REMOTE) {
		result = remote;
		return;
	}
	//local and remote changed, but both the same:
	if (remote == local)
		result = local;
	//local and remote changed, both different
	if (common != remote && common != local) {
		switch (style) {
		case USE_LOCAL:
			result = local;
			return;
		case USE_REMOTE:
			result = remote;
			return;
		case TRIAL_RUN:
			error = true;
			result = local;
			return;
		case MANUAL:
			//todo: error, can't return manual result.
			error = true;
			MergeConflict mc;
			mc.data_type = MergeConflict::BOOL;
			mc.description=error_description;
			mc.description+=std::string("\noriginal version: "+std::string((common)?"True":"False"));
			mc.description+=std::string("\nlocal revision: "+std::string((local)?"True":"False"));
			mc.description+=std::string("\nremote revision: "+std::string((remote)?"True":"False"));
			mc.common='0'+common;
			mc.remote='0'+remote;
			mc.local='0'+local;
			mc.data_ptr = &result;
			merge_list.push_back(mc);
			result = remote;
		}
	}
	//only remote changed:
	if (common != remote) {
		result = remote;
		return;
	}
	//only local changed:
	if (common != local) {
		result = local;
		return;
	}
	//neither changed:
	assert(common == local && remote == local);
	result = common;
}

bool GitOpsWithTmp::commonHistoryExists() {
	return common_history.commitCount()>0;
}

} /* namespace ops */
} /* namespace gyoa */
