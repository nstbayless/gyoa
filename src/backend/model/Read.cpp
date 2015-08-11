/*
 * Read.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: n
 */

#include "Read.h"

namespace gyoa {
namespace model {

const opt_id_t getOption(room_t& room, int n) {
	{
		for (auto iter : room.options) {
			n -= 1;
			if (n == 0) {
				return iter.first;
			}
		}
		return {-1,-1};
	}
}

} /* namespace model */
} /* namespace gyoa */
