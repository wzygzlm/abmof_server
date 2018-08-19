/**
 * OpenCV video streaming over TCP/IP
 * Client: Receives video from server and display it
 * by Steve Tuenkam
 */

#include "opencv2/opencv.hpp"
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

using namespace cv;


int main(int argc, char** argv)
{

    //--------------------------------------------------------
    //networking stuff: socket , connect
    //--------------------------------------------------------
    int         sokt;
    char*       serverIP;
    int         serverPort;

    if (argc < 3) {
           std::cerr << "Usage: cv_video_cli <serverIP> <serverPort> " << std::endl;
    }

    serverIP   = argv[1];
    serverPort = atoi(argv[2]);

    struct  sockaddr_in serverAddr;
    socklen_t           addrLen = sizeof(struct sockaddr_in);

    if ((sokt = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket() failed" << std::endl;
    }

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_port = htons(serverPort);

    if (connect(sokt, (sockaddr*)&serverAddr, addrLen) < 0) {
        std::cerr << "connect() failed!" << std::endl;
    }



    //----------------------------------------------------------
    //OpenCV Code
    //----------------------------------------------------------

    Mat img, img_color;
    img = Mat::zeros(180 , 240, CV_8UC1);    
    int imgSize = img.total() * img.elemSize();
    uchar *iptr = img.data;
    int bytes = 0;
    int key;

    //make img continuos
    if ( ! img.isContinuous() ) { 
          img = img.clone();
    }
        
    std::cout << "Image Size:" << imgSize << std::endl;


    namedWindow("Event slice Client", CV_WINDOW_NORMAL);
    // resizeWindow("Event sice Client", img.rows * 3,  img.cols * 3);

    while (key != 'q') {

        double minIntensity, maxIntensity;

        if ((bytes = recv(sokt, iptr, imgSize , MSG_WAITALL)) == -1) {
            std::cerr << "recv failed, received bytes = " << bytes << std::endl;
        }

        minMaxLoc(img, &minIntensity, &maxIntensity);
        cvtColor(img, img_color, COLOR_GRAY2BGR);

        for (int  row = 0; row < 180; row++)
        {
            for (int col = 0; col < 240; col++)
            {
                int index = row * 240 + col;
                if(iptr[index] > 0 && iptr[index] < 127)
                {
                    img_color.at<Vec3b>(row, col)[0] = 0;
                    img_color.at<Vec3b>(row, col)[1] = 0 ;
                    img_color.at<Vec3b>(row, col)[2] = 255;
                    // iptr[index] = (uchar)(iptr[i]/maxIntensity*255.0);
                }
                else if(iptr[index] >= 127)
                {
                    img_color.at<Vec3b>(row, col)[0] = 0;
                    img_color.at<Vec3b>(row, col)[1] = 255 ;
                    img_color.at<Vec3b>(row, col)[2] = 0;
                }
            }
        }
        
        cv::imshow("Event slice Client", img_color); 
      
        // if (key = cv::waitKey(10) >= 0) break;
        if (key = cv::waitKey(10) >= 0);
    }   

    close(sokt);

    return 0;
}	
