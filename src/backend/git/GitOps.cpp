/*
 * GitOps.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: n
 */

#include "GitOps.h"

#include "git2/blob.h"
#include "git2/clone.h"
#include "git2/commit.h"
#include "git2/object.h"
#include "git2/oid.h"
#include "git2/refs.h"
#include "git2/remote.h"
#include "git2/repository.h"
#include "git2/signature.h"
#include "git2/strarray.h"
#include "git2/tag.h"
#include "git2/transport.h"
#include "git2/tree.h"
#include "git2/types.h"
#include "git2.h"
#include "stddef.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <thread>

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
	if (repo)
		git_repository_free(repo);
	git_libgit2_shutdown();
}

struct walk_data {
	model::world_t& world;
	git_repository* repo;
};

int walk_cb(const char *root, const git_tree_entry *entry, void *dv) {
	walk_data &d = *(walk_data*) dv;
	git_object* obj=nullptr;
	git_blob* blob=nullptr;
	git_tree_entry_to_object(&obj,d.repo,entry);
	//filename associated with blob
	std::string filename = git_tree_entry_name(entry);
	if (git_blob_lookup(&blob,d.repo,git_object_id(obj))) {
		git_object_free(obj);
		git_blob_free(blob);
		return 0;
	}
	const void* content_ptr=git_blob_rawcontent(blob);
	git_object_free(obj);

	std::string output((const char*)content_ptr);

	if (!filename.substr(0,3).compare("rm_")) {
		int ext_loc = filename.find(".txt");
		assert(ext_loc!=std::string::npos);
		std::string id_str = filename.substr(3,ext_loc);
		model::id_type id = parse_id(id_str);
		model::room_t rm = FileIO::loadRoomFromText(output);
		d.world.rooms[id]=rm;
	} else if (!filename.compare("world.txt")) {
		model::world_t w = FileIO::loadWorldFromText(output);
		w.rooms=d.world.rooms;
		d.world=w;
	}

	//git_buf_free(&buf);
	git_blob_free(blob);
	return 0;
}

model::world_t GitOps::modelFromCommit(git_commit* com) {
	model::world_t world;
	if (!com)
		return world;
	git_tree* tree;
	git_commit_tree(&tree, com);
	walk_data d = {world,repo};
	git_tree_walk(tree, GIT_TREEWALK_PRE, &walk_cb, &d);
	return world;
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

	int parent_count=0;
	git_commit* head = getHead();

	const git_commit* parents[] = {nullptr};

	if (head)
		parents[parent_count++]=head;

	std::vector<std::string> paths=FileIO::getAllFiles(repo_dir);

	git_tree* tree = setStaged(paths);

	git_oid oid_for_commit;

	git_commit_create(
	  &oid_for_commit,
	  repo,
	  "HEAD",						/* name of ref to update */
	  sig,							/* author */
	  sig,							/* committer */
	  "UTF-8",						/* message encoding */
	  message.c_str(),				/* message */
	  tree,							/* root tree to commit*/
	  parent_count,					/* parent count */
	  parents);						/* parents */

	git_tree_free(tree);

	if (head)
		git_commit_free(head);
	git_signature_free(sig);
}

void GitOps::fetch(context::context_t& context) {
	git_remote* origin = getOrigin(context);
	git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
	opts.download_tags=GIT_REMOTE_DOWNLOAD_TAGS_NONE;
	git_remote_fetch(origin,
		NULL, /* refspecs, NULL to use the configured ones */
		&opts, /* options, empty for defaults */
		NULL); /* reflog mesage, usually "fetch" or "pull", you can leave it NULL for "fetch" */
	git_remote_free(origin);
}

MergeResult GitOps::merge(
		merge_style style, ops::Operation& ops,context::context_t& context) {

	git_commit* remote_commit = getFetchCommit();
	git_commit* common_commit = getCommon(context);

	assert(!ops.savePending());
	ops.loadAll();

	MergeResult to_return;
	std::vector<MergeConflict> merge_list;

	model::world_t common=modelFromCommit(common_commit);
	model::world_t remote=modelFromCommit(remote_commit);
	model::world_t& local=*ops.model;

	//1. merge tmp (remote) model into parent (local) model
	//2. save local model
	//3. add/commit this model
	//4. pull to last_pull

	//first, merge world:
	merge_string(local.title, common.title,remote.title,local.title,style,to_return, "World title");

	merge_id(local.first_room,common.first_room,remote.first_room,local.first_room,style,to_return,"ID of opening scenario");

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
		merge_string(rm_local.title,rm_common.title,rm_remote.title,rm_local.title,style,to_return,"Title of scenario id " + write_id(rm_id));

		//merge room body
		merge_string(rm_local.body,rm_common.body,rm_remote.body,rm_local.body,style,to_return,"Body text for scenario id " + write_id(rm_id));

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
					opt_remote.option_text,opt_local.option_text,style,to_return,"Option text for scenario id " + write_id(rm_id));

			//merge option destination:
			merge_id(opt_local.dst,opt_common.dst,opt_remote.dst,opt_local.dst,style,to_return,
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
					style,to_return,"dead-end flag for scenario id " + write_id(rm_id));
	}
	for (auto iter_rm : remote.rooms)
		if (!common.rooms.count(iter_rm.first))
			local.rooms[iter_rm.first]=iter_rm.second;

	if (style==TRIAL_RUN) {
		if (remote_commit)
			git_commit_free(remote_commit);
		if (common_commit)
			git_commit_free(common_commit);
		return to_return;
	}

	//save changes
	ops.saveAll(true);

	//commit
	git_signature* sig=nullptr;
	git_signature_now(&sig, context.user_name.c_str(), context.user_email.c_str());

	git_commit* head = getHead();

	int parent_n=0;

	const git_commit* parents[] = {nullptr,nullptr};
	if (head)
		parents[parent_n++]=head;
	if (remote_commit)
		parents[parent_n++]=remote_commit;

	git_tree* tree = setStaged(FileIO::getAllFiles(repo_dir));

	git_oid commit;

	git_commit_create(
	  &commit,
	  repo,
	  "HEAD",						/* name of ref to update */
	  sig,							/* author */
	  sig,							/* committer */
	  "UTF-8",						/* message encoding */
	  "merge remote",				/* message */
	  tree,							/* root tree */
	  parent_n,						/* parent count */
	  parents);						/* parents */

	git_oid oid_tag;

	git_object* obj_commit;
	git_object_lookup(&obj_commit,repo,git_commit_id(remote_commit),GIT_OBJ_COMMIT);

	git_tag_create(
			&oid_tag,
			repo,
			"last_common",
			obj_commit,
			sig,
			"local tag only",
			true
			);

	git_object_free(obj_commit);
	git_tree_free(tree);

	if (head)
		git_commit_free(head);
	git_signature_free(sig);

	//update last_pull.

	if (remote_commit)
		git_commit_free(remote_commit);
	if (common_commit)
		git_commit_free(common_commit);

	return to_return;
}

struct cred_payload {
	push_cred& credentials;
	bool* kill;
};

int packbuilder_progress_cb(int stage, unsigned int current, unsigned int total, void *payload){
	return 0;
}

int push_negotiation_cb(const git_push_update **updates, size_t len, void *payload){
	return 0;
}

int push_progress_cb(unsigned int current, unsigned int total, size_t bytes, void *payload)
{
	return 0;
}

int transfer_progress_cb(const git_transfer_progress *stats, void *payload){
	return 0;
}

int text_cb(const char *str, int len, void *payload){
	std::cout<<str<<'\n';
	return 0;
}

int cred_cb(git_cred** cred, const char * url, const char * url_username, unsigned int, void* payload_void){
	cred_payload* payload=(cred_payload*)payload_void;
	if (*payload->kill) {
		return -1;
	}
	git_credtype_t& credtype=*(git_credtype_t*)nullptr;
	switch (payload->credentials.credtype) {
	case push_cred::USERNAME:
		git_cred_userpass_plaintext_new(cred,
				payload->credentials.cred_a.c_str(),
				nullptr);
		break;
	case push_cred::PLAINTEXT:
		git_cred_userpass_plaintext_new(cred,
				payload->credentials.cred_a.c_str(),
				payload->credentials.cred_b.c_str());
		break;
	case push_cred::SSH:
		git_cred_ssh_key_new(cred,
				payload->credentials.cred_c.c_str(),
				payload->credentials.cred_b.c_str(),
				payload->credentials.cred_a.c_str(),
				payload->credentials.cred_d.c_str());
		break;
	}
	return 0;
}

int cert_check_cb(git_cert* cert, int valid, const char* host, void* payload){
	return 1;
}

bool GitOps::push_direct(context::context_t& context,push_cred credentials,bool* kill){
	cred_payload payload={credentials,kill};
	git_remote* remote = getOrigin(context);
	git_strarray refspecs;
	git_remote_get_push_refspecs(&refspecs,remote);
	if (refspecs.count==0)
		//add default refspec
		git_remote_add_push(repo,git_remote_name(remote), "refs/heads/master:refs/heads/master");
	git_remote_free(remote);
	remote=getOrigin(context);
	git_push_options opts;
	git_push_init_options(&opts,GIT_PUSH_OPTIONS_VERSION);
	git_remote_init_callbacks(&opts.callbacks,GIT_REMOTE_CALLBACKS_VERSION);
	opts.callbacks.credentials=cred_cb;
	opts.callbacks.payload=(void*)&payload;
	opts.callbacks.certificate_check=cert_check_cb;
	opts.callbacks.sideband_progress=text_cb;
	opts.callbacks.push_transfer_progress=push_progress_cb;
	opts.callbacks.push_negotiation=push_negotiation_cb;
	opts.callbacks.pack_progress=packbuilder_progress_cb;
	opts.callbacks.transfer_progress=transfer_progress_cb;
	git_remote_get_push_refspecs(&refspecs,remote);
	std::cout<<"refspecs:\n";
	bool to_return = !git_remote_push(remote,&refspecs,&opts);
	git_remote_free(remote);
	return to_return;
}

bool GitOps::push(context::context_t& context, push_cred credentials,bool (*push_kill_callback)(void*), void (*disconnect_interrupt)(bool,void*),void* varg) {
	//set to zero if t_helper returns, 1 if t_kill returns
	bool completed=false;
	bool returnval[2]={false,false};
	bool kill=false;

	std::thread t_helper([&]{
		returnval[0]= push_direct(context,credentials,&kill);
		disconnect_interrupt(returnval[0],varg);
	});

	std::thread t_kill([&] {
		if (push_kill_callback(varg)&&!completed) {
			kill=true;
			std::this_thread::yield();
			returnval[1]=true;
		} else
			returnval[1]=false;
	});

	//wait for remote thread or kill signal
	t_helper.join();
	completed=true;
	t_kill.join();
	return returnval[0];
}

git_tree* GitOps::setStaged(std::vector<std::string> paths) {

	/*git_index* index;
	git_repository_index(&index, repo);
	//add paths to index:
	for (std::string path : paths) {
		git_index_add_bypath(index, path.c_str());
	}

	git_oid tree_oid;
	git_tree* tree;
	git_index_write_tree(&tree_oid, index);
	git_tree_lookup(&tree, repo, &tree_oid);

	git_index_free(index);

	return tree;*/

	git_treebuilder* bld = nullptr;
	git_tree* src=nullptr;
	git_commit* head = getHead();
	if (head)
		git_commit_tree(&src, head);
	git_treebuilder_new(&bld, repo, src);
	assert(bld);
	if (head)
		git_commit_free(head);

	// Add some entries
	for (auto path : paths) {
		git_oid oid;
		//create blob, then add to tree:
		assert(!git_blob_create_fromdisk(&oid, repo, path.c_str()));
		std::string filename=FileIO::getFilename(path);
		git_treebuilder_insert(nullptr, bld,
			   filename.c_str(),
			   &oid, // OID
			   GIT_FILEMODE_BLOB);  // mode
	}

	git_oid oid;
	git_treebuilder_write(&oid, bld);
	git_treebuilder_free(bld);
	git_tree* tree=nullptr;
	git_tree_lookup(&tree,repo,&oid);
	return tree;
}

git_commit* GitOps::getHead() {
	git_commit* head = nullptr;

	git_reference * ref = nullptr;
	git_reference_lookup(&ref, repo, "refs/heads/master");
	if (ref) {
		const git_oid * parent_oid = git_reference_target(ref);
		git_commit_lookup(&head, repo, parent_oid);
	}
	git_reference_free(ref);
	return head;
}

git_commit* GitOps::getFetchCommit() {
	git_commit* remote = nullptr;

	git_reference * ref = nullptr;
	git_reference_dwim(&ref, repo, "remotes/origin/master");
	if (ref) {
		const git_oid * parent_oid = git_reference_target(ref);
		git_commit_lookup(&remote, repo, parent_oid);
	}
	git_reference_free(ref);
	return remote;
}

bool GitOps::commonHistoryExists(context::context_t& context) {
	git_commit* commit=getCommon(context);
	if (commit) {
		git_commit_free(commit);
		return true;
	}
	return false;
}

git_commit* GitOps::getCommon(context::context_t& context) {
	git_tag* last_common=getTagFromName("last_common");
	if (last_common) {
		git_commit* common;
		git_commit_lookup(&common,repo,git_tag_target_id(last_common));
		git_tag_free(last_common);
		return common;
	}

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
			git_commit* tmp=nullptr;
			if (!git_commit_parent(&tmp, local_c, 0))
				local_ancestors.push_back(tmp);
			local_c=tmp;
		} else
			break;

	while (remote_c != nullptr)
		if (git_commit_parentcount(remote_c)) {
			git_commit* tmp=nullptr;
			if (!git_commit_parent(&tmp, remote_c, 0))
				remote_ancestors.push_back(tmp);
			remote_c=tmp;
		} else
			break;

	//find first difference in list

	auto iter_loc = local_ancestors.rbegin();
	auto iter_rem = remote_ancestors.rbegin();

	if (local_ancestors.size()==0||remote_ancestors.size()==0||*iter_loc != *iter_rem)
		//no common history
		to_return = nullptr;
	else
		while (!to_return) {
			/*char id[40];
			git_oid_tostr(id,40,git_commit_id(*iter_loc));
			std::cout<<id<<'\n';
			git_oid_tostr(id,40,git_commit_id(*iter_rem));
			std::cout<<id<<"\n\n";*/
			if (iter_loc + 1 == local_ancestors.rend())
				to_return = *iter_loc;
			else if (iter_rem + 1 == remote_ancestors.rend())
				to_return = *iter_rem;
			else if (*(iter_loc + 1) != *(iter_rem + 1)) {
				to_return = *iter_loc; //==iter_rem
			}
			iter_loc++;
			iter_rem++;
		}

	for (auto iter : local_ancestors)
		if (iter)
			git_commit_free(iter);

	for (auto iter : remote_ancestors)
		if (iter)
			git_commit_free(iter);

	return to_return;
}

void GitOps::setOrigin(context::context_t context) {
	//method sets origin if not pre-existing
	git_remote_free(getOrigin(context));
}

void GitOps::clear() {
	git_repository_free(repo);
	FileIO::deletePath(repo_dir);
	init();
}

typedef struct {

	git_oid found_oid;
	bool found = false;

} tag_data;

int each_tag(const char *name, git_oid *oid, void *payload) {
	tag_data *d = (tag_data*) payload;
	if (!std::string(name).compare("refs/tags/last_common")) {
		d->found_oid = *oid;
		d->found = true;
	}
	return 0;
}

git_tag* GitOps::getTagFromName(std::string name) {
	tag_data d;
	int error = git_tag_foreach(repo, each_tag, &d);
	if (!d.found)
		return nullptr;
	git_tag* tag = nullptr;
	git_tag_lookup(&tag, repo, &d.found_oid);
	return tag;
}

git_remote* GitOps::getOrigin(context::context_t& context) {
	git_remote* origin = nullptr;
	git_remote_lookup(&origin, repo, "origin");
	if (origin) {
		if (context.upstream_url.compare(std::string(git_remote_url(origin)))) {
			git_remote_set_url(repo, "origin", context.upstream_url.c_str());
			git_remote_set_pushurl(repo, "origin",
					context.upstream_url.c_str());
		}
		return origin;
	}
	git_remote_create(&origin, repo, "origin", context.upstream_url.c_str());
	git_remote_set_pushurl(repo, "origin", context.upstream_url.c_str());
	return origin;
}

void GitOps::merge_string(std::string& result, std::string common,
		std::string remote, std::string local, merge_style style,
		MergeResult& log, std::string error_description) {
	if (style == FORCE_LOCAL) {
		result = local;
		return;
	}
	if (style == FORCE_REMOTE) {
		result = remote;
		log.changes++;
		return;
	}
	//local and remote possibly changed, but both the same:
	if (!remote.compare(local)) {
		result = local;
		return;
	}
	//local and remote changed, both different
	if (common.compare(remote) && common.compare(local)) {
		switch (style) {
		case USE_LOCAL:
			result = local;
			log.resolved += 1;
			return;
		case USE_REMOTE:
			result = remote;
			log.resolved += 1;
			log.changes++;
			return;
		case TRIAL_RUN:
			log.resolved += 1;
			log.changes++;
			result = local;
			return;
		case FORCE_LOCAL:
		case FORCE_REMOTE:
			// were already handled earlier in the function.
			assert(false);
			return;
		case MANUAL:
			MergeConflict mc;
			mc.data_type = MergeConflict::STRING;
			mc.description = error_description;
			mc.description += "\noriginal version: " + common;
			mc.description += "\nlocal revision: " + local;
			mc.description += "\nremote revision: " + remote;
			mc.common = common;
			mc.remote = remote;
			mc.local = local;
			mc.data_ptr = &result;
			log.conflicts.push_back(mc);
			return;
		}
	}
	//only remote changed:
	if (common.compare(remote)) {
		if (style != TRIAL_RUN)
			result = remote;
		log.changes++;
		return;
	}
	//only local changed:
	if (common.compare(local)) {
		result = local;
		return;
	}
	//neither changed:
	assert(!common.compare(local) && !remote.compare(local));
	result = common;
}

void GitOps::merge_id(model::id_type& result, model::id_type common,
		model::id_type remote, model::id_type local, merge_style style,
		MergeResult& log, std::string error_description) {
	if (style == FORCE_LOCAL) {
		result = local;
		return;
	}
	if (style == FORCE_REMOTE) {
		result = remote;
		log.changes++;
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
			log.resolved++;
			return;
		case USE_REMOTE:
			result = remote;
			log.resolved++;
			log.changes++;
			return;
		case TRIAL_RUN:
			result = local;
			log.resolved++;
			log.changes++;
			return;
		case FORCE_LOCAL:
		case FORCE_REMOTE:
			// were already handled earlier in the function.
			assert(false);
			return;
		case MANUAL:
			MergeConflict mc;
			mc.data_type = MergeConflict::ID;
			mc.description = error_description;
			mc.description += "\noriginal version: " + write_id(common);
			mc.description += "\nlocal revision: " + write_id(local);
			mc.description += "\nremote revision: " + write_id(remote);
			mc.common = write_id(common);
			mc.remote = write_id(remote);
			mc.local = write_id(local);
			mc.data_ptr = &result;
			log.conflicts.push_back(mc);
			result = remote;
		}
	}
	//only remote changed:
	if (common != remote) {
		if (style != TRIAL_RUN)
			result = remote;
		log.changes++;
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

void GitOps::merge_bool(bool& result, bool common, bool remote, bool local,
		merge_style style, MergeResult& log,
		std::string error_description) {
	if (style == FORCE_LOCAL) {
		result = local;
		return;
	}
	if (style == FORCE_REMOTE) {
		result = remote;
		log.changes++;
		return;
	}
	//local and remote changed, but both the same:
	if (remote == local) {
		result = local;
		return;
	}
	//local and remote changed, both different -- conflict!
	if (common != remote && common != local) {
		switch (style) {
		case USE_LOCAL:
			result = local;
			log.resolved++;
			return;
		case USE_REMOTE:
			result = remote;
			log.changes++;
			log.resolved++;
			return;
		case TRIAL_RUN:
			log.resolved++;
			log.changes++;
			result = local;
			return;
		case FORCE_LOCAL:
		case FORCE_REMOTE:
			// Were already handled earlier in the function.
			assert(false);
			return;
		case MANUAL:
			MergeConflict mc;
			mc.data_type = MergeConflict::BOOL;
			mc.description = error_description;
			mc.description += std::string(
					"\noriginal version: "
							+ std::string((common) ? "True" : "False"));
			mc.description += std::string(
					"\nlocal revision: "
							+ std::string((local) ? "True" : "False"));
			mc.description += std::string(
					"\nremote revision: "
							+ std::string((remote) ? "True" : "False"));
			mc.common = '0' + common;
			mc.remote = '0' + remote;
			mc.local = '0' + local;
			mc.data_ptr = &result;
			log.conflicts.push_back(mc);
			result = remote;
		}
	}
	//only remote changed:
	if (common != remote) {
		if (style != TRIAL_RUN)
			result = remote;
		log.changes++;
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

push_cred make_push_cred_username(std::string username) {
	push_cred cred;
	cred.credtype=push_cred::USERNAME;
	cred.cred_a=username;
	return cred;
}

push_cred make_push_cred_plaintext(std::string username, std::string password) {
	push_cred cred;
	cred.credtype=push_cred::PLAINTEXT;
	cred.cred_a=username;
	cred.cred_b=password;
	return cred;
}

push_cred make_push_cred_ssh(std::string path_to_privkey,
		std::string path_to_pubkey, std::string username,
		std::string passphrase)  {
	push_cred cred;
	cred.credtype=push_cred::SSH;
	cred.cred_a=path_to_privkey;
	cred.cred_b=path_to_pubkey;
	cred.cred_c=username;
	cred.cred_d=passphrase;
	return cred;
}

}
}
