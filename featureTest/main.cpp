#include "drivers/xfeature.h"
#include "include/xparameters.h"

#include <opencv2/opencv.hpp>
#include <chrono>
#include <string.h>
#include <fstream>
#include <iostream>

#define IMG_H 240
#define IMG_W 320

#define M_AXI_BOUNDING_0 0x21000000
#define M_AXI_FEATUREH_0 0x29000000

#define RX_BASE_ADDR 0x12000000
#define RGB_TX_BASE_ADDR 0x15000000

#define AXILITE_RANGE 0xFFFF
#define DDR_RANGE 0x01000000

using namespace cv;
using namespace std;

int fdIP;
int fd; // A file descriptor to the video device
int type;

XFeature feature0;

uint8_t * rgb_src;

uint16_t * m_axi_feature0;
uint16_t * m_axi_bound0;


int feature_init(XFeature * ptr){
    ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_0_S_AXI_AXILITES_BASEADDR);
    ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_0_S_AXI_CRTL_BUS_BASEADDR);
    ptr->IsReady = XIL_COMPONENT_IS_READY;
    return 0;
}

void feature_config() {
    printf("config\n");
    XFeature_Set_frame_in(&feature0,(u32)RGB_TX_BASE_ADDR);
    XFeature_Set_bounding(&feature0,(u32)M_AXI_BOUNDING_0);
    XFeature_Set_featureh(&feature0,(u32)M_AXI_FEATUREH_0);
}

void feature_rel(XFeature * ptr){
    munmap((void*)ptr->Crtl_bus_BaseAddress, AXILITE_RANGE);
    munmap((void*)ptr->Axilites_BaseAddress, AXILITE_RANGE);
}

void signalHandler( int signum ) {
    cout << "Interrupt signal (" << signum << ") received.\n";

    feature_rel(&feature0);

    munmap((void*)rgb_src, DDR_RANGE);
    munmap((void*)m_axi_bound0, 8);
    munmap((void*)m_axi_feature0, 512*2);

    close(fdIP);

    exit(signum);
}


uint16_t featureHist[512];


int main(int argc, char *argv[]) {

	fdIP = open ("/dev/mem", O_RDWR);
    if (fdIP < 1) {
        perror(argv[0]);
        return -1;
    }

    printf("mmap begin\n");
    rgb_src = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RGB_TX_BASE_ADDR); 

    m_axi_bound0 = (uint16_t*)mmap(NULL, 8,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_0);
    m_axi_feature0 = (uint16_t*)mmap(NULL, 512*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_0);

	Mat im = imread("testim.jpg",IMREAD_COLOR);

	memcpy(rgb_src, im.data, sizeof(uint8_t)*76800*3);

	if(feature_init(&feature0)==0) {
        printf("Feature 0 IP Core Initialized\n");
    }

    feature_config();
    uint16_t m_axi_bound0_sw[4];

    m_axi_bound0_sw[0] = 64;
    m_axi_bound0_sw[1] = 64;
    m_axi_bound0_sw[2] = 128;
    m_axi_bound0_sw[3] = 128;

    for (int y=0;y<512;y++){
		m_axi_feature0[y] = 0;
		featureHist[y]= 0;
	}
    memcpy(m_axi_bound0,m_axi_bound0_sw,8);

    XFeature_Start(&feature0);
    while(!XFeature_IsDone(&feature0));

    printf("Feature done\n");

    int index1 = 0;
	int iterator = 0;

	for (int i = 0; i < IMG_H; i++) {
		for (int j = 0; j < IMG_W; j++) {
			for (int h = 0; h < 1; h++) {
				if ((m_axi_bound0_sw[0] <= i)
						&& (m_axi_bound0_sw[1] <= j)
						&& (m_axi_bound0_sw[2] >= i)
						&& (m_axi_bound0_sw[3] >= j)) {

					index1 = h * 512 + 64 * (im.data[iterator + 2] >> 5) + 8 * (im.data[iterator + 1] >> 5) + (im.data[iterator + 0] >> 5);
					//printf("Index i=%d, j=%d : %d\n",i,j,index1);
					featureHist[index1] += 1;

				}
			}
			iterator += 3;

		}
	}

	for (int x = 0;x < 512; x++){
		if (m_axi_feature0[x] != featureHist[x]){
			printf("Mismatch %d, expected : %d, actual : %d\n",x,featureHist[x],m_axi_feature0[x]);
		}
	}

    feature_rel(&feature0);

    munmap((void*)rgb_src, DDR_RANGE);
    munmap((void*)m_axi_bound0, 8);
    munmap((void*)m_axi_feature0, 512*2);

    close(fdIP);

	return 0;
}
