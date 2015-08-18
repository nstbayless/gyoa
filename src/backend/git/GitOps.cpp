/*
 * GitOps.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: n
 */

#include "GitOps.h"

#include <git2.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>

#include "../context/Context.h"
#include "../error.h"
#include "../id_parse.h"
#include "../model/Room.h"
#include "../model/World.h"
#include "../ops/FileIO.h"
#include "../ops/Operation.h"

namespace gyoa {
namespace ops {

GitOps::GitOps() {
	git_libgit2_init();
}

GitOps::~GitOps() {
	git_libgit2_shutdown();
	if (repo)
		git_repository_free(repo);
}

void GitOps::setLocalRepoDirectory(std::string dir) {
	repo_dir=dir;
}

std::string GitOps::getLocalRepoDirectory() {
	return repo_dir;
}

bool GitOps::isRepo() {
	return !git_repository_open_ext(NULL, repo_dir.c_str(),
				GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr);
}

void GitOps::open() {
	if (git_repository_open_ext(&repo, repo_dir.c_str(),
			GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr))
		throw GitNotRepo(repo_dir);
}

void GitOps::init() {
	if (git_repository_init(&repo, repo_dir.c_str(), false))
		throw GitInitFail(repo_dir);
}

void GitOps::clone(context::context_t& context) {
	if (git_clone(&repo,context.upstream_url.c_str(),repo_dir.c_str(),nullptr));
		throw GitCloneFail(repo_dir,context.upstream_url);
}


void GitOps::commit(context::context_t& context,std::string message) {
	git_signature* sig=nullptr;
	git_signature_now(&sig, context.user_name.c_str(), context.user_email.c_str());

	git_commit* head = getHead();

	const git_commit* parents[] = {head};

	std::vector<std::string> paths;
	//todo: add paths

	git_commit_create(
	  nullptr,
	  repo,
	  "HEAD",						/* name of ref to update */
	  sig,							/* author */
	  sig,							/* committer */
	  "UTF-8",						/* message encoding */
	  message.c_str(),				/* message */
	  setStaged(paths),				/* root tree */
	  2,							/* parent count */
	  parents);						/* parents */

	if (head)
		git_commit_free(head);
	git_signature_free(sig);
}

void GitOps::fetch(context::context_t& context) {
	git_remote* origin = getOrigin(context);
	git_remote_fetch(origin,
		NULL, /* refspecs, NULL to use the configured ones */
		NULL, /* options, empty for defaults */
		NULL); /* reflog mesage, usually "fetch" or "pull", you can leave it NULL for "fetch" */
}

struct walk_data {
	model::world_t& world;
	git_repository* repo;
};

std::pair<bool, std::vector<MergeConflict> > GitOps::merge(
		merge_style style, ops::Operation& ops,context::context_t& context) {
	git_commit* remote_commit = GitOps::getFetchCommit();
	git_commit* common_commit = GitOps::getCommon();

	if (style!=TRIAL_RUN) {
		assert(!ops.savePending());
		ops.loadAll();
	}

	bool err=false;
	std::vector<MergeConflict> merge_list;

	model::world_t common=modelFromCommit(common_commit);
	model::world_t remote=modelFromCommit(remote_commit);
	model::world_t& local=*ops.model;

	//1. merge tmp (remote) model into parent (local) model
	//2. save local model
	//3. add/commit this model
	//4. pull to last_pull

	//first, merge world:
	merge_string(local.title, common.title,remote.title,local.title,style,err,merge_list, "World title");

	merge_id(local.first_room,common.first_room,remote.first_room,local.first_room,style,err,merge_list,"ID of opening scenario");

	//set world next_gid to the larger of the two:
	if (style!=TRIAL_RUN)
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

	if (style==TRIAL_RUN) {
		if (remote_commit)
			git_commit_free(remote_commit);
		if (common_commit)
			git_commit_free(common_commit);
		return {err,merge_list};
	}

	//save changes
	ops.saveAll(true);

	//commit
	git_signature* sig=nullptr;
	git_signature_now(&sig, context.user_name.c_str(), context.user_email.c_str());

	git_commit* head = getHead();

	const git_commit* parents[] = {head,remote_commit};

	std::vector<std::string> paths;
	//todo: add paths

	git_commit_create(
	  nullptr,
	  repo,
	  "HEAD",						/* name of ref to update */
	  sig,							/* author */
	  sig,							/* committer */
	  "UTF-8",						/* message encoding */
	  "merge remote",				/* message */
	  setStaged(paths),				/* root tree */
	  2,							/* parent count */
	  parents);						/* parents */

	if (head)
		git_commit_free(head);
	git_signature_free(sig);

	//update last_pull.

	if (remote_commit)
		git_commit_free(remote_commit);
	if (common_commit)
		git_commit_free(common_commit);

	return {err,merge_list};
}

int walk_cb(const char *root, const git_tree_entry *entry, void *dv) {
	walk_data &d = *(walk_data*) dv;
	git_object* obj=nullptr;
	git_blob* blob=nullptr;
	git_buf buf = {nullptr,0,0};
	git_tree_entry_to_object(&obj,d.repo,entry);
	std::string filename = git_tree_entry_name(entry);
	git_blob_lookup(&blob,d.repo,git_object_id(obj));
	git_blob_filtered_content(&buf,blob,filename.c_str(),true);
	git_object_free(obj);
	buf.ptr;


	//the name of the git file

	std::string output;
	for (int i=0;i<buf.size;i++)
		output+=*((char*)(buf.ptr)+i);
	std::cout<<output;

	FileIO io;

	if (!filename.substr(0,3).compare("rm_")) {
		int ext_loc = filename.find(".txt");
		assert(ext_loc!=std::string::npos);
		std::string id_str = filename.substr(3,ext_loc);
		model::id_type id = parse_id(id_str);
		model::room_t rm = io.loadRoomFromText(output);
		d.world.rooms[id]=rm;
	} else if (!filename.compare("world.txt")) {
		model::world_t w = io.loadWorldFromText(output);
		w.rooms=d.world.rooms;
		d.world=w;
	}

	git_buf_free(&buf);
	return 0;
}

model::world_t GitOps::modelFromCommit(git_commit*) {
	git_commit * fetched = getFetchCommit();
	git_tree* tree;
	git_commit_tree(&tree, fetched);
	model::world_t world;
	walk_data d = {world,repo};
	git_tree_walk(tree, GIT_TREEWALK_PRE, &walk_cb, &d);
	return world;
}

void GitOps::push(context::context_t& context) {
	git_remote_push(getOrigin(context),nullptr,nullptr);
}

const git_tree* GitOps::setStaged(std::vector<std::string> paths) {
	git_treebuilder* bld = nullptr;
	git_tree* src;
	git_commit* head = getHead();
	git_commit_tree(&src, head);
	git_treebuilder_new(&bld, repo, src);
	if (head)
		git_commit_free(head);

	/* Add some entries */
	git_object *obj = NULL;
	for (auto path : paths) {
		git_revparse_single(&obj, repo, std::string("HEAD:"+path).c_str());
		git_treebuilder_insert(NULL, bld,
			   path.c_str(),        /* filename */
			   git_object_id(obj), /* OID */
			   GIT_FILEMODE_BLOB); /* mode */
		git_object_free(obj);
	}

	git_oid oid;
	git_treebuilder_write(&oid, bld);
	git_treebuilder_free(bld);
	return src;
}

git_commit* GitOps::getHead() {
	git_commit* head = nullptr;

	git_reference * ref = nullptr;
	if (!git_reference_lookup(&ref, repo, "refs/heads/master")) {
		const git_oid * parent_oid = git_reference_target(ref);
		git_commit_lookup(&head, repo, parent_oid);
	}
	git_reference_free(ref);
	return head;
}

git_commit* GitOps::getFetchCommit() {
	git_commit* head = nullptr;

	git_reference * ref = nullptr;
	if (!git_reference_dwim(&ref, repo, "origin/master")) {
		const git_oid * parent_oid = git_reference_target(ref);
		git_commit_lookup(&head, repo, parent_oid);
	}
	git_reference_free(ref);
	return head;
}

git_commit* GitOps::getCommon() {
	git_commit* const local = getHead();
	git_commit* const remote = getFetchCommit();

	git_commit* to_return=nullptr;

	git_commit* local_c=local;
	git_commit* remote_c=remote;

	std::vector<git_commit*> local_ancestors;
	std::vector<git_commit*> remote_ancestors;

	//construct list of ancestors (from first parent)
	//todo: check all parents
	while (local_c != nullptr)
		if (git_commit_parentcount(local_c)) {
			if (!git_commit_parent(&local_c, local_c, 0))
				local_ancestors.push_back(local_c);
		} else
			break;

	while (remote_c != nullptr)
		if (git_commit_parentcount(remote_c)) {
			if (!git_commit_parent(&remote_c, remote_c, 0))
				remote_ancestors.push_back(remote_c);
		} else
			break;

	//find first difference in list

	auto iter_loc = local_ancestors.rbegin();
	auto iter_rem = remote_ancestors.rbegin();

	if (iter_loc!=iter_rem)
		//no common history
		return nullptr;

	while (true){
		if (iter_loc==local_ancestors.rend())
			to_return = *iter_loc;
		if (iter_rem==remote_ancestors.rend())
			to_return = *iter_rem;
		if (*(iter_loc+1)!=*(iter_rem+1))
			to_return = *iter_loc; //==iter_rem
		iter_loc++;
		iter_rem++;

		if (to_return)
			break;
	}

	for (auto iter : local_ancestors)
		if (iter)
			git_commit_free(iter);

	for (auto iter : remote_ancestors)
		if (iter)
			git_commit_free(iter);

	return to_return;
}

git_remote* GitOps::getOrigin(context::context_t& context) {
	git_remote* origin = nullptr;
	git_remote_lookup(&origin, repo, "origin");
	if (origin) {
		if (context.upstream_url.compare(std::string(git_remote_url(origin))))
				git_remote_set_url(repo,"origin",context.upstream_url.c_str());
		return origin;
	}
	git_remote_create(&origin, repo, "origin",context.upstream_url.c_str());
	return origin;
}

void GitOps::merge_string(std::string& result, std::string common, std::string remote, std::string local, merge_style style,
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
	if (!remote.compare(local)) {
		result = local;
		return;
	}
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
		case FORCE_LOCAL:
		case FORCE_REMOTE:
			// Were already handled earlier in the function.
			assert(false);
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

void GitOps::merge_id(model::id_type& result, model::id_type common, model::id_type remote, model::id_type local,
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
	if (remote==local) {
		result=local;
		return;
	}
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
		case FORCE_LOCAL:
		case FORCE_REMOTE:
			// Were already handled earlier in the function.
			assert(false);
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

void GitOps::setOrigin(context::context_t context) {
	//method sets origin if not pre-existing
	getOrigin(context);
}

bool GitOps::commonHistoryExists() {
	return false;
}

void GitOps::merge_bool(bool& result, bool common, bool remote, bool local, merge_style style, bool& error,
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
	if (remote == local) {
		result = local;
		return;
	}
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
		case FORCE_LOCAL:
		case FORCE_REMOTE:
			// Were already handled earlier in the function.
			assert(false);
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

}}
