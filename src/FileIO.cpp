/*
 * Fileio.cpp
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#include "FileIO.h"

#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <cassert>
#include <fstream>
#include <sstream>

FileIO::FileIO() {

}

FileIO::~FileIO() {
}

void FileIO::writeRoomToFile(room_t rm, std::string fname) const {
	using namespace rapidxml;

	assert(rm.id!=-1);

	if (fname=="")
		//default file name:
		fname = "room_" + std::to_string(rm.id) + ".txt";

	fname = data_path + "/" + fname;

	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	xml_node<>* root = doc.allocate_node(node_element, "room");
	root->append_attribute(doc.allocate_attribute("version", "0.1"));
	root->append_attribute(doc.allocate_attribute("id", std::to_string(rm.id).c_str()));
	doc.append_node(root);

	xml_node<>* title = doc.allocate_node(node_element, "title");
	title->append_attribute(doc.allocate_attribute("text", rm.title.c_str()));
	root->append_node(title);

	xml_node<>* body = doc.allocate_node(node_element, "body");
	body->append_attribute(doc.allocate_attribute("text", rm.body.c_str()));
	root->append_node(body);

	int option = 0;
	for (auto opt : rm.options) {
		xml_node<>* opt_n = doc.allocate_node(node_element, "option");;
		opt_n->append_attribute(doc.allocate_attribute("dst", std::to_string(opt.dst).c_str()));
		opt_n->append_attribute(doc.allocate_attribute("text", opt.option_text.c_str()));
		root->append_node(opt_n);
		option++;
	}

	std::ofstream myfile;
	myfile.open(fname);
	myfile << doc;
	myfile.close();
}

room_t FileIO::loadRoom(room_id id) const {
	std::string fn = data_path + "/room_"+std::to_string(id) + ".txt";
	room_t rm =  loadRoom(fn);
	assert(rm.id==id);
	return rm;
}

room_t FileIO::loadRoom(std::string filename) const  {
	using namespace rapidxml;
	room_t rm_import;
	xml_document<> doc;
	std::ifstream file(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content(buffer.str());
	doc.parse<0>(&content[0]);
	xml_node<>* nd_rm = doc.first_node("room");
	rm_import.id = atoi(nd_rm->first_attribute("id")->value());
	rm_import.title = nd_rm->first_node("title")->first_attribute("text")->value();
	rm_import.body = nd_rm->first_node("body")->first_attribute("text")->value();
	bool tr1 = true;
	xml_node<>* nd_opt = nd_rm->first_node("option");
	while (true) {
		if (!tr1)
			nd_opt = nd_opt->next_sibling("option");
		if (!nd_opt)
			break;
		option_t opt_import;
		opt_import.dst = atoi(nd_opt->first_attribute("dst")->value());
		opt_import.option_text = nd_opt->first_attribute("text")->value();
		rm_import.options.push_back(opt_import);
		tr1=false;
	}
	return rm_import;
}
