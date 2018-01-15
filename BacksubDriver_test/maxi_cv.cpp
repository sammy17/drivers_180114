#include "drivers/xbgsub.h"
//#include "drivers/xfeature.h"

#include "include/xparameters.h"
#include <chrono>
#include <string.h>
#include <fstream>
#include <iostream>

// #include "detection/MyTypes.h"
#include "detection/NodeClient.h"
#include "detection/MyTypes.h"
#include "detection/BGSDetector.h"
#include <csignal>

#define RGB_TX_BASE_ADDR 0x15000000
#define TX_BASE_ADDR 0x01000000
#define DDR_RANGE 0x01000000
#define RX_BASE_ADDR 0x02000000

#define AXILITES_BASEADDR 0x43C00000
#define CRTL_BUS_BASEADDR 0x43C10000
#define AXILITE_RANGE 0xFFFF

#define M_AXI_BOUNDING 0x21000000
#define M_AXI_FEATUREH 0x29000000

#define BG_MODEL 0x10000000


using namespace cv;
using namespace std;


XBgsub backsub;
//XFeature feature;

int fdIP;
int fd; // A file descriptor to the video device
int type;
// uint8_t * ybuffer = new uint8_t[N];

uint8_t * src; 
uint8_t * dst; 

int backsub_init(XBgsub * backsub_ptr){
    backsub_ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_BGSUB_0_S_AXI_AXILITES_BASEADDR);
    backsub_ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_XBGSUB_0_S_AXI_CRTL_BUS_BASEADDR);
    backsub_ptr->IsReady = XIL_COMPONENT_IS_READY;
    return 0;
}

void backsub_rel(XBgsub * backsub_ptr){
    munmap((void*)backsub_ptr->Axilites_BaseAddress, AXILITE_RANGE);
    munmap((void*)backsub_ptr->Crtl_bus_BaseAddress, AXILITE_RANGE);
}

void backsub_config(bool ini) {
    XBgsub_Set_frame_in(&backsub,(u32)TX_BASE_ADDR);
    XBgsub_Set_frame_out(&backsub,(u32)RX_BASE_ADDR);
    XBgsub_Set_init(&backsub, ini);
    XBgsub_Set_bgmodel(&backsub, (u32)BG_MODEL);
}


void print_config() {
    printf("Is Ready = %d \n", XBgsub_IsReady(&backsub));
    printf("Frame in = %X \n", XBgsub_Get_frame_in(&backsub));
    printf("Frame out = %X \n", XBgsub_Get_frame_out(&backsub));
    printf("Init = %d \n", XBgsub_Get_init(&backsub));
}


void signalHandler( int signum ) {
    cout << "Interrupt signal (" << signum << ") received.\n";

    // cleanup and close up stuff here
    // terminate program

    //Release IP Core
    backsub_rel(&backsub);
    // feature_rel(&feature);

    munmap((void*)src, DDR_RANGE);
    munmap((void*)dst, DDR_RANGE);
    // munmap((void*)m_axi_bound, 80);
    // munmap((void*)m_axi_feature, 5120*2);

    close(fdIP);

    exit(signum);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, signalHandler);

    // Initialization communication link
    NodeClient client("10.0.0.200",8080);
    client.connect();
    uint16_t frameNo=0;
    const uint8_t cameraID = 0;

    // Initializing IP Core Starts here .........................
    fdIP = open ("/dev/mem", O_RDWR);
    if (fdIP < 1) {
        perror(argv[0]);
        return -1;
    }

    VideoCapture cap("/home/debian/output4.avi");
    cap.set(CV_CAP_PROP_FRAME_WIDTH,320);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,240);
    cap.set(CV_CAP_PROP_FPS,30);
    cap.set(CV_CAP_PROP_CONVERT_RGB,true);


    src = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, TX_BASE_ADDR); 
    dst = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_EXEC|PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RX_BASE_ADDR); 

    if(backsub_init(&backsub)==0) {
        printf("Backsub IP Core Initialized\n");
    }


    auto begin = std::chrono::high_resolution_clock::now();
    bool isFirst = true;
    Mat img, grey;
    
    for (int it=0;it<100000;it++){

        print_config();
        if (isFirst){
            backsub_config(true);
            isFirst = false;
        }
        else{
            backsub_config(false);
        }

        cap>>img;
        cv::cvtColor(img, grey, CV_BGR2GRAY);

        memcpy(src,grey.data,76800);

        XBgsub_Start(&backsub);

        while(!XBgsub_IsDone(&backsub));
        printf("backsub finished\n");

        // printf("Elapsed time Backsub: %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end2-begin2).count());
 
        Mat mask(240, 320, CV_8UC1);
        memcpy(mask.data, dst, 76800); 
        // for (int idxRows = 0; idxRows < 240; idxRows++) {
        //     for (int idxCols = 0; idxCols < 320; idxCols++) {
        //         mask.at<unsigned char>(idxRows, idxCols) = dst[idxRows*320+idxCols];    //.to_uchar();
        //     }
        // }
        // string nm1 = "ipcoreim"+to_string(it)+".jpg";
        // string nm2 = "originim"+to_string(it)+".jpg";

        //imwrite(nm1,mask);
        //imwrite(nm2,receive_image);

        client.sendBinMask(mask);
    }


}
