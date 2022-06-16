/*
 * Calculations.cpp
 *
 *  Created on: May 29, 2022
 *      Author: diktux
 */

#include "Calculations.h"
#include "TFT_Functions.h"


Calculations::Calculations() {
	// TODO Auto-generated constructor stub

}

Calculations::~Calculations() {
	// TODO Auto-generated destructor stub
}

// returns a rdm value between 1..7
uint8_t Calculations::getRdmBlock(){
	uint8_t rdm;
	for(uint8_t i = 0;i<5;i++){
		rdm += rand()%11;
	}
	rdm += HAL_GetTick()%13;
	return rdm%7+1;
}

// returns a rdm value between 0..9
uint8_t Calculations::getRdmSpaceInNewLine(){ //uint16_t
	uint8_t rdm;
	for(uint8_t i = 0;i<5;i++){
		rdm += rand()%11;
	}
	rdm += HAL_GetTick()%13;
    return rdm%9; // devide through RAND_MAX/7
}

