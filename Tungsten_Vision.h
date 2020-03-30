#ifndef _TUNGSTEN_VISION_H
#define _TUNGSTEN_VISION_H
#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <librealsense2/rs.hpp>

#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = NULL; }

enum Features
{
    ColorStream = 1,
    DepthStream = 2,
    LInfraredStream = 4,
    RInfraredStream = 8,
    Features_Last = 16,
};

template<typename ... Args>
std::string StringFormat(const char* format, Args ... args);

class RsCamera
{
public:
    typedef struct RsCamera_Clipper
    {
    private:
        float _start;
        float _end;

    public:
        RsCamera_Clipper() : _start(0.f), _end(0.f) {}
        RsCamera_Clipper(float start, float end) : _start(start), _end(end) {}

        void set(float start, float end) { _start = start; _end = end; }
        void set(const RsCamera_Clipper& clipper) { _start = clipper._start; _end = clipper._end; }
        void set_start(float start) { _start = start; }
        void set_end(float end) { _end = end; }

        const float get_start() const { return _start; }
        const float get_end() const { return _end; }
        const float get_distance() const { return _start - _end; }

        operator bool() const { return this != nullptr; }
        RsCamera_Clipper& operator =(const RsCamera_Clipper& clipper) { this->set(clipper); return *this; }
    } rscam_clipper;

    enum class Status
    {
        Stopped = 0,
        Ready = 1,
        Streaming = 2,
        Error = 3,
        Closed = 4,
    };

    RsCamera();
    ~RsCamera();

    bool SetFeatures(int enable_features);
    bool SetResolution(int width, int height);
    void Process();
    bool Connect();
    bool Disconnect();
    bool Display(Features stream_type);

    void SetThreshold(const rscam_clipper& clipper);
    void SetThreshold(float start, float end);

    static void StartStreaming(RsCamera* rscam);

    std::string   GetWindowName() { return m_sWindowName; }
    rs2::frameset GetFrames();
    bool AlignFrames(rs2::frameset& frameset, rs2_stream align_to, const rscam_clipper& clipper = rscam_clipper());

private:
    bool Initialize();
    bool Uninitialize();

    static void ProcStreamByCV(RsCamera* rscam, Features stream_type);
    void KeepImageByDepth(rs2::frameset& frameset, rs2_stream align_to, const rscam_clipper& clipper);

    std::vector<std::thread> m_wndProc;
    std::thread getFmsThread;

    rs2::config	          m_rsConfig;
    rs2::pipeline         m_rsPipeline;
    rs2::pipeline_profile m_rsProfile;

    int m_Features;
    Status m_eState;

    std::string   m_sWindowName;

    int   m_iRes_height;
    int   m_iRes_width;
    float m_fDepthScale;
    RsCamera_Clipper m_threshold;
    rs2::frameset m_frameset;
};

inline Features& operator++(Features& state, int)
{
    const int i = static_cast<int>(state) << 1;
    state = static_cast<Features>((i < 1 || i >= Features_Last) ? 1 : i);
    return state;
}
inline Features & operator--(Features & state, int)
{
    const int i = static_cast<int>(state) >> 1;
    state = static_cast<Features>((i < 1 || i >= Features_Last) ? Features_Last >> 1 : i);
    return state;
}

#endif