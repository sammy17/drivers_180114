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

#define RGB_TX_BASE_ADDR  0x30000000
#define MASK_BASE_ADDR    0x30100000
#define TX_BASE_ADDR      0x31000000
#define RX_BASE_ADDR      0x31800000
#define BG_MODEL          0x33000000
#define DDR_RANGE         0x00800000

#define AXILITES_BASEADDR 0x43C00000
#define CRTL_BUS_BASEADDR 0x43C10000
#define AXILITE_RANGE     0x0000FFFF

#define M_AXI_BOUNDING_0  0x35000000
#define M_AXI_FEATUREH_0  0x36000000

#define M_AXI_BOUNDING_1  0x35001000
#define M_AXI_FEATUREH_1  0x36100000

#define M_AXI_BOUNDING_2  0x35002000
#define M_AXI_FEATUREH_2  0x36200000


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
uint8_t * mask_in;

uint16_t m_axi_feature[5120];
uint16_t * m_axi_bound0;
uint16_t * m_axi_feature0;
uint16_t * m_axi_bound1;
uint16_t * m_axi_feature1;
uint16_t * m_axi_bound2;
uint16_t * m_axi_feature2;
uint16_t m_axi_bound0_sw[4];



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
    XFeature_Set_mask_in(&feature0,(u32)MASK_BASE_ADDR);
    // }
    // else if (n==1){
    // XFeature_Set_frame_in(&feature1,(u32)RGB_TX_BASE_ADDR);
    // XFeature_Set_bounding(&feature1,(u32)M_AXI_BOUNDING_1);
    // XFeature_Set_featureh(&feature1,(u32)M_AXI_FEATUREH_1);
    // // }
    // // else if (n==2){
    // XFeature_Set_frame_in(&feature2,(u32)RGB_TX_BASE_ADDR);
    // XFeature_Set_bounding(&feature2,(u32)M_AXI_BOUNDING_2);
    // XFeature_Set_featureh(&feature2,(u32)M_AXI_FEATUREH_2);
    // }
    printf("config end\n");
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
    feature_rel(&feature0);
    // feature_rel(&feature1);
    // feature_rel(&feature2);

    munmap((void*)src, DDR_RANGE);
    munmap((void*)dst, DDR_RANGE);
    munmap((void*)rgb_src, DDR_RANGE);
    munmap((void*)m_axi_bound0, 8);
    munmap((void*)m_axi_feature0, 512*2);
    // munmap((void*)m_axi_bound1, 8);
    // munmap((void*)m_axi_feature1, 512*2);
    // munmap((void*)m_axi_bound2, 8);
    // munmap((void*)m_axi_feature2, 512*2);

    close(fdIP);

    exit(signum);
}


int main(int argc, char *argv[]) {
    signal(SIGINT, signalHandler);

    // Initialization communication link
    boost::asio::io_service io_service;
    ClientUDP client(io_service,"10.10.21.49",8080);
    uint16_t frameNo=0;
    const uint8_t cameraID = 0;

    // Initializing IP Core Starts here .........................
    fdIP = open ("/dev/mem", O_RDWR);
    if (fdIP < 1) {
        perror(argv[0]);
        return -1;
    }

    VideoCapture cap(argv[1]);
    cap.set(CV_CAP_PROP_FRAME_WIDTH,320);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,240);
    cap.set(CV_CAP_PROP_FPS,30);
    cap.set(CV_CAP_PROP_CONVERT_RGB,true);
   // cap.set(CV_CAP_PROP_AUTOFOCUS, 0);

    src = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, TX_BASE_ADDR); 
    rgb_src = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RGB_TX_BASE_ADDR); 
    dst = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_EXEC|PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RX_BASE_ADDR);
    mask_in = (uint8_t*)mmap(NULL, 76800,PROT_EXEC|PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, MASK_BASE_ADDR);


    m_axi_bound0 = (uint16_t*)mmap(NULL, 8,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_0);
    m_axi_feature0 = (uint16_t*)mmap(NULL, 512*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_0);

    // m_axi_bound1 = (uint16_t*)mmap(NULL, 8,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_1);
    // m_axi_feature1 = (uint16_t*)mmap(NULL, 512*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_1);

    // m_axi_bound2 = (uint16_t*)mmap(NULL, 8,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING_2);
    // m_axi_feature2 = (uint16_t*)mmap(NULL, 512*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH_2);
    printf("init begin\n");

    if(backsub_init(&backsub)==0) {
        printf("Backsub IP Core Initialized!\n");
    }

    if(feature_init(&feature0,0)==0) {
        printf("Feature 0 IP Core Initialized!\n");
    }
    // if(feature_init(&feature1,1)==0) {
    //     printf("Feature 1 IP Core Initialized\n");
    // }
    // if(feature_init(&feature2,2)==0) {
    //     printf("Feature 2 IP Core Initialized\n");
    // }
    // Initializing IP Core Ends here .........................

    
    BGSDetector detector(30,
                          BGS_HW,
                          false,
                          "./pca_coeff.xml",
                          false);

    /***************************** Begin looping here *********************/
//    auto begin = std::chrono::high_resolution_clock::now();
    bool isFirst = true;
    Mat img, grey;

    feature_config();
    for (;;){
        // Queue the buffer
        //auto begin = std::chrono::high_resolution_clock::now();

        backsub_config(isFirst);
        if(isFirst) isFirst = false;

	auto begin = std::chrono::high_resolution_clock::now();
        cap>>img;
	auto begin2 = std::chrono::high_resolution_clock::now();
	if(!img.data) break;
        cv::cvtColor(img, grey, CV_BGR2GRAY);
        memcpy(rgb_src,img.data,76800*3);
        memcpy(src,grey.data,76800);
        //auto begin2 = std::chrono::high_resolution_clock::now();

        XBgsub_Start(&backsub);
        while(!XBgsub_IsDone(&backsub));

        auto end2 = std::chrono::high_resolution_clock::now();
        Mat mask = Mat(240, 320, CV_8UC1); 
        memcpy(mask.data,dst,76800);

        std::vector<cv::Rect> detections = detector.detect(mask);
            int len = detections.size();
            if (len>10){
                len = 10;
            }
            int det =0;
            memset(m_axi_bound0_sw,0,8); // initialize bounds to 0
            memset(m_axi_feature, 0, 5120*2);
            // memset(m_axi_feature0, 0, 512*2);
            memcpy(mask_in, detector.shape.data, 76800);
            // if(len>0){
            //     for (int l=0;l<76800;l++){
            //         if(mask_in[l]!=0)
            //             printf("Index %d \n",l);
            //     }
            // }
            // memset(m_axi_bound1,0,8);
            // memset(m_axi_bound2,0,8);         
            auto end3 = std::chrono::high_resolution_clock::now();
            while(true){
                if (det < len){
                    m_axi_bound0_sw[0] = detections.at(det).x;
                    m_axi_bound0_sw[1] = detections.at(det).y;
                    m_axi_bound0_sw[2] = detections.at(det).x + detections.at(det).width;
                    m_axi_bound0_sw[3] = detections.at(det).y + detections.at(det).height;
                    memcpy(m_axi_bound0, m_axi_bound0_sw, 8);
                    det++;
                    // printf("Bounds : %d, %d, %d, %d\n",m_axi_bound0[0],m_axi_bound0[1],m_axi_bound0[2],m_axi_bound0[3]);
                    XFeature_Start(&feature0);
                    while(!XFeature_IsDone(&feature0));
                    memcpy(&m_axi_feature[512*det],m_axi_feature0,512*2);
                    for (int l=0;l<512;l++){
                        if(m_axi_feature0[l]>0)
                            printf("Index %d : %d\n",l,m_axi_feature0[l]);
                    }
                }else {
                    break;
                }
                // if (det < len){
                //     m_axi_bound1[0] = detections.at(det).x;
                //     m_axi_bound1[1] = detections.at(det).y;
                //     m_axi_bound1[2] = detections.at(det).x + detections.at(det).width;
                //     m_axi_bound1[3] = detections.at(det).y + detections.at(det).height;
                //     det++;
                //     XFeature_Start(&feature1);
                //     while(!XFeature_IsDone(&feature1));
                //     memcpy(&m_axi_feature[512*det],m_axi_feature1,512*2);
                // }else {
                //     break;
                // }
                // if (det < len){
                //     m_axi_bound2[0] = detections.at(det).x;
                //     m_axi_bound2[1] = detections.at(det).y;
                //     m_axi_bound2[2] = detections.at(det).x + detections.at(det).width;
                //     m_axi_bound2[3] = detections.at(det).y + detections.at(det).height;
                //     det++;
                //     XFeature_Start(&feature2);
                //     while(!XFeature_IsDone(&feature2));
                //     memcpy(&m_axi_feature[512*det],m_axi_feature2,512*2);
                // }else {
                //     break;
                // }

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
            // frame.histograms.push_back(histogram);
            for (int l=0;l<512;l++){
                if(m_axi_feature[512*q+l]!=detector.histograms[q].at<unsigned short>(l))
                    printf("Mismatch : %d, expected : %d, actual : %d\n",l,detector.histograms[q].at<unsigned short>(l),m_axi_feature[512*q+l]);
            }
            frame.histograms.push_back(detector.histograms[q]);
        }
        frameNo++;
        frame.setMask(detector.mask);
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
	printf("Elapsed time total   : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count());
    
}


    //Release IP Core
    backsub_rel(&backsub);
    feature_rel(&feature0);
    // feature_rel(&feature1);
    // feature_rel(&feature2);

    munmap((void*)src, DDR_RANGE);
    munmap((void*)dst, DDR_RANGE);
    munmap((void*)rgb_src, DDR_RANGE);
    munmap((void*)m_axi_bound0, 8);
    munmap((void*)m_axi_feature0, 512*2);
    // munmap((void*)m_axi_bound1, 8);
    // munmap((void*)m_axi_feature1, 512*2);
    // munmap((void*)m_axi_bound2, 8);
    // munmap((void*)m_axi_feature2, 512*2);

    close(fdIP);
     
    printf("Device unmapped\n");

    return 0;
}

