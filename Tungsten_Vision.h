#ifndef _TUNGSTEN_VISION_H
#define _TUNGSTEN_VISION_H
#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <librealsense2/rs.hpp>
#include "Tungsten_Motion.h"
#include "Tungsten_Define.h"
#include "Tungsten_PythonHelper.h"

#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = NULL; }

enum Features
{
    ColorStream = 1,
    DepthStream = 2,
    LInfraredStream = 4,
    RInfraredStream = 8,
    WebCam = 16,
    Features_Last = 32,
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
    bool Restart();

    void SetRobot(ScRobot* scrbt) { m_scrbt = scrbt; }
    void SetQbjQueue(ObjectQueue* objQueue) { m_pTgObjQueue = objQueue; }
    void SetThreadSafe(std::mutex* m, std::condition_variable* c) { m_mutex = m; m_objQueue_cond = c; }
    void SetThreshold(const rscam_clipper& clipper);
    void SetThreshold(float start, float end);
    Status GetStatus() { return m_eState; }

    static void StartStreaming(RsCamera* rscam);

    std::string   GetWindowName() { return m_sWindowName; }
    rs2::frameset GetFrames();
    bool AlignFrames(rs2::frameset& frameset, rs2_stream align_to, const rscam_clipper& clipper = rscam_clipper());

private:
    bool Initialize();
    bool Uninitialize();

    static void ProcStreamByCV(RsCamera* rscam, Features stream_type);
    void KeepImageByDepth(rs2::frameset& frameset, rs2_stream align_to, const rscam_clipper& clipper);

    std::mutex*              m_mutex;
    std::condition_variable* m_objQueue_cond;
    std::map<Features, std::thread> m_wndProc;
    std::thread              getFmsThread;

    rs2::config	          m_rsConfig;
    rs2::pipeline         m_rsPipeline;
    rs2::pipeline_profile m_rsProfile;

    int    m_Features;
    Status m_eState;

    std::string      m_sWindowName;
    float            m_fStreamingFPS;
    float            m_fImshowFPS;
    int              m_iRes_height;
    int              m_iRes_width;
    float            m_fDepthScale;
    RsCamera_Clipper m_threshold;
    rs2::frameset    m_frameset;

    ScRobot*     m_scrbt;
    ObjectQueue* m_pTgObjQueue;
    long         oid_count;
};

inline Features& operator++(Features& state, int)
{
    const int i = static_cast<int>(state) << 1;
    state = static_cast<Features>((i < 1 || i >= Features_Last) ? 1 : i);
    return state;
}
inline Features& operator--(Features & state, int)
{
    const int i = static_cast<int>(state) >> 1;
    state = static_cast<Features>((i < 1 || i >= Features_Last) ? Features_Last >> 1 : i);
    return state;
}

void CircleDetection(cv::Mat& img, ScRobot* scrbt);
void WndMouseCallBack(int event, int x, int y, int flags, void* userdata);
bool PointY_Cmp(py::obj_info& obj1, py::obj_info& obj2);
Side GetObjectSide(py::obj_info& obj1);
int GetROIObjPoint_Axis_Y(std::vector<py::obj_info>* obj, int upper, int lower);

#endif