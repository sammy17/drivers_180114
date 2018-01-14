#include "drivers/xbgsub.h"
#include "drivers/xfeature.h"

#include "include/xparameters.h"
#include <chrono>
#include <string.h>
#include <fstream>
#include <iostream>

// #include "detection/MyTypes.h"
#include "detection/ClientUDP.h"
// #include "detection/MyTypes.h"
#include "detection/BGSDetector.h"
#include <csignal>

#define RGB_TX_BASE_ADDR 0x15000000
#define TX_BASE_ADDR 0x11000000
#define RX_BASE_ADDR 0x12000000
#define BG_MODEL 0x14000000
#define DDR_RANGE 0x01000000
#define RX_BASE_ADDR 0x02000000

#define AXILITES_BASEADDR 0x43C00000
#define CRTL_BUS_BASEADDR 0x43C10000
#define AXILITE_RANGE 0xFFFF

#define M_AXI_BOUNDING_0 0x21000000
#define M_AXI_FEATUREH_0 0x29000000

#define M_AXI_BOUNDING_1 0x21100000
#define M_AXI_FEATUREH_1 0x29100000

#define M_AXI_BOUNDING_2 0x21200000
#define M_AXI_FEATUREH_2 0x29200000


using namespace cv;
using namespace std;


/***************** Global Variables *********************/


XBgsub backsub;
XFeature feature0;
XFeature feature1;
XFeature feature2;

int fdIP;
int fd; // A file descriptor to the video device
int type;
// uint8_t * ybuffer = new uint8_t[N];

uint8_t * src; 
uint8_t * rgb_src; 
uint8_t * dst; 

uint16_t * m_axi_feature;
uint16_t * m_axi_bound0;
uint16_t * m_axi_feature0;
uint16_t * m_axi_bound1;
uint16_t * m_axi_feature1;
uint16_t * m_axi_bound2;
uint16_t * m_axi_feature2;



int feature_init(XFeature * ptr, int n){
    if (n==0){
        ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_0_S_AXI_AXILITES_BASEADDR);
        ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_0_S_AXI_CRTL_BUS_BASEADDR);
        ptr->IsReady = XIL_COMPONENT_IS_READY;
    }
    else if (n==1){
        ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_1_S_AXI_AXILITES_BASEADDR);
        ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_1_S_AXI_CRTL_BUS_BASEADDR);
        ptr->IsReady = XIL_COMPONENT_IS_READY;
    }
    else if (n==2){
        ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_2_S_AXI_AXILITES_BASEADDR);
        ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_2_S_AXI_CRTL_BUS_BASEADDR);
        ptr->IsReady = XIL_COMPONENT_IS_READY;
    }
    return 0;
}

void feature_rel(XFeature * ptr){
    munmap((void*)ptr->Crtl_bus_BaseAddress, AXILITE_RANGE);
    munmap((void*)ptr->Axilites_BaseAddress, AXILITE_RANGE);
}

void feature_config() {
    printf("config\n");
    // if (n==0){
    XFeature_Set_frame_in(&feature0,(u32)RGB_TX_BASE_ADDR);
    XFeature_Set_bounding(&feature0,(u32)M_AXI_BOUNDING_0);
    XFeature_Set_featureh(&feature0,(u32)M_AXI_FEATUREH_0);
    // }
    // else if (n==1){
    XFeature_Set_frame_in(&feature1,(u32)RGB_TX_BASE_ADDR);
    XFeature_Set_bounding(&feature1,(u32)M_AXI_BOUNDING_1);
    XFeature_Set_featureh(&feature1,(u32)M_AXI_FEATUREH_1);
    // }
    // else if (n==2){
    XFeature_Set_frame_in(&feature2,(u32)RGB_TX_BASE_ADDR);
    XFeature_Set_bounding(&feature2,(u32)M_AXI_BOUNDING_2);
    XFeature_Set_featureh(&feature2,(u32)M_AXI_FEATUREH_2);
    // }
}



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
    XBacksub_Set_frame_in(&backsub,(u32)TX_BASE_ADDR);
    XBacksub_Set_frame_out(&backsub,(u32)RX_BASE_ADDR);
    XBacksub_Set_init(&backsub, ini);
    XBacksub_Set_bgmodel(&backsub, (u32)BG_MODEL);
}

void print_config() {
    printf("Is Ready = %d \n", XBacksub_IsReady(&backsub));
    printf("Frame in = %X \n", XBacksub_Get_frame_in(&backsub));
    printf("Frame out = %X \n", XBacksub_Get_frame_out(&backsub));
    printf("Init = %d \n", XBacksub_Get_init(&backsub));
}


void signalHandler( int signum ) {
    cout << "Interrupt signal (" << signum << ") received.\n";

    // cleanup and close up stuff here
    // terminate program

    //Release IP Core
    backsub_rel(&backsub);
    feature_rel(&feature0);
    feature_rel(&feature1);
    feature_rel(&feature2);

    munmap((void*)src, DDR_RANGE);
    munmap((void*)dst, DDR_RANGE);
    munmap((void*)rgb_src, DDR_RANGE);
    munmap((void*)m_axi_bound0, 2);
    munmap((void*)m_axi_feature0, 512*2);
    munmap((void*)m_axi_bound1, 2);
    munmap((void*)m_axi_feature1, 512*2);
    munmap((void*)m_axi_bound2, 2);
    munmap((void*)m_axi_feature2, 512*2);

    close(fdIP);

    exit(signum);
}


int main(int argc, char *argv[]) {

    signal(SIGINT, signalHandler);

    // Initialization communication link
    boost::asio::io_service io_service;
    ClientUDP client(io_service,"10.0.0.200",8080);
    uint16_t frameNo=0;
    const uint8_t cameraID = 0;

    // Initializing IP Core Starts here .........................
    fdIP = open ("/dev/mem", O_RDWR);
    if (fdIP < 1) {
        perror(argv[0]);
        return -1;
    }

    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH,320);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,240);
    cap.set(CV_CAP_PROP_FPS,30);
    cap.set(CV_CAP_PROP_CONVERT_RGB,true);
   // cap.set(CV_CAP_PROP_AUTOFOCUS, 0);


    src = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, TX_BASE_ADDR); 
    rgb_src = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RGB_TX_BASE_ADDR); 
    dst = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_EXEC|PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RX_BASE_ADDR); 


    m_axi_bound0 = (uint16_t*)mmap(NULL, 80,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_0);
    m_axi_feature0 = (uint16_t*)mmap(NULL, 5120*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_0);

    m_axi_bound1 = (uint16_t*)mmap(NULL, 80,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_1);
    m_axi_feature1 = (uint16_t*)mmap(NULL, 5120*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_1);

    m_axi_bound2 = (uint16_t*)mmap(NULL, 80,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_2);
    m_axi_feature2 = (uint16_t*)mmap(NULL, 5120*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_2);


    if(backsub_init(&backsub)==0) {
        printf("Backsub IP Core Initialized\n");
    }

    if(feature_init(&feature0,0)==0) {
        printf("Feature 0 IP Core Initialized\n");
    }
    if(feature_init(&feature1,1)==0) {
        printf("Feature 1 IP Core Initialized\n");
    }
    if(feature_init(&feature2,2)==0) {
        printf("Feature 2 IP Core Initialized\n");
    }
    // Initializing IP Core Ends here .........................

    
    BGSDetector detector(30,
                          BGS_HW,
                          false,
                          "./pca_coeff.xml",
                          false);

    /***************************** Begin looping here *********************/
//    auto begin = std::chrono::high_resolution_clock::now();
    bool isFirst = true;
    bool isSecond = false;
    Mat img, grey;


    for (;;){
        // Queue the buffer
        //auto begin = std::chrono::high_resolution_clock::now();

        if (isFirst){
            backsub_config(true);
            isFirst = false;
            isSecond = true;
        }
        if (isSecond){
            backsub_config(false);
            isSecond = false;
        }
	auto begin = std::chrono::high_resolution_clock::now();
        cap>>img;
	auto begin2 = std::chrono::high_resolution_clock::now();
	if(!img.data) break;
        cv::cvtColor(img, grey, CV_BGR2GRAY);
        memcpy(rgb_src,img.data,76800*3);
        memcpy(src,grey.data,76800);

        //auto begin2 = std::chrono::high_resolution_clock::now();

        XBacksub_Start(&backsub);

        while(!XBacksub_IsDone(&backsub));

        auto end2 = std::chrono::high_resolution_clock::now();

        Mat mask = Mat(240, 320, CV_8UC1, dst); 

        std::vector<cv::Rect> detections = detector.detect(mask);

            int len = detections.size();
            if (len>10){
                len = 10;
            }
            int det =0;
            memset(m_axi_bound0,0,2); // initialize bounds to 0
            memset(m_axi_bound1,0,2);
            memset(m_axi_bound2,0,2);
            feature_config();
            auto end3 = std::chrono::high_resolution_clock::now();
            while(true){
                if (det < len){
                    m_axi_bound0[0] = detections.at(det).x;
                    m_axi_bound0[1] = detections.at(det).y;
                    m_axi_bound0[2] = detections.at(det).x + detections.at(k).width;
                    m_axi_bound0[3] = detections.at(det).y + detections.at(k).height;
                    det++;
                    XFeature_Start(&feature0);
                    while(!XFeature_IsDone(&feature0));
                    memcpy(&m_axi_feature[512*det],m_axi_feature0,512*2);
                }else {
                    break;
                }
                if (det < len){
                    m_axi_bound1[0] = detections.at(det).x;
                    m_axi_bound1[1] = detections.at(det).y;
                    m_axi_bound1[2] = detections.at(det).x + detections.at(k).width;
                    m_axi_bound1[3] = detections.at(det).y + detections.at(k).height;
                    det++;
                    XFeature_Start(&feature1);
                    while(!XFeature_IsDone(&feature1));
                    memcpy(&m_axi_feature[512*det],m_axi_feature1,512*2);
                }else {
                    break;
                }
                if (det < len){
                    m_axi_bound2[0] = detections.at(det).x;
                    m_axi_bound2[1] = detections.at(det).y;
                    m_axi_bound2[2] = detections.at(det).x + detections.at(k).width;
                    m_axi_bound2[3] = detections.at(det).y + detections.at(k).height;
                    det++;
                    XFeature_Start(&feature2);
                    while(!XFeature_IsDone(&feature2));
                    memcpy(&m_axi_feature[512*det],m_axi_feature2,512*2);
                }else {
                    break;
                }

            }
            //printf("Detection Length: %d",len);

            // for (int k=0;k<len;k++){
            //     m_axi_bound[k*4+0] = detections.at(k).x;
            //     m_axi_bound[k*4+1] = detections.at(k).y;
            //     m_axi_bound[k*4+2] = detections.at(k).x + detections.at(k).width;
            //     m_axi_bound[k*4+3] = detections.at(k).y + detections.at(k).height;
            //     //printf("testloop %d \n",k);
            // }
	
        
        // XFeature_Start(&feature);
        
        // while(!XFeature_IsDone(&feature));
	auto end4 = std::chrono::high_resolution_clock::now();


        Frame frame;
        frame.frameNo = frameNo;
        frame.cameraID = cameraID;
        frame.detections.clear();
        frame.histograms.clear();
        for(int q=0;q<len;q++)
        {
            BoundingBox bbox;
            bbox.x = detections[q].x;
            bbox.y = detections[q].y;
            bbox.width = detections[q].width;
            bbox.height = detections[q].height;
            frame.detections.push_back(bbox);

            vector<uint16_t> histogram(512);
      
            std::copy ( m_axi_feature+512*q, m_axi_feature+512*(q+1), histogram.begin() );
            frame.histograms.push_back(histogram);
        }
        frameNo++;
        frame.setMask(detector->mask);
        frame.set_now();
        client.send(frame);

        // outFile.close();
        auto end = std::chrono::high_resolution_clock::now();
        // printf("Elapsed time : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count());

        // char c=getch();
        // if (c=='q')
        //   break;
	printf("Elapsed time capture : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(begin2-begin).count());
	printf("Elapsed time backsub : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end2-begin2).count());
	printf("Elapsed time opencv  : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end3-end2).count());
	printf("Elapsed time feature : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end4-end3).count());
	printf("Elapsed time send    : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end-end4).count());
	printf("Elapsed time tital   : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count());
    
}


    //Release IP Core
    backsub_rel(&backsub);
    feature_rel(&feature0);
    feature_rel(&feature1);
    feature_rel(&feature2);

    munmap((void*)src, DDR_RANGE);
    munmap((void*)dst, DDR_RANGE);
    munmap((void*)rgb_src, DDR_RANGE);
    munmap((void*)m_axi_bound0, 2);
    munmap((void*)m_axi_feature0, 512*2);
    munmap((void*)m_axi_bound1, 2);
    munmap((void*)m_axi_feature1, 512*2);
    munmap((void*)m_axi_bound2, 2);
    munmap((void*)m_axi_feature2, 512*2);

    close(fdIP);
     
    printf("Device unmapped\n");

    return 0;
}

