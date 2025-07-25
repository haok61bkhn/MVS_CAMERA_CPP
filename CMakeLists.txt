cmake_minimum_required(VERSION 3.10)
project(hikrobot_camera)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COMPILE_WITHOUT_MVS_SDK "Compile without MVS SDK" OFF)
if(COMPILE_WITHOUT_MVS_SDK)
    add_definitions(-DCOMPILE_WITHOUT_MVS_SDK)
endif()

find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)

set(MVS_SDK_PATH "/opt/MVS")

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    include
    ${MVS_SDK_PATH}/include/
)

link_directories(
    ${MVS_SDK_PATH}/lib/64/
)

set(MVS_LIBS
    GCBase_gcc485_v3_0
    GenCP_gcc485_v3_0
    log4cpp_gcc485_v3_0
    Log_gcc485_v3_0
    MvCameraControl
    CLProtocol_gcc485_v3_0
    FormatConversion
)

set(FOUND_MVS_LIBS "")
set(MISSING_MVS_LIBS "")

foreach(lib ${MVS_LIBS})
    find_library(${lib}_FOUND ${lib} PATHS ${MVS_SDK_PATH}/lib/64/ NO_DEFAULT_PATH)
    if(${lib}_FOUND)
        list(APPEND FOUND_MVS_LIBS ${lib})
    else()
        list(APPEND MISSING_MVS_LIBS ${lib})
    endif()
endforeach()

if(MISSING_MVS_LIBS)
    message(WARNING "Missing HIKROBOT libraries: ${MISSING_MVS_LIBS}")
    message(WARNING "Building without HIKROBOT SDK support. Install SDK for full functionality.")
    set(USE_MVS_SDK FALSE)
else()
    message(STATUS "Found all HIKROBOT libraries")
    set(USE_MVS_SDK TRUE)
endif()

add_library(hikrobot_camera_lib
    src/hikrobot_camera.cpp
    src/plc.cpp
)

add_executable(${PROJECT_NAME}
    main.cpp
)

if(USE_MVS_SDK)
    target_link_libraries(hikrobot_camera_lib
        ${OpenCV_LIBRARIES}
        ${FOUND_MVS_LIBS}
        ${Boost_LIBRARIES}
        pthread
    )
else()
    target_link_libraries(hikrobot_camera_lib
        ${OpenCV_LIBRARIES}
        ${Boost_LIBRARIES}
        pthread
    )
endif()

target_link_libraries(${PROJECT_NAME}
    hikrobot_camera_lib
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
