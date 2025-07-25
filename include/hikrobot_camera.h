#ifndef HIKROBOT_CAMERA_H
#define HIKROBOT_CAMERA_H

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <vector>
#define IMAGE_PATH "data/image.jpg"

#ifndef COMPILE_WITHOUT_MVS_SDK
#include "CameraParams.h"
#include "MvCameraControl.h"
#include "MvErrorDefine.h"
#endif

struct CameraInfo {
  int camera_id;
  std::string camera_name;
  std::string camera_serial;
  std::string device_type;
};

class HikCamera {
public:
  HikCamera();
  ~HikCamera();

  static std::vector<CameraInfo> GetCameraList();
  bool Connect(int camera_id);
  bool ConnectBySerial(const std::string &camera_serial);
  bool Read(cv::Mat &frame);
  void Disconnect();
  bool IsConnected() const;

private:
  static void *WorkerThread(void *param);
  void InitializeCamera();
  void CleanupCamera();

#ifndef COMPILE_WITHOUT_MVS_SDK
  static bool GetDeviceInfo(MV_CC_DEVICE_INFO *device_info,
                            CameraInfo &camera_info);
#endif

  void *camera_handle;
  pthread_t worker_thread_id;
  pthread_mutex_t frame_mutex;

  cv::Mat current_frame;
  bool frame_available;
  bool is_connected;
  bool thread_running;
  int connected_camera_id;
  std::string connected_camera_serial;

  static const int MAX_IMAGE_DATA_SIZE = 4 * 2048 * 3072;
};

#endif
