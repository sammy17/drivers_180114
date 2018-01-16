#include <iostream>
#include <cmath>
#include <cfloat>
#include <ap_int.h>
#include <hls_math.h>
#include <string.h>


using namespace std;

#define WIDTH 320*2
#define HEIGHT 240*2
#define IMG_SIZE WIDTH*HEIGHT
#define K  2 // no of gaussian mxitures
#define BGM_SIZE IMG_SIZE*K
#define PARTS 60
typedef float data_t;
//typedef ap_fixed<32,16> data_t;




struct MixData
{
	data_t sortKey;
	data_t weight;
	data_t mean;
	data_t var;
};



void bgsub(uint8_t frame_in[IMG_SIZE],
           uint8_t frame_out[IMG_SIZE],
		   bool init,
		   data_t bgmodel[4*BGM_SIZE]);

void process(uint8_t frame_in[IMG_SIZE/PARTS],
             uint8_t frame_out[IMG_SIZE/PARTS],
			 float bgmodel[4*BGM_SIZE/PARTS],
			 const data_t learningRate);
