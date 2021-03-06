/*
 * Fileio.cpp
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#include "FileIO.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/iterator/iterator_facade.hpp"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gyoa/Context.h"
#include "gyoa/error.h"
#include "gyoa/id_parse.h"
#include "gyoa/Room.h"
#include "gyoa/World.h"

using namespace gyoa;

std::vector<std::string> gyoa::FileIO::getAllFiles(std::string directory, bool (*valid_name)(const std::string)) {
	using namespace boost::filesystem;
	path p(directory);
	std::vector<std::string> to_return;
	directory_iterator end_itr;
	for (directory_iterator iter(p); iter != end_itr; ++iter) {
		// If it's not a directory, list it.
		if (is_regular_file(iter->path()))
			if (valid_name(iter->path().filename().string()))
				to_return.push_back(boost::filesystem::canonical(iter->path()).string());
	}
	return to_return;
}

void gyoa::FileIO::deletePath(std::string path_str) {
	boost::filesystem::remove_all(boost::filesystem::path(path_str));
}

void FileIO::writeRoomToFile(const model::room_t rm, std::string fname) {
	using namespace rapidxml;

	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	//write room version to doc (not currently read)
	xml_node<>* root = doc.allocate_node(node_element, "room");
	root->append_attribute(doc.allocate_attribute("version", "0.2"));
	doc.append_node(root);

	//write room title to doc
	xml_node<>* title = doc.allocate_node(node_element, "title");
	title->append_attribute(doc.allocate_attribute("text", rm.title.c_str()));
	root->append_node(title);

	//write room header to doc
	xml_node<>* body = doc.allocate_node(node_element, "body");
	body->append_attribute(doc.allocate_attribute("text", rm.body.c_str()));
	root->append_node(body);

	//stores strings so that they persist past the following loop.
	struct Store {
		std::string dst, text, id;
	};
	std::vector<Store> store;
	store.reserve(rm.options.size()); // Don't reallocate ever. The xml lib
									  // will write to file after the loop.

	for (auto opt_it : rm.options) {
		store.push_back( { write_id(opt_it.second.dst),
						   opt_it.second.option_text,
						   write_id(opt_it.first) } );
		xml_node<>* opt_n = doc.allocate_node(node_element, "option");
		opt_n->append_attribute(doc.allocate_attribute("dst",  store.back().dst .c_str()));
		opt_n->append_attribute(doc.allocate_attribute("text", store.back().text.c_str()));
		opt_n->append_attribute(doc.allocate_attribute("id",   store.back().id  .c_str()));
		root->append_node(opt_n);
	}

	//include dead-end flag as node in doc if applicable.
	xml_node<>* dead_end = doc.allocate_node(node_element, "dead_end");
	if (rm.dead_end)
		root->append_node(dead_end);

	//write doc to file
	std::ofstream myfile;
	myfile.open(fname);
	myfile << doc;
	myfile.close();
}

model::room_t FileIO::loadRoom(std::string filename)  {

	std::ifstream file(filename);
	if (!file.good())
		throw FileNotFoundException(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content(buffer.str());

	return loadRoomFromText(content);
}

model::room_t gyoa::FileIO::loadRoomFromText(std::string text) {
	using namespace rapidxml;
	model::room_t rm_import;
	rm_import.loaded=true;
	xml_document<> doc;
	doc.parse<0>(&text[0]);
	xml_node<>* nd_rm = doc.first_node("room");
	rm_import.title = nd_rm->first_node("title")->first_attribute("text")->value();
	rm_import.body = nd_rm->first_node("body")->first_attribute("text")->value();
	rm_import.dead_end = !!nd_rm->first_node("dead_end");
	bool tr1 = true;
	xml_node<>* nd_opt = nd_rm->first_node("option");
	while (true) {
		if (!tr1)
			nd_opt = nd_opt->next_sibling("option");
		if (!nd_opt)
			break;
		model::option_t opt_import;
		opt_import.dst = parse_id(nd_opt->first_attribute("dst")->value());
		opt_import.option_text = nd_opt->first_attribute("text")->value();
		model::id_type opt_id = parse_id(nd_opt->first_attribute("id")->value());
		rm_import.options[opt_id]=opt_import;
		tr1=false;
	}
	return rm_import;
}

void gyoa::FileIO::writeWorldToFile(const gyoa::model::world_t& world,
		std::string filename) {
	using namespace rapidxml;

	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	//write version information to doc; not currently read.
	xml_node<>* root = doc.allocate_node(node_element, "world");
	root->append_attribute(doc.allocate_attribute("version", "0.2"));

	//write the id of the opening room to the doc
	std::string id = write_id(world.first_room);
	root->append_attribute(doc.allocate_attribute("start_rm_id", id.c_str()));

	//write the id of the next room to be created to the doc
	std::string gid = std::to_string(world.next_rm_gid);
	root->append_attribute(doc.allocate_attribute("next_rm_gid", gid.c_str()));
	doc.append_node(root);

	//write the title of the world to the doc
	xml_node<>* title = doc.allocate_node(node_element, "title");
	title->append_attribute(doc.allocate_attribute("text", world.title.c_str()));
	root->append_node(title);

	//stores char* strings of id information to persist after loop below
	std::vector<std::string> id_store;
	id_store.reserve(world.rooms.size());

	//write the gid of every room in the world to the doc.
	for (auto rm_it : world.rooms) {
		xml_node<>* rm = doc.allocate_node(node_element, "rm");
		id_store.push_back(write_id(rm_it.first));
		rm->append_attribute(doc.allocate_attribute("id", id_store.back().c_str()));
		root->append_node(rm);
	}

	//write the doc to a file
	std::ofstream myfile;
	myfile.open(filename);
	myfile << doc;
	myfile.close();
}

gyoa::model::world_t gyoa::FileIO::loadWorld(std::string filename) {

	std::ifstream file(filename);
	if (!file.good())
		throw FileNotFoundException(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content(buffer.str());
	return loadWorldFromText(content);
}

gyoa::model::world_t gyoa::FileIO::loadWorldFromText(std::string text) {
	using namespace rapidxml;

	model::world_t wd_import;
	xml_document<> doc;

	doc.parse<0>(&text[0]);
	xml_node<>* nd_wd = doc.first_node("world");
	wd_import.first_room = parse_id(
			nd_wd->first_attribute("start_rm_id")->value());
	wd_import.next_rm_gid = atoi(
			nd_wd->first_attribute("next_rm_gid")->value());
	wd_import.title =
			nd_wd->first_node("title")->first_attribute("text")->value();
	auto node_rm = nd_wd->first_node("rm");
	while (node_rm) {
		model::room_t fake;
		fake.loaded = false;
		auto id = parse_id(node_rm->first_attribute("id")->value());
		wd_import.rooms[id] = fake;
		node_rm = node_rm->next_sibling("rm");
	}
	return wd_import;
}

void gyoa::FileIO::writeGitignore(std::string filename) {
	std::ofstream myfile;
	myfile.open(filename.c_str());
	myfile << "context.txt";
	myfile.close();
}

void gyoa::FileIO::writeContext(context::context_t context,
		std::string filename) {
	using namespace rapidxml;
	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	//write version information to doc; not currently read.
	xml_node<>* root = doc.allocate_node(node_element, "context");
	root->append_attribute(doc.allocate_attribute("version", "0.1"));
	doc.append_node(root);

	//write username
	xml_node<>* usrnm = doc.allocate_node(node_element, "username");
	usrnm->append_attribute(doc.allocate_attribute("text", context.user_name.c_str()));
	root->append_node(usrnm);

	//write email
	xml_node<>* email = doc.allocate_node(node_element, "email");
	email->append_attribute(doc.allocate_attribute("text", context.user_email.c_str()));
	root->append_node(email);

	//write upstream remote
	xml_node<>* upstream = doc.allocate_node(node_element, "upstream");
	upstream->append_attribute(doc.allocate_attribute("text", context.upstream_url.c_str()));
	root->append_node(upstream);

	//write current room
	std::string id = write_id(context.current_room);
	xml_node<>* rm = doc.allocate_node(node_element, "current_room");
	rm->append_attribute(doc.allocate_attribute("id", id.c_str()));
	root->append_node(rm);

	//write authentication details:
	xml_node<>* authentication = doc.allocate_node(node_element, "authentication");
	authentication->append_attribute(doc.allocate_attribute("no_store", (context.git_authentication_prefs.do_not_store)?"true":"false"));

	if (!context.git_authentication_prefs.do_not_store) {
		//username:
		xml_node<>* auth_username = doc.allocate_node(node_element, "user");
		auth_username->append_attribute(doc.allocate_attribute("name",context.git_authentication_prefs.user_name.c_str()));
		authentication->append_node(auth_username);

		xml_node<>* auth_privkey_path = doc.allocate_node(node_element, "privkey");
		auth_privkey_path->append_attribute(doc.allocate_attribute("path",context.git_authentication_prefs.path_to_privkey.c_str()));
		authentication->append_node(auth_privkey_path);

		xml_node<>* auth_pubkey_path = doc.allocate_node(node_element, "pubkey");
		auth_pubkey_path->append_attribute(doc.allocate_attribute("path",context.git_authentication_prefs.path_to_pubkey.c_str()));
		authentication->append_node(auth_pubkey_path);
	}

	root->append_node(authentication);

	//write the doc to a file
	std::ofstream myfile;
	myfile.open(filename);
	myfile << doc;
	myfile.close();
}

context::context_t gyoa::FileIO::loadContext(std::string filename) {
	using namespace rapidxml;
	std::ifstream file(filename);
	if (!file.good())
		throw FileNotFoundException(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string text(buffer.str());

	context::context_t context;
	xml_document<> doc;
	doc.parse<0>(&text[0]);
	xml_node<>* nd_ct = doc.first_node("context");
	context.user_name =
			nd_ct->first_node("username")->first_attribute("text")->value();
	context.user_email =
			nd_ct->first_node("email")->first_attribute("text")->value();
	context.upstream_url =
			nd_ct->first_node("upstream")->first_attribute("text")->value();
	context.current_room = parse_id(
			nd_ct->first_node("current_room")->first_attribute("id")->value());
	//authentication:
	auto node_auth = nd_ct->first_node("authentication");
	if (node_auth) {
		//determine if authentication is stored:
		char* str = node_auth->first_attribute("no_store")->value();
		if (!std::strcmp(str,"false"))
			context.git_authentication_prefs.do_not_store=false;
		else if (!std::strcmp(str,"true"))
			context.git_authentication_prefs.do_not_store=true;
		else
			assert(false);

		if (!context.git_authentication_prefs.do_not_store) {
			//remaining authentication preferences:
			context.git_authentication_prefs.user_name = node_auth->first_node(
					"user")->first_attribute("name")->value();
			context.git_authentication_prefs.path_to_privkey =
					node_auth->first_node("privkey")->first_attribute("path")->value();
			context.git_authentication_prefs.path_to_pubkey =
					node_auth->first_node("pubkey")->first_attribute("path")->value();
		}
	}
	return context;
}

std::string gyoa::FileIO::getFilename(std::string filepath) {
	return boost::filesystem::path(filepath).filename().string();
}

bool gyoa::FileIO::fileExists(std::string path) {
	return boost::filesystem::exists(path);
}

std::string gyoa::FileIO::getCanonicalPath(std::string path) {
	if (!path.substr(0,1).compare("~"))
		path=getenv("HOME")+path.substr(1);
	return boost::filesystem::canonical(boost::filesystem::path(path)).string();
}
