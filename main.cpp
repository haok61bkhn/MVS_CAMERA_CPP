#include "hikrobot_camera.h"
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <signal.h>

volatile bool g_running = true;

void SignalHandler(int signal) {
  std::cout << "\nReceived signal " << signal << ", stopping..." << std::endl;
  g_running = false;
}

void PrintCameraList(const std::vector<CameraInfo>& camera_list) {
    std::cout << "\n=== Available Cameras ===" << std::endl;
    if (camera_list.empty()) {
        std::cout << "No cameras found!" << std::endl;
        return;
    }
    
    for (const auto& camera : camera_list) {
        std::cout << "Camera ID: " << camera.camera_id 
                  << " | Name: " << camera.camera_name 
                  << " | Type: " << camera.device_type << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

int main() {
  // signal(SIGINT, SignalHandler);
  // signal(SIGTERM, SignalHandler);
  
  std::cout << "Getting camera list..." << std::endl;
  std::vector<CameraInfo> cameras = HikCamera::GetCameraList();
  
  if (cameras.empty()) {
    std::cout << "No cameras found!" << std::endl;
    return -1;
  }
  
  std::cout << "Found " << cameras.size() << " camera(s):" << std::endl;
  for (const auto& camera : cameras) {
    std::cout << "Index: " << camera.camera_id 
              << ", Name: " << camera.camera_name
              << ", Serial: " << camera.camera_serial
              << ", Type: " << camera.device_type << std::endl;
  }
  
  HikCamera hikrobot_camera;
  
  if (!cameras.empty()) {
    std::string target_serial = cameras[0].camera_serial;
    std::cout << "\nConnecting to camera with serial: " << target_serial << std::endl;
    if (hikrobot_camera.ConnectBySerial(target_serial)) {      
      cv::namedWindow("HIKROBOT Camera", cv::WINDOW_AUTOSIZE);
      cv::Mat frame;
      while (g_running && hikrobot_camera.IsConnected()) {
        if (hikrobot_camera.Read(frame)) {
          if (!frame.empty()) {
            cv::imshow("HIKROBOT Camera", frame);
          }
        }
        
        if (cv::waitKey(1) == 27) {
          std::cout << "ESC key pressed, stopping..." << std::endl;
          break;
        }
        
  
      }
      hikrobot_camera.Disconnect();
      cv::destroyAllWindows();
    } else {
      std::cout << "Failed to connect to camera" << std::endl;
    }
  }
  
  return 0;
} 