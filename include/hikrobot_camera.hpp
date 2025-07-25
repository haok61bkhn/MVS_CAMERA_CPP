#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <stdio.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>

#ifndef COMPILE_WITHOUT_MVS_SDK
#include "MvErrorDefine.h"
#include "CameraParams.h"
#include "MvCameraControl.h"
#endif

namespace camera
{
    #define MAX_IMAGE_DATA_SIZE (4 * 2048 * 3072)
    
    cv::Mat frame;
    bool frame_empty = true;
    pthread_mutex_t mutex;

    class Camera
    {
    public:
        Camera(int camera_index = 0);
        ~Camera();
        static void *HKWorkThread(void *p_handle);
        void ReadImg(cv::Mat &image);
        
#ifndef COMPILE_WITHOUT_MVS_SDK
        bool PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo);
#endif

    private:
        void *handle;
        pthread_t nThreadID;
        int camera_index;
        int nRet;
        bool sdk_available;
    };

    Camera::Camera(int cam_index)
    {
        handle = NULL;
        camera_index = cam_index;
        sdk_available = false;

#ifdef COMPILE_WITHOUT_MVS_SDK
        printf("Warning: Compiled without HIKROBOT MVS SDK support!\n");
        printf("This is a demo build. Install HIKROBOT SDK for actual camera functionality.\n");
        
        pthread_mutex_init(&mutex, NULL);
        
        frame = cv::Mat::zeros(480, 640, CV_8UC3);
        cv::putText(frame, "HIKROBOT SDK NOT AVAILABLE", cv::Point(50, 240), 
                   cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
        frame_empty = false;
        
        return;
#else
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet)
        {
            printf("MV_CC_EnumDevices fail! nRet [%x]\n", nRet);
            exit(-1);
        }

        if (stDeviceList.nDeviceNum > 0)
        {
            printf("Found %d camera(s)\n", stDeviceList.nDeviceNum);
            for (int i = 0; i < stDeviceList.nDeviceNum; i++)
            {
                printf("[device %d]:\n", i);
                MV_CC_DEVICE_INFO *pDeviceInfo = stDeviceList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                }
                PrintDeviceInfo(pDeviceInfo);
            }
        }
        else
        {
            printf("Find No Devices!\n");
            exit(-1);
        }

        if (camera_index >= (int)stDeviceList.nDeviceNum)
        {
            printf("Camera index %d not found! Only %d cameras available.\n", camera_index, stDeviceList.nDeviceNum);
            exit(-1);
        }

        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[camera_index]);
        if (MV_OK != nRet)
        {
            printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
            exit(-1);
        }

        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
            exit(-1);
        }

        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
        if (MV_OK == nRet)
        {
            printf("Set TriggerMode to continuous OK!\n");
        }
        else
        {
            printf("Set TriggerMode failed! nRet [%x]\n", nRet);
        }

        nRet = MV_CC_SetEnumValue(handle, "PixelFormat", 0x02180014);
        if (MV_OK == nRet)
        {
            printf("Set PixelFormat to RGB OK!\n");
        }
        else
        {
            printf("Set PixelFormat failed! nRet [%x]\n", nRet);
        }
        
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_StartGrabbing fail! nRet [%x]\n", nRet);
            exit(-1);
        }
        
        nRet = pthread_mutex_init(&mutex, NULL);
        if (nRet != 0)
        {
            perror("pthread_mutex_init failed\n");
            exit(-1);
        }
        
        nRet = pthread_create(&nThreadID, NULL, HKWorkThread, handle);
        if (nRet != 0)
        {
            printf("thread create failed. ret = %d\n", nRet);
            exit(-1);
        }

        sdk_available = true;
        printf("Camera %d initialized successfully!\n", camera_index);
#endif
    }

    Camera::~Camera()
    {
#ifdef COMPILE_WITHOUT_MVS_SDK
        pthread_mutex_destroy(&mutex);
        return;
#else
        if (!sdk_available) return;
        
        pthread_join(nThreadID, NULL);

        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_StopGrabbing fail! nRet [%x]\n", nRet);
        }
        printf("MV_CC_StopGrabbing succeed.\n");
        
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_CloseDevice fail! nRet [%x]\n", nRet);
        }
        printf("MV_CC_CloseDevice succeed.\n");
        
        nRet = MV_CC_DestroyHandle(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_DestroyHandle fail! nRet [%x]\n", nRet);
        }
        printf("MV_CC_DestroyHandle succeed.\n");
        
        pthread_mutex_destroy(&mutex);
#endif
    }

#ifndef COMPILE_WITHOUT_MVS_SDK
    bool Camera::PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo)
    {
        if (NULL == pstMVDevInfo)
        {
            printf("The Pointer of pstMVDevInfoList is NULL!\n");
            return false;
        }
        if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            printf("GigE Device - IP: %x, Name: %s\n", 
                   pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp,
                   pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
        }
        else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
        {
            printf("USB Device - Name: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
        }
        else
        {
            printf("Unknown device type.\n");
        }
        return true;
    }
#endif

    void Camera::ReadImg(cv::Mat &image)
    {
        pthread_mutex_lock(&mutex);
        if (frame_empty)
        {
            image = cv::Mat();
        }
        else
        {
            image = camera::frame.clone();
        }
        pthread_mutex_unlock(&mutex);
    }

    void *Camera::HKWorkThread(void *p_handle)
    {
#ifdef COMPILE_WITHOUT_MVS_SDK
        return 0;
#else
        int nRet;
        unsigned char *m_pBufForDriver = (unsigned char *)malloc(sizeof(unsigned char) * MAX_IMAGE_DATA_SIZE);
        unsigned char *m_pBufForSaveImage = (unsigned char *)malloc(MAX_IMAGE_DATA_SIZE);
        MV_FRAME_OUT_INFO_EX stImageInfo = {0};
        MV_CC_PIXEL_CONVERT_PARAM stConvertParam = {0};
        int image_empty_count = 0;
        
        while (true)
        {
            nRet = MV_CC_GetOneFrameTimeout(p_handle, m_pBufForDriver, MAX_IMAGE_DATA_SIZE, &stImageInfo, 100);
            if (nRet != MV_OK)
            {
                if (++image_empty_count > 100)
                {
                    printf("Too many failed readings, exiting thread!\n");
                    break;
                }
                continue;
            }
            image_empty_count = 0;
            
            stConvertParam.nWidth = stImageInfo.nWidth;
            stConvertParam.nHeight = stImageInfo.nHeight;
            stConvertParam.pSrcData = m_pBufForDriver;
            stConvertParam.nSrcDataLen = stImageInfo.nFrameLen;
            stConvertParam.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
            stConvertParam.pDstBuffer = m_pBufForSaveImage;
            stConvertParam.nDstBufferSize = MAX_IMAGE_DATA_SIZE;
            stConvertParam.enSrcPixelType = stImageInfo.enPixelType;
            MV_CC_ConvertPixelType(p_handle, &stConvertParam);
            
            pthread_mutex_lock(&mutex);
            camera::frame = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC3, m_pBufForSaveImage).clone();
            frame_empty = false;
            pthread_mutex_unlock(&mutex);
        }
        
        free(m_pBufForDriver);
        free(m_pBufForSaveImage);
        return 0;
#endif
    }

}
#endif
