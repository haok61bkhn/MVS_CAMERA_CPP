# HIKROBOT Camera - Simplified Version

A simplified camera application for HIKROBOT (Hikvision) industrial cameras. 

## Features

- Support for multiple camera indices
- Real-time image display using OpenCV
- Simple keyboard controls (quit, save frames)
- Standard cmake build system

## Requirements

1. **OpenCV** (tested with OpenCV 3.x/4.x)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libopencv-dev
   ```

2. **HIKROBOT MVS SDK** - Download and install from Hikvision
   - Default installation path: `/opt/MVS/`
   - **Important**: The SDK must be installed before building this project (can download at release)

## Installation

1. Clone the repository:
```bash
git clone <repository-url>
cd HIKROBOT-MVS-CAMERA-ROS
```

2. Make sure OpenCV and HIKROBOT MVS SDK are installed

3. Build the project:
```bash
./build.sh
```

## Usage

### Basic Usage
```bash
# Use default camera (index 0)
./build/bin/hikrobot_camera

# Use specific camera index
./build/bin/hikrobot_camera 1
```

### Controls
- **q** or **ESC**: Quit the application
- **s**: Save current frame as JPEG
- **Ctrl+C**: Graceful shutdown

### Camera Selection
If you have multiple HIKROBOT cameras connected, you can specify which one to use:
```bash
./build/bin/hikrobot_camera 0  # First camera
./build/bin/hikrobot_camera 1  # Second camera
./build/bin/hikrobot_camera 2  # Third camera
```

The application will list all detected cameras on startup.

## Manual Build (Alternative)

If you prefer to build manually:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```