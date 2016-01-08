/*
 * driver.cpp
 *
 *  Created on: Apr 21, 2015
 *      Author: n
 */

#include "frontend/console/ConsoleUI.h"

int main(int arg_c, char** arg_v) {
	using namespace gyoa::model;
	gyoa::ui::ConsoleUI cui;
	if (arg_c>1)
		cui.start(arg_v[1]);
	else
		cui.start();
}
