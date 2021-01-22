#ifndef __camera_cpp__
#define __camera_cpp__

#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <iostream>

#include "camera.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

Camera::Camera(/* args */) {
    this->fd = 0x00;
    this->state = CameraState::UNINITIALIZED;
}

Camera::~Camera() {
    if (this->state == CameraState::CAPTURING) {
        this->Stop();
    }

    for (int i = 0; i < this->numBuffers; i++) {
        munmap(this->buffers[i].start, this->buffers[i].length);
    }

    if (this->fd != 0x00) {
        close(this->fd);
    }
}

int Camera::OpenCameraFileDescriptor() {
    if ((this->fd = open("/dev/video0", O_RDWR)) < 0) {
        return 1;
    }

    return 0;
}

int Camera::CheckCameraCapabilities() {
    struct v4l2_capability cap;
    if (ioctl(this->fd, VIDIOC_QUERYCAP, &cap) < 0) {
        return 1;
    }

    // std::cout << "Bus Info: " << cap.bus_info << std::endl;
    // std::cout << "Driver: " << cap.driver << std::endl;
    // std::cout << "Card: " << cap.card << std::endl;
    // std::cout << "Version: " << ((cap.version > 16) & 0xFF) << "." << ((cap.version > 8) & 0xFF) << "." << (cap.version & 0xFF) << std::endl;
    // std::cout << "Capabilities:        " << std::bitset<32>(cap.capabilities) << std::endl;
    // std::cout << "Device Capabilities: " << std::bitset<32>(cap.device_caps) << std::endl;

    if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
        return 1;
    }

    if (!(cap.device_caps & V4L2_CAP_STREAMING)) {
        return 1;
    }

    return 0;
}

int Camera::SetFormat() {
    struct v4l2_format format;
    CLEAR(format);

    // V4L2_PIX_FMT_YUV420
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;

    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        return 1;
    }

    this->width = format.fmt.pix.width;
    this->height = format.fmt.pix.height;

    return 0;
}

int Camera::BuildBuffers() {
    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 3;

    if (ioctl(this->fd, VIDIOC_REQBUFS, &bufrequest) < 0) {
        return 1;
    }

    this->numBuffers = bufrequest.count;

    for (int i = 0; i < this->numBuffers; i++) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        // this->buffers[i].bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        // this->buffers[i].bufferinfo.memory = V4L2_MEMORY_MMAP;
        // this->buffers[i].bufferinfo.index = i;

        if (ioctl(this->fd, VIDIOC_QUERYBUF, &buf) < 0) {
            return 1;
        }

        this->buffers[i].length = buf.length;
        this->buffers[i].start = mmap(
            NULL,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            this->fd,
            buf.m.offset
        );

        if (this->buffers[i].start == MAP_FAILED) {
            return 1;
        }
    }

    return 0;
}

int Camera::Init() {
    if (this->OpenCameraFileDescriptor() != 0) {
        return 1;
    }

    if (this->CheckCameraCapabilities() != 0) {
        return 2;
    }

    if (this->SetFormat() != 0) {
        return 3;
    }
    
    if (this->BuildBuffers() != 0) {
        return 4;
    }

    this->state = CameraState::INITIALIZED;

    return 0;
}

int Camera::Start() {
    if (this->state != CameraState::INITIALIZED) {
        return 1;
    }
    if (this->state == CameraState::CAPTURING) {
        return 0;
    }

    enum v4l2_buf_type type;

    for (int i = 0; i < this->numBuffers; i++) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
            return 1;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))
        return 1;

    this->state = CameraState::CAPTURING;

    return 0;
}

int Camera::ReadFrame(std::function<int (void*, uint)> processFrame) {
    if (this->state != CameraState::CAPTURING) {
        return 1;
    }

    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
            case EAGAIN:
                return 0;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                return 1;
        }
    }

    if (buf.index > this->numBuffers) {
        return 1;
    }

    processFrame(this->buffers[buf.index].start, buf.bytesused);

    if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
        return 1;

    return 0;
}

int Camera::Stop() {
    if (this->state != CameraState::CAPTURING) {
        return 0;
    }

    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(this->fd, VIDIOC_STREAMOFF, &type) < 0) {
        return 1;
    }

    this->state = CameraState::INITIALIZED;

    return 0;
}

#endif