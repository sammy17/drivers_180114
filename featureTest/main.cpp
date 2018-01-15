#include "drivers/xfeature.h"
#include "include/xparameters.h"

#include <opencv2/opencv.hpp>
#include <chrono>
#include <string.h>
#include <fstream>
#include <iostream>

#define M_AXI_BOUNDING_0 0x21000000
#define M_AXI_FEATUREH_0 0x29000000

#define RX_BASE_ADDR 0x12000000
#define RGB_TX_BASE_ADDR 0x15000000

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

	Mat im = imread("testim.jpg",CV_LOAD_IMAGE_GREY);

	memcpy(rgb_src, im.data, sizeof(uint8_t)*76800);

	if(feature_init(&feature0)==0) {
        printf("Feature 0 IP Core Initialized\n");
    }

    feature_config();

    m_axi_bound0[0] = 64;
    m_axi_bound0[1] = 64;
    m_axi_bound0[2] = 128;
    m_axi_bound0[3] = 128;

    XFeature_Start(&feature0);
    while(!XFeature_IsDone(&feature0));

    printf("Feature done\n");

	return 0;
}
