#ifndef __camera_h__
#define __camera_h__

#include <functional> 
#include <linux/videodev2.h>

struct Buffer {
    void* start;
    unsigned long length;
};

enum CameraState {
	UNINITIALIZED = 0,
	INITIALIZED = 1,
	CAPTURING = 2
};

class Camera {
	private:
		int fd;
        uint numBuffers = 0;
        Buffer buffers[3];
		int state;

		int OpenCameraFileDescriptor();
        int CheckCameraCapabilities();
        int SetFormat();
        int BuildBuffers();
	public:
        int width;
        int height;

		Camera();
		~Camera();
		int Init();
		int Start();
		int ReadFrame(std::function<int (void*, uint)> processFrame);
		int Stop();
};

#include "camera.cpp"

#endif