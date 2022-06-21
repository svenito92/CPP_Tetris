/*
 * Calculations.h
 *
 *  Created on: May 29, 2022
 *      Author: diktux
 */
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "Playground.h"
#include "Block.h"



#ifndef SRCCPP_CALCULATIONS_H_
#define SRCCPP_CALCULATIONS_H_

class Calculations {
public:
	Calculations();
	virtual ~Calculations();

	uint8_t getRdmBlock();
	void getRdmSpaceInNewLine(uint8_t* pointer);



private:


};

#endif /* SRCCPP_CALCULATIONS_H_ */
