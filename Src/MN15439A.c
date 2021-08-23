/*
 * MN15439A.c
 *
 *  Created on: Aug 23, 2021
 *      Author: TinLethax
 */

#include "MN15439A.h"
#include "font8x8_basic.h"
#include <string.h>
#include <stdlib.h>

// 288bit data buffer for 1 grid.
static uint8_t GPbuff[36] ; // 8x36 bit array (288 bits == What display expected).

//6 bit array ; Format (MSB)[a,b,c,d,e,f,0,0](LSB)
static uint8_t ABCarray[ABCARRAY_SIZE];

// LUT for converting normal ABCDEF format to AFBDCD format
static const uint8_t reOrder0[2][6]={// first 4 bytes (GPbuff[0and3])
{0,2,0,3,0,4}, /* the bit shift to convert def to the afbecd format */
{7,0,6,0,5,0} /* the bit shift to convert abc to the afbecd format */
};
static const uint8_t reOrder1[2][6]={// next 4 bytes (GPbuff[1and4])
{0,3,0,4,0,2}, /* the bit shift to convert efd to the afbecd format */
{6,0,5,0,7,0} /* the bit shift to convert bca to the afbecd format */
};
static const uint8_t reOrder2[2][6]={// last 4 bytes (GPbuff[2and5])
{0,4,0,2,0,3}, /* the bit shift to convert fde to the afbecd format */
{5,0,7,0,6,0} /* the bit shift to convert cab to the afbecd format */
};

// used for Display blanking
void Delay_us(uint16_t usecs){
	uint16_t count = (usecs * 48) / 4;
	for (uint16_t i = 0; i < count; ++i) {
	    count--;
	}
}

// Setting up all parameters
void VFDsetup(MN15439A *VFDDisp, SPI_HandleTypeDef *Bus, GPIO_TypeDef *vfdGPIO, uint16_t BLANK_pin, uint16_t LAT_pin){

	// Store params into our struct
	VFDDisp->Bus = Bus;
	VFDDisp->vfdGPIO = vfdGPIO;
	VFDDisp->BLANK_pin = BLANK_pin;
	VFDDisp->LAT_pin = LAT_pin;

	FB0 = malloc(750);// malloc the frame buffer memory

}

// Load 6bit data to the send buffer
void VFDLoadBMP(MN15439A *VFDDisp, uint8_t Grid){
	// Logically thinking : Determine the Grid is Event or Odd number (Important, For the simple algorithm to convert abcdef to afbecd format).
		uint8_t EvOd = 0;
		if(Grid%2){
			EvOd = 1;// odd number (odd grid), Only manipulate the a, b, c Dots
		}else{
			EvOd = 0;// event number (event grid), Only manipulate the d, e, f Dots
		}

		// clean the previous left over Grid bit
			memset(GPbuff, 0x00, 36);

uint8_t nxtWrd = 0;
uint8_t CMP= 0;

uint16_t X_offset = (uint8_t)((Grid-1) / 8) * 3;// calculate the horizontal offset, starting point for each grid number
uint16_t Y_offset =  0;

/* convert normal 8bit array into 6bit format */
// Convert the normal 154*39 bitmap frame buffer into 6bit format used by VFDLoadBMP
// need to work with this pattern below (repeat every 3 bytes, 8 grids per 3 bytes)
// [a][b][c][d][e][f]/[a][b] [c][d][e][f]/[a][b][c][d] [e][f]/[a][b][c][d][e][f]
//

//img1[x] format = (bit7)[a][b][c][d][e][f][0][0](bit0)

switch ((Grid-1) % 8){
case(0):
case(1):
	for(uint8_t i=0;i < 39; i++){
		Y_offset = X_offset + (i * 20);// calculate the vertical offset
		ABCarray[i] = FB0[Y_offset];// filtered only first 6 bit, 2 remain bits will be on next array
	}
break;

case(2):
case(3):
	for(uint8_t i=0;i < 39; i++){
		Y_offset = X_offset + (i * 20);// calculate the vertical offset
		ABCarray[i] = FB0[Y_offset] << 6;// bit 0 and 1 will be on [a] and [b] bit on next 6 bit array
		ABCarray[i] |= (FB0[Y_offset+1] >> 2) & 0x3C;// bit 7-4 will be on [c][d][e][f], 4 remain bits will be on next array
	}
break;

case(4):
case(5):
	for(uint8_t i=0;i < 39; i++){
		Y_offset = X_offset + (i * 20);// calculate the vertical offset
		ABCarray[i] = FB0[Y_offset+1] << 4;// bit 3-0 will be on [a][b][c][d]
		ABCarray[i] |= FB0[Y_offset+2] >> 4;// bit 7 and 6 will be on [e][f], 6 remain bits will be on next array
	}
break;

case(6):
case(7):
	for(uint8_t i=0;i < 39; i++){
		Y_offset = X_offset + (i * 20);// calculate the vertical offset
		ABCarray[i] = FB0[Y_offset+2] << 2;// push 6 remain bits to align with 6 bit array.
	}
break;

default:
	break;
}

/* Craft the 288 bit (39 bytes) packet per 1 grid */

for(uint8_t wdCount=0; wdCount < 2;wdCount++){// First 6 array repeat itself so do this twice.

		// GPbuff[0-3] and GPbuf[12-15]
		// 4 bytes on first buffer array
		// 1a 1f 1b 1e 1c 1d to 6a 6f

		for(uint8_t i=0;i < 32;i++){

			if(ABCarray[CMP] & (1 << reOrder0[EvOd][i%6])){
				GPbuff[(i/8)+nxtWrd] |= (1 << i%8);
			}else{
				GPbuff[(i/8)+nxtWrd] &= ~(1 << i%8);
			}

			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			CMP++;
			}
		}

		// GPbuff[4-7] and GPbuff[16-19]
		//
		//4 bytes on second buffer array + 4 bits for 11a, 11f, 11b and 11e
		for(uint8_t i=0;i < 32;i++){

			if(ABCarray[CMP] & (1 << reOrder1[EvOd][(i%6)])){
				GPbuff[4+(i/8)+nxtWrd] |= (1 << i%8);
			}else{
				GPbuff[4+(i/8)+nxtWrd] &= ~(1 << i%8);
			}

			if((i%6) == 3){// move to next bitmap when we complete previous 6bit BMP
			CMP++;
			}
		}

		// GPbuff[8-11] and GPbuff[20-23]
		//
		//4 bytes on 3rd buffer array
		for(uint8_t i=0;i < 32;i++){

			//memcpy(&CMP, sBMP,1);
			if(ABCarray[CMP] & (1 << reOrder2[EvOd][i%6])){
				GPbuff[8+(i/8)+nxtWrd] |= (1 << i%8);
			}else{
				GPbuff[8+(i/8)+nxtWrd] &= ~(1 << i%8);
			}

			if((i%6) == 1){// move to next bitmap when we complete previous 6bit BMP
			CMP++;
			}
		}

		nxtWrd = 12;// next time we start the array at GPbuff[12-15]
}//for(uint8_t wdCount=0; wdCount < 2;wdCount++)

		// GPbuff[24-27]
		// 33a to 38f

		for(uint8_t i=0;i < 32;i++){// for 2nd round

			if(ABCarray[CMP] & (1 << reOrder0[EvOd][i%6])){
				GPbuff[24+(i/8)] |= (1 << i%8);
			}else{
				GPbuff[24+(i/8)] &= ~(1 << i%8);
			}

			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			CMP++;
			}
		}

		for(uint8_t i=0;i < 10;i++){

			if(ABCarray[CMP] & (1 << reOrder1[EvOd][i%6])){
				GPbuff[28+(i/8)] |= (1 << i%8);
			}else{
				GPbuff[28+(i/8)] &= ~(1 << i%8);
			}

			if((i%6) == 3){// move to next bitmap when we complete previous 6bit BMP
			CMP++;
			}
		}

			if(Grid < 7){// Grid 1-6
				GPbuff[29] |= (3 << (Grid+1));// move to align with byte and Grid bit and turn Grid N and N+1 on
				if (Grid == 6)// in case Grid bit is on bit 7
					GPbuff[30] |= 0x01;// set next Grid bit on next array to 1

			}else if(Grid > 6){// Grid number is between 7 - 21
				// Finding array offset of each grid number
				//uint8_t G1Byteoffset = ((Grid-7) / 8) + 30;// Grid 1-21

				GPbuff[((Grid-7) / 8) + 30] |= (3 << ((Grid-7)%8));// Grid N and N+1 on
				if ((Grid-7)%8 == 7)// in case Grid bit is on bit 7
					GPbuff[((Grid-7) / 8) + 31] = 0x01;// set next Grid bit on next array to 1

			}else if(Grid == 22){// Grid number is 22
				GPbuff[31] = (1 << 7);// Grid 22 on
				GPbuff[32] = (1 << 0);// Grid 23 on

			}else{// Grid number is between 23-54
				// Finding array offset of each grid number
				//uint8_t G2Byteoffset = ((Grid - 23) / 8) + 32;// Grid 23-54

				GPbuff[((Grid - 23) / 8) + 32] |= (3 << ((Grid-23)%8));// Grid N and N+1 on
				if ((Grid-23)% 8 == 7)// in case Grid bit is on bit 7
					GPbuff[((Grid - 23) / 8) + 33] = 0x01;// set next Grid bit on next array to 1

			}

/* VFD Update : send data over SPI.*/

HAL_GPIO_WritePin(VFDDisp->vfdGPIO, VFDDisp->BLANK_pin, GPIO_PIN_SET);// BLK high
HAL_GPIO_WritePin(VFDDisp->vfdGPIO, VFDDisp->LAT_pin, GPIO_PIN_SET);// LAT high
Delay_us(1);
HAL_GPIO_WritePin(VFDDisp->vfdGPIO, VFDDisp->LAT_pin, GPIO_PIN_RESET);// LAT low
HAL_GPIO_WritePin(VFDDisp->vfdGPIO, VFDDisp->BLANK_pin, GPIO_PIN_RESET);// BLK low

HAL_SPI_Transmit(VFDDisp->Bus, GPbuff, 36, HAL_MAX_DELAY);// Send data over SPI

}

// Load full 160x39 into Frame buffer
void VFDLoadFull(uint8_t * BMP){
	memcpy(FB0, BMP, 780);
}

void VFDClear(){
	memset(FB0, 0, 780);
}

void VFDFill(bool fill){
	memset(FB0, fill ? 0xFF : 0, 780);
}
// Buffer update (with X,Y Coordinate and image WxH) X,Y Coordinate start at (1,1) to (50,240)
//
//NOTE THAT THE X COOR and WIDTH ARE BYTE NUMBER NOT PIXEL NUMBER (8 pixel = 1 byte). A.K.A IT'S BYTE ALIGNED
//
void VFDLoadPart(uint8_t* BMP, uint8_t Xcord, uint8_t Ycord, uint8_t bmpW, uint8_t bmpH){

	Xcord = Xcord - 1;
	Ycord = Ycord - 1;
	uint16_t XYoff,WHoff = 0;

	//Counting from Y origin point to bmpH using for loop
	for(uint8_t loop = 0; loop < bmpH; loop++){
		// turn X an Y into absolute offset number for Buffer
		XYoff = (Ycord+loop) * 20;
		XYoff += Xcord;// offset start at the left most, The count from left to right for Xcord times

		// turn W and H into absolute offset number for Bitmap image
		WHoff = loop * bmpW;

		memcpy(FB0 + XYoff, BMP + WHoff, bmpW);
	}

}
