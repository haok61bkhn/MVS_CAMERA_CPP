#include "hikrobot_camera.h"

HikCamera::HikCamera()
    : camera_handle(nullptr), worker_thread_id(0), frame_available(false),
      is_connected(false), thread_running(false), connected_camera_id(-1),
      connected_camera_serial("") {
  pthread_mutex_init(&frame_mutex, nullptr);
}

HikCamera::~HikCamera() {
  Disconnect();
  pthread_mutex_destroy(&frame_mutex);
}

std::vector<CameraInfo> HikCamera::GetCameraList() {
  std::vector<CameraInfo> camera_list;

#ifdef COMPILE_WITHOUT_MVS_SDK
  CameraInfo demo_camera;
  demo_camera.camera_id = 0;
  demo_camera.camera_name = "Demo Camera (SDK Not Available)";
  demo_camera.camera_serial = "DEMO_0000001";
  demo_camera.device_type = "Virtual";
  camera_list.push_back(demo_camera);
#else
  MV_CC_DEVICE_INFO_LIST device_list;
  memset(&device_list, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

  int ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &device_list);
  if (MV_OK != ret) {
    std::cerr << "Failed to enumerate devices. Error: " << std::hex << ret
              << std::endl;
    return camera_list;
  }

  for (unsigned int i = 0; i < device_list.nDeviceNum; i++) {
    CameraInfo camera_info;
    camera_info.camera_id = i;

    if (GetDeviceInfo(device_list.pDeviceInfo[i], camera_info)) {
      camera_list.push_back(camera_info);
    }
  }
#endif

  return camera_list;
}

bool HikCamera::Connect(int camera_id) {
  if (is_connected) {
    Disconnect();
  }

  connected_camera_id = camera_id;

#ifdef COMPILE_WITHOUT_MVS_SDK
  current_frame = cv::imread(IMAGE_PATH);
  frame_available = true;
  is_connected = true;
  connected_camera_serial = "DEMO_0000001";
  return true;
#else
  MV_CC_DEVICE_INFO_LIST device_list;
  memset(&device_list, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
  int ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &device_list);
  if (MV_OK != ret || camera_id >= (int)device_list.nDeviceNum) {
    std::cerr << "Camera ID " << camera_id << " not found" << std::endl;
    return false;
  }

  CameraInfo camera_info;
  camera_info.camera_id = camera_id;
  if (GetDeviceInfo(device_list.pDeviceInfo[camera_id], camera_info)) {
    connected_camera_serial = camera_info.camera_serial;
  }

  ret = MV_CC_CreateHandle(&camera_handle, device_list.pDeviceInfo[camera_id]);
  if (MV_OK != ret) {
    std::cerr << "Failed to create camera handle. Error: " << std::hex << ret
              << std::endl;
    return false;
  }

  ret = MV_CC_OpenDevice(camera_handle);
  if (MV_OK != ret) {
    std::cerr << "Failed to open device. Error: " << std::hex << ret
              << std::endl;
    MV_CC_DestroyHandle(camera_handle);
    camera_handle = nullptr;
    return false;
  }

  InitializeCamera();

  ret = MV_CC_StartGrabbing(camera_handle);
  if (MV_OK != ret) {
    std::cerr << "Failed to start grabbing. Error: " << std::hex << ret
              << std::endl;
    CleanupCamera();
    return false;
  }

  thread_running = true;
  ret = pthread_create(&worker_thread_id, nullptr, WorkerThread, this);
  if (ret != 0) {
    std::cerr << "Failed to create worker thread" << std::endl;
    thread_running = false;
    CleanupCamera();
    return false;
  }

  is_connected = true;
  return true;
#endif
}

bool HikCamera::ConnectBySerial(const std::string &camera_serial) {
  std::vector<CameraInfo> cameras = GetCameraList();
  
  for (const auto &camera : cameras) {
    if (camera.camera_serial == camera_serial) {
      return Connect(camera.camera_id);
    }
  }
  
  std::cerr << "Camera with serial " << camera_serial << " not found" << std::endl;
  return false;
}

bool HikCamera::Read(cv::Mat &frame) {
  pthread_mutex_lock(&frame_mutex);
  if (!frame_available || current_frame.empty()) {
    pthread_mutex_unlock(&frame_mutex);
    return false;
  }
  frame = current_frame.clone();
  pthread_mutex_unlock(&frame_mutex);
  return true;
}

void HikCamera::Disconnect() {
  if (!is_connected) {
    return;
  }

  thread_running = false;

#ifndef COMPILE_WITHOUT_MVS_SDK
  if (worker_thread_id != 0) {
    pthread_join(worker_thread_id, nullptr);
    worker_thread_id = 0;
  }

  CleanupCamera();
#endif

  pthread_mutex_lock(&frame_mutex);
  current_frame.release();
  frame_available = false;
  pthread_mutex_unlock(&frame_mutex);

  is_connected = false;
  connected_camera_id = -1;
  connected_camera_serial = "";
}

bool HikCamera::IsConnected() const { return is_connected; }

void HikCamera::InitializeCamera() {
#ifndef COMPILE_WITHOUT_MVS_SDK
  // MV_CC_SetEnumValue(camera_handle, "TriggerMode", 0);
  // MV_CC_SetEnumValue(camera_handle, "AcquisitionMode", 2);
  
  // std::cout << "Setting camera to continuous acquisition mode..." << std::endl;

  // MVCC_ENUMVALUE enum_value = {0};
  // int ret = MV_CC_GetEnumValue(camera_handle, "PixelFormat", &enum_value);
  // if (MV_OK == ret) {
  //   std::cout << "Using PixelFormat: 0x" << std::hex << enum_value.nCurValue
  //             << std::endl;
  // }
  
  // ret = MV_CC_GetEnumValue(camera_handle, "TriggerMode", &enum_value);
  // if (MV_OK == ret) {
  //   std::cout << "TriggerMode: " << enum_value.nCurValue << std::endl;
  // }
  
  // ret = MV_CC_GetEnumValue(camera_handle, "AcquisitionMode", &enum_value);
  // if (MV_OK == ret) {
  //   std::cout << "AcquisitionMode: " << enum_value.nCurValue << std::endl;
  // }
#endif
}

void HikCamera::CleanupCamera() {
#ifndef COMPILE_WITHOUT_MVS_SDK
  if (camera_handle) {
    MV_CC_StopGrabbing(camera_handle);
    MV_CC_CloseDevice(camera_handle);
    MV_CC_DestroyHandle(camera_handle);
    camera_handle = nullptr;
  }
#endif
}

void *HikCamera::WorkerThread(void *param) {
  HikCamera *camera = static_cast<HikCamera *>(param);

#ifdef COMPILE_WITHOUT_MVS_SDK
  return nullptr;
#else
  unsigned char *buffer_driver =
      static_cast<unsigned char *>(malloc(MAX_IMAGE_DATA_SIZE));
  unsigned char *buffer_convert =
      static_cast<unsigned char *>(malloc(MAX_IMAGE_DATA_SIZE));

  if (!buffer_driver || !buffer_convert) {
    free(buffer_driver);
    free(buffer_convert);
    return nullptr;
  }

  MV_FRAME_OUT_INFO_EX frame_info = {0};
  MV_CC_PIXEL_CONVERT_PARAM convert_param = {0};
  int consecutive_failures = 0;

  while (camera->thread_running) {
    int ret = MV_CC_GetOneFrameTimeout(camera->camera_handle, buffer_driver,
                                       MAX_IMAGE_DATA_SIZE, &frame_info, 100);

    if (MV_OK != ret) {
      if (++consecutive_failures > 50) {
        std::cerr << "Too many consecutive frame capture failures" << std::endl;
        break;
      }
      usleep(10000);
      continue;
    }

    consecutive_failures = 0;

    convert_param.nWidth = frame_info.nWidth;
    convert_param.nHeight = frame_info.nHeight;
    convert_param.pSrcData = buffer_driver;
    convert_param.nSrcDataLen = frame_info.nFrameLen;
    convert_param.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
    convert_param.pDstBuffer = buffer_convert;
    convert_param.nDstBufferSize = MAX_IMAGE_DATA_SIZE;
    convert_param.enSrcPixelType = frame_info.enPixelType;

    ret = MV_CC_ConvertPixelType(camera->camera_handle, &convert_param);
    if (MV_OK == ret) {
      pthread_mutex_lock(&camera->frame_mutex);
      camera->current_frame = cv::Mat(frame_info.nHeight, frame_info.nWidth,
                                      CV_8UC3, buffer_convert)
                                  .clone();
      camera->frame_available = true;
      pthread_mutex_unlock(&camera->frame_mutex);
    }
  }

  free(buffer_driver);
  free(buffer_convert);
  return nullptr;
#endif
}

#ifndef COMPILE_WITHOUT_MVS_SDK
bool HikCamera::GetDeviceInfo(MV_CC_DEVICE_INFO *device_info,
                              CameraInfo &camera_info) {
  if (!device_info) {
    return false;
  }

  if (device_info->nTLayerType == MV_GIGE_DEVICE) {
    camera_info.device_type = "GigE";
    camera_info.camera_name = std::string(reinterpret_cast<const char *>(
        device_info->SpecialInfo.stGigEInfo.chUserDefinedName));
    camera_info.camera_serial = std::string(reinterpret_cast<const char *>(
        device_info->SpecialInfo.stGigEInfo.chSerialNumber));

    if (camera_info.camera_name.empty()) {
      camera_info.camera_name =
          "GigE_Camera_" + std::to_string(camera_info.camera_id);
    }
  } else if (device_info->nTLayerType == MV_USB_DEVICE) {
    camera_info.device_type = "USB";
    camera_info.camera_name = std::string(reinterpret_cast<const char *>(
        device_info->SpecialInfo.stUsb3VInfo.chUserDefinedName));
    camera_info.camera_serial = std::string(reinterpret_cast<const char *>(
        device_info->SpecialInfo.stUsb3VInfo.chSerialNumber));

    if (camera_info.camera_name.empty()) {
      camera_info.camera_name =
          "USB_Camera_" + std::to_string(camera_info.camera_id);
    }
  } else {
    camera_info.device_type = "Unknown";
    camera_info.camera_name =
        "Unknown_Camera_" + std::to_string(camera_info.camera_id);
    camera_info.camera_serial = "UNKNOWN_" + std::to_string(camera_info.camera_id);
  }

  return true;
}
#endif
