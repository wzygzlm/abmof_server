/**
 * OpenCV video streaming over UDP
 * Client: Receives video from server and display it
 * by Steve Tuenkam
 */

#include "opencv2/opencv.hpp"
#include <iostream>
#include <math.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include "motion_field.h"

using namespace cv;

#define PI 3.14159265

int main(int argc, char** argv)
{

    //--------------------------------------------------------
    //networking stuff: socket , connect
    //--------------------------------------------------------
    int         sokt;
    char*       serverIP;
    int         serverPort;

    //creat color map
    int colormap[7][7][3];
    for(int i = -3; i < 4; i++)
    {
        for(int j = -3; j < 4; j++)
        {
        double parammf, resultmf;
        //param = (double(j)/double(i))+0.001;
        resultmf = (atan2(double(j),double(i))+0.01) * 180 / PI;
        resultmf = resultmf + 179;
        HSV data = HSV(resultmf, 0.9, 0.9);

        RGB valuemf = HSVToRGB(data);

        colormap[i+3][j+3][0] = valuemf.B;
        colormap[i+3][j+3][1] = valuemf.G;
        colormap[i+3][j+3][2] = valuemf.R;
    }
    }   
    colormap[3][3][0] =  255;
    colormap[3][3][1] =  255;
    colormap[3][3][2] =  255;

    if (argc < 3) {
           std::cerr << "Usage: cv_video_cli <serverIP> <serverPort> " << std::endl;
    }

    serverIP   = argv[1];
    serverPort = atoi(argv[2]);

    struct  sockaddr_in serverAddr;
    socklen_t           addrLen = sizeof(struct sockaddr_in);

    if ((sokt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        std::cerr << "socket() failed" << std::endl;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_port = htons(serverPort);

    // if (connect(sokt, (sockaddr*)&serverAddr, addrLen) < 0) {
    //     std::cerr << "connect() failed!" << std::endl;
    //}



    //----------------------------------------------------------
    //OpenCV Code
    //----------------------------------------------------------

    Mat img, img_color, img_resize;
    img = Mat::ones(180 , 240, CV_8UC1)*127;;    
    int imgSize = img.total() * img.elemSize();
    uchar *iptr = img.data;
    int bytes = 0;
    int key;
    int scalsz = 3;

    //make img continuos
    if ( ! img.isContinuous() ) { 
          img = img.clone();
    }
        
    std::cout << "Image Size:" << imgSize << std::endl;


    namedWindow("Event slice Client", CV_WINDOW_NORMAL);
    // resizeWindow("Event sice Client", img.rows * 3,  img.cols * 3);

    // Send an simple message to start the connection.
    char message[] = "OK.";
    if ((bytes = sendto(sokt, message, sizeof(message) , MSG_WAITALL, (struct sockaddr *) &serverAddr, addrLen)) == -1) {
        std::cerr << "send failed, received bytes = " << bytes << std::endl;
    }

    char recvBuf[imgSize];

    while (key != 'q') {

        double minIntensity, maxIntensity;

        if ((bytes = recvfrom(sokt, recvBuf, imgSize , MSG_WAITALL, (struct sockaddr *) &serverAddr, &addrLen)) == -1) {
            std::cerr << "recv failed, received bytes = " << bytes << std::endl;
        }

        printf("Received %d data from the server.\n", bytes);
        printf("The first four bytes are %x, %x, %x, %x.\n", (uchar)recvBuf[0], (uchar)recvBuf[1], (uchar)recvBuf[2], (uchar)recvBuf[3]);
        minMaxLoc(img, &minIntensity, &maxIntensity);
        printf("The maximum intensity is %f.\n", maxIntensity);

        // Reset image
        // img = Mat::zeros(img.size(), img.type());

        cvtColor(img, img_color, COLOR_GRAY2BGR);
        cv::resize(img_color, img_resize, cv::Size(), scalsz, scalsz);

        // start reading from the fourth bytes. The first bytes are used to store some debug information from the server.
        for(int bufIndex = 4; bufIndex  < bytes; bufIndex = bufIndex + 4)
        {
            uchar x = recvBuf[bufIndex];
            uchar y = recvBuf[bufIndex + 1];
            uchar pol = recvBuf[bufIndex + 2] & 0x01; // The last bit of the third bytes is polarity.
            bool isCorner = recvBuf[bufIndex + 3] & 0x80; // The highest bit is the corner result

            // Only print once
            if (bufIndex%4 == 0 && isCorner) printf("Current event is a corner.\n");

            if(pol == 1)
            {
                for(int i = 0; i < scalsz; i++)
                {
                    for(int j = 0; j < scalsz; j++)
                    {
                        img_resize.at<Vec3b>(y*scalsz+i, x*scalsz+j)[0] = 255;
                        img_resize.at<Vec3b>(y*scalsz+i, x*scalsz+j)[1] = 255;
                        img_resize.at<Vec3b>(y*scalsz+i, x*scalsz+j)[2] = 255;
                    }
                }
                //img_color.at<Vec3b>(y, x)[0] = 255;
                //img_color.at<Vec3b>(y, x)[1] = 255;
                //img_color.at<Vec3b>(y, x)[2] = 255;
            }
            else
            {
                for(int i = 0; i < scalsz; i++)
                {
                    for(int j = 0; j < scalsz; j++)
                    {
                        img_resize.at<Vec3b>(y*scalsz+i, x*scalsz+j)[0] = 0;
                        img_resize.at<Vec3b>(y*scalsz+i, x*scalsz+j)[1] = 0;
                        img_resize.at<Vec3b>(y*scalsz+i, x*scalsz+j)[2] = 0;
                    }
                }                
                //img_color.at<Vec3b>(y, x)[0] = 0;
                //img_color.at<Vec3b>(y, x)[1] = 0;
                //img_color.at<Vec3b>(y, x)[2] = 0;
            }

            /// Drawing a circle around corners
            if( isCorner )
            {
                circle( img_resize, Point( x, y ), 5,  Scalar(0), 2, 8, 0 );
            }

        }

        cv::imshow("Event slice Client", img_resize); 
      
        // if (key = cv::waitKey(10) >= 0) break;
        if (key = cv::waitKey(10) >= 0);
    }   

    close(sokt);

    return 0;
}   
