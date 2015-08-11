/*
 * Fileio.cpp
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#include "FileIO.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include "../error.h"
#include "../id_parse.h"
#include "../model/Room.h"
#include "../model/World.h"

using namespace cyoa;

FileIO::FileIO() {

}

FileIO::~FileIO() {
}

void FileIO::writeRoomToFile(model::room_t rm, std::string fname) const {
	using namespace rapidxml;

	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	xml_node<>* root = doc.allocate_node(node_element, "room");
	root->append_attribute(doc.allocate_attribute("version", "0.2"));
	doc.append_node(root);

	xml_node<>* title = doc.allocate_node(node_element, "title");
	title->append_attribute(doc.allocate_attribute("text", rm.title.c_str()));
	root->append_node(title);

	xml_node<>* body = doc.allocate_node(node_element, "body");
	body->append_attribute(doc.allocate_attribute("text", rm.body.c_str()));
	root->append_node(body);
	std::vector<std::pair<std::string,std::string>> aaa;
	int option = 0;
	for (auto opt_p : rm.options) {
		auto opt=opt_p.second;
		xml_node<>* opt_n = doc.allocate_node(node_element, "option");
		auto a = std::pair<std::string,std::string>(write_id(opt.dst),write_id(opt_p.first));
		aaa.push_back(a);
		opt_n->append_attribute(doc.allocate_attribute("dst", aaa.back().first.c_str()));
		opt_n->append_attribute(doc.allocate_attribute("text", opt.option_text.c_str()));
		opt_n->append_attribute(doc.allocate_attribute("id",aaa.back().second.c_str()));
		root->append_node(opt_n);
		option++;
	}

	xml_node<>* dead_end = doc.allocate_node(node_element, "dead_end");
	if (rm.dead_end)
		root->append_node(dead_end);

	std::ofstream myfile;
	myfile.open(fname);
	myfile << doc;
	myfile.close();
}

model::room_t FileIO::loadRoom(std::string filename) const  {
	using namespace rapidxml;

	model::room_t rm_import;
	xml_document<> doc;
	std::ifstream file(filename);
	if (!file.good())
		throw FileNotFoundException(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content(buffer.str());
	doc.parse<0>(&content[0]);
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

void cyoa::FileIO::writeWorldToFile(cyoa::model::world_t& world,
		std::string filename) {
	using namespace rapidxml;

	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	xml_node<>* root = doc.allocate_node(node_element, "world");
	root->append_attribute(doc.allocate_attribute("version", "0.2"));
	std::string id = write_id(world.first_room);
	root->append_attribute(doc.allocate_attribute("start_rm_id", id.c_str()));
	std::string gid = std::to_string(world.next_rm_gid);
	root->append_attribute(doc.allocate_attribute("next_rm_gid", gid.c_str()));
	doc.append_node(root);

	xml_node<>* title = doc.allocate_node(node_element, "title");
	title->append_attribute(doc.allocate_attribute("text", world.title.c_str()));
	root->append_node(title);

	std::vector<std::string> aa;
	for (auto iter : world.rooms){
		xml_node<>* rm = doc.allocate_node(node_element, "rm");
		aa.push_back(write_id(iter.first));
		rm->append_attribute(doc.allocate_attribute("id", aa.back().c_str()));
		root->append_node(rm);
	}
	std::ofstream myfile;
	myfile.open(filename);
	myfile << doc;
	myfile.close();
}

cyoa::model::world_t cyoa::FileIO::loadWorld(std::string filename) {
	using namespace rapidxml;

	model::world_t wd_import;
	xml_document<> doc;
	std::ifstream file(filename);
	if (!file.good())
		throw FileNotFoundException(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content(buffer.str());
	doc.parse<0>(&content[0]);
	xml_node<>* nd_wd = doc.first_node("world");
	wd_import.first_room=parse_id(nd_wd->first_attribute("start_rm_id")->value());
	wd_import.next_rm_gid=atoi(nd_wd->first_attribute("next_rm_gid")->value());
	wd_import.title = nd_wd->first_node("title")->first_attribute("text")->value();
	auto node_rm = nd_wd->first_node("rm");
	while (node_rm) {
		model::room_t fake;
		fake.loaded=false;
		auto id = parse_id(node_rm->first_attribute("id")->value());
		wd_import.rooms[id]=fake;
		node_rm=node_rm->next_sibling("rm");
	}
	return wd_import;
}
