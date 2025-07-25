#include <iostream>
#include <opencv2/opencv.hpp>
#include <signal.h>
#include <chrono>
#include <thread>
#include "hikrobot_camera.hpp"

using namespace std;
using namespace cv;

bool running = true;

void signalHandler(int signum) {
    cout << "\nReceived signal " << signum << ", shutting down gracefully..." << endl;
    running = false;
}

int main(int argc, char **argv)
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    int camera_index = 0;
    if (argc > 1) {
        camera_index = atoi(argv[1]);
        if (camera_index < 0) {
            cout << "Invalid camera index: " << camera_index << endl;
            return -1;
        }
    }

    cout << "Initializing HIKROBOT camera " << camera_index << "..." << endl;
    
    try {
        camera::Camera MVS_cap(camera_index);
        
        cv::namedWindow("HIKROBOT Camera", cv::WINDOW_AUTOSIZE);
        cout << "Camera initialized successfully!" << endl;
        cout << "Press 'q' or ESC to quit, 's' to save current frame" << endl;
        
        cv::Mat src;
        int frame_count = 0;
        
        while (running)
        {
            MVS_cap.ReadImg(src);
            
            if (src.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            frame_count++;
            
            cv::imshow("HIKROBOT Camera", src);
            
            char key = cv::waitKey(1) & 0xFF;
            if (key == 'q' || key == 27) {
                cout << "Exit requested by user." << endl;
                break;
            }
            else if (key == 's') {
                string filename = "hikrobot_frame_" + to_string(frame_count) + ".jpg";
                cv::imwrite(filename, src);
                cout << "Saved frame to " << filename << endl;
            }
        }
        
        cout << "Captured " << frame_count << " frames total." << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }
    
    cv::destroyAllWindows();
    cout << "HIKROBOT Camera application terminated." << endl;
    
    return 0;
}
