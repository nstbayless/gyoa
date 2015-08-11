/*
 * Read.h
 *
 *  Created on: Aug 11, 2015
 *      Author: n
 *
 *  various methods for reading information from the model.
 *  (structs in the model can also be read directly.)
 */

#ifndef BACKEND_MODEL_READ_H_
#define BACKEND_MODEL_READ_H_

#include "Room.h"

namespace gyoa {
namespace model {

//! retrieves nth option by id from given room, where n is from 1 to 9.
//! returns {-1, -1} if no option found.
const opt_id_t getOption(room_t& room, int n);

} /* namespace model */
} /* namespace gyoa */

#endif /* BACKEND_MODEL_READ_H_ */
