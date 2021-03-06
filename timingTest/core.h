#include <iostream>
#include <cmath>
#include <cfloat>
#include <string.h>


using namespace std;

#define WIDTH 320
#define HEIGHT 240
#define IMG_SIZE WIDTH*HEIGHT
#define BGM_SIZE IMG_SIZE*2
#define PARTS 120
typedef float data_t;








void bgsub(uint8_t frame_in[IMG_SIZE],
           uint8_t frame_out[IMG_SIZE],
		   bool init,
		   data_t bgmodel[4*BGM_SIZE]);

void process(uint8_t frame_in[IMG_SIZE/PARTS],
             uint8_t frame_out[IMG_SIZE/PARTS],
			 float bgmodel[4*BGM_SIZE/PARTS],
			 const data_t learningRate);
