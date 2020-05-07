#include "Tungsten_Vision.h"
#include <fstream>
#include "Tungsten_PythonHelper.h"
#include "Tungsten_Motion.h"
#pragma warning(disable: 26444)
#pragma warning(disable: 4996)

cv::Point* g_pWndPoint;

static bool GetOptions(const char* cmd, const char* opt, char* ret)
{
    ret[0] = '\0';

    char* ptr = (char*)strstr(cmd, opt);
    char* ptr2 = ret;

    if (ptr == NULL)
        return false;

    ptr += strlen(opt);

    if (*ptr != ' ')
        return true;

    ptr++;
    while (*ptr != 0)
    {
        *ptr2++ = *ptr++;
    }
    *ptr2 = 0;

    return true;
}

RsCamera::RsCamera()
{
    m_sWindowName = "";

    m_eState = Status::Closed;
    m_Features = 0;

    m_fDepthScale = 0;
    m_iRes_width = 640;
    m_iRes_height = 480;
    //m_dstAve = 0;
    //m_bAlign = false;
}

RsCamera::~RsCamera()
{
    Uninitialize();
}

bool RsCamera::SetResolution(int width, int height)
{
    if (m_eState != Status::Closed)
    {
        return false;
    }
    // Todo: Check if w & h is over the range.

    m_iRes_width = width;
    m_iRes_height = height;
    return true;
}

bool RsCamera::SetFeatures(int enable_features)
{
    if (enable_features >= Features_Last ||
        m_eState != Status::Closed)
    {
        return false;
    }

    m_Features = enable_features;
    return true;
}

bool RsCamera::Initialize()
{
    m_sWindowName = StringFormat("RealSense Streaming (%dx%d, 30fps)", m_iRes_width, m_iRes_height);

    try {
        // Enable to support the features.
        if (m_Features & ColorStream)
        {
            m_rsConfig.enable_stream(RS2_STREAM_COLOR, m_iRes_width, m_iRes_height, RS2_FORMAT_BGR8, 30);
            std::cout << "[RsCamera::InitializeEnvironment] Color Stream Enabled.\n";
        }
        if (m_Features & DepthStream)
        {
            m_rsConfig.enable_stream(RS2_STREAM_DEPTH, m_iRes_width, m_iRes_height, RS2_FORMAT_Z16, 30);
            std::cout << "[RsCamera::InitializeEnvironment] Depth Stream Enabled.\n";
        }
        if (m_Features & LInfraredStream)
        {
            m_rsConfig.enable_stream(RS2_STREAM_INFRARED, 1, m_iRes_width, m_iRes_height, RS2_FORMAT_Y8, 30);
            std::cout << "[RsCamera::InitializeEnvironment] Left Infrared Stream Enabled.\n";
        }
        if (m_Features & RInfraredStream)
        {
            m_rsConfig.enable_stream(RS2_STREAM_INFRARED, 2, m_iRes_width, m_iRes_height, RS2_FORMAT_Y8, 30);
            std::cout << "[RsCamera::InitializeEnvironment] Right Infrared Stream Enabled.\n";
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool RsCamera::Uninitialize()
{
    if (m_eState == Status::Closed)
        return false;

    m_eState = Status::Stopped;
    
    for (std::vector<std::thread>::iterator iter = m_wndProc.begin(); iter != m_wndProc.end(); ++iter)
    {
        (*iter).join();
    }
    m_wndProc.clear();

    m_rsPipeline.stop();
    getFmsThread.join();

    m_eState = Status::Closed;
    return true;
}

void RsCamera::Process()
{
    using namespace std;

    while (true)
    {
        char cmd[32] = { 0 };
        char out[32] = { 0 };
        cin.getline(cmd, 31);
        if (cmd[0] != '/')
        {
            continue;
        }

        if (0 == strcmp(cmd, "/connect"))
        {
            Connect();
        }
        else if (GetOptions(cmd, "/display", out))
        {
            char out2[8];
            if (0 == strcmp(out, "color"))
            {
                cout << (Display(ColorStream) ? "[Process] Start to stream color frame." : "[Process] Fail to stream color frame from RealSense Camera.") << endl;
            }
            else if (0 == strcmp(out, "depth"))
            {
                cout << (Display(DepthStream) ? "[Process] Start to stream depth frame." : "[Process] Fail to stream depth frame from RealSense Camera.") << endl;
            }
            else if (GetOptions(out, "infrared", out2))
            {
                if (0 == strcmp(out, "left"))
                {
                    cout << (Display(LInfraredStream) ? "[Process] Start to stream left infrared frame." : "[Process] Fail to stream left infrared frame from RealSense Camera.") << endl;
                }
                else if (0 == strcmp(out, "right"))
                {
                    cout << (Display(RInfraredStream) ? "[Process] Start to stream right infrared frame." : "[Process] Fail to stream right infrared frame from RealSense Camera.") << endl;
                }
            }
        }
        else if (0 == strcmp(cmd, "/toggle res"))
        {
            if (m_iRes_height == 720)
            {
                if (SetResolution(640, 480))
                {
                    cout << "[Resolution] Toggle resolution to 480p." << endl;
                }
                else
                {
                    cout << "[Resolution] Toggle resolution to 480p failed." << endl;
                }
            }
            else
            {

                if (SetResolution(1280, 720))
                {
                    cout << "[Resolution] Toggle resolution to 720p." << endl;
                }
                else
                {
                    cout << "[Resolution] Toggle resolution to 720p failed." << endl;
                }
            }
        }
        else if (GetOptions(cmd, "/threshold", out))
        {
            vector<float> values;
            
            char* ptr = out;
            
            while (*ptr != 0)
            {
                char val[6] = { 0 };
                char* val_ptr = val;
                while (*ptr != 0)
                {
                    if (val_ptr != &val[5])
                    {
                        if (*ptr == '.' || (*ptr >= '0' && *ptr <= '9'))
                        {
                            *val_ptr++ = *ptr;
                        }
                    }
                    ptr++;
                    if (*ptr == ' ')
                    {
                        ptr++;
                        break;
                    }
                }
                values.push_back((float)atof(val));
            }

            if (values.size() < 2 || values[0] == values[1])
            {
                cout << "[Threshold] Error value." << endl;
            }

            (values[0] < values[1]) ? SetThreshold(values[0], values[1]) : SetThreshold(values[1], values[2]);
        }
        else if (0 == strcmp(cmd, "/disconnect"))
        {
            Uninitialize();
        }
        else if (0 == strcmp(cmd, "/exit"))
        {
            break;
        }
        else if (0 == strcmp(cmd, "/error"))
        {
            m_eState = Status::Error;
        }
        else
        {
            cout << "[Process] Can not found the command." << endl;
        }
    }
}

bool RsCamera::Connect()
{
    rs2::depth_sensor* s;
    if (m_Features <= 0 ||
        !Initialize())
    {
        goto exit_error;
    }

    // Start to connect d435i depth camera, and get the depth scale data from device
    // to support to align the color and depth frame.
    m_rsProfile = m_rsPipeline.start(m_rsConfig);
    if (!m_rsProfile)
    {
        goto exit_error;
    }

    s = &m_rsProfile.get_device().first<rs2::depth_sensor>();
    m_fDepthScale = s->get_depth_scale();
    m_eState = Status::Ready;
    getFmsThread = std::thread(&RsCamera::StartStreaming, this);
    std::cout << "[Connect] Successed." << std::endl;
    return true;

exit_error:
    m_eState = Status::Error;
    std::cout << "[Connect] Failed." << std::endl;
    return false;
}

bool RsCamera::Disconnect()
{
    if (Uninitialize())
    {
        m_eState = Status::Stopped;
        return true;
    }

    m_eState = Status::Error;
    return false;
}

bool RsCamera::Restart()
{
    if (Uninitialize() &&
        Connect() &&
        Display(Features::ColorStream) &&
        Display(Features::DepthStream)
        )
    {
        return true;
    }

    return false;
}

void RsCamera::StartStreaming(RsCamera* rscam)
{
    if (rscam == NULL)
    {
        return;
    }
    rscam->m_eState = Status::Ready;

    do
    {
        try
        {
            rs2::frameset fms = rscam->m_rsPipeline.wait_for_frames();
            if (fms == NULL)
            {
                break;
            }
            rscam->AlignFrames(fms, RS2_STREAM_COLOR, rscam->m_threshold);
            rscam->m_frameset = fms;
            rscam->m_frameset.keep();

            if (rscam->m_eState == Status::Ready)
            {
                rscam->m_eState = Status::Streaming;
            }
            
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
            rscam->m_eState = Status::Error;
            break;
        }
    }
    while (rscam->m_eState == Status::Streaming || rscam->m_eState == Status::Ready);

    std::cout << "[Stream Process] Stopped." << std::endl;
}

bool RsCamera::Display(Features stream_type)
{
    if (!(m_Features & stream_type))
    {
        return false;
    }

    if (WebCam == stream_type)
    {
        m_eState = Status::Streaming;
    }

    if (m_eState != Status::Streaming && m_eState != Status::Ready)
    {
        std::cout << "[Display] stream_type(" << (int)stream_type << ") can not streaming, reason: No frameset detected." << std::endl;
        return false;
        /*if (!Connect())
        {
            return false;
        }*/
    }

    m_wndProc.push_back(std::thread(&RsCamera::ProcStreamByCV, this, stream_type));

    return true;
}

void RsCamera::ProcStreamByCV(RsCamera * rscam, Features stream_type)
{
    using namespace cv;
    using namespace rs2;
    using namespace std;

    colorizer color_map;
    char stream_type_str[16] = "";

    bool rec = false;

    switch (stream_type)
    {
    case ColorStream:
        strncpy_s(stream_type_str, "Color", sizeof(stream_type_str));
        break;
    case DepthStream:
        strncpy_s(stream_type_str, "Depth", sizeof(stream_type_str));
        break;
    case LInfraredStream:
        strncpy_s(stream_type_str, "LInfrared", sizeof(stream_type_str));
        break;
    case RInfraredStream:
        strncpy_s(stream_type_str, "RInfrared", sizeof(stream_type_str));
        break;
    case WebCam:
        strncpy_s(stream_type_str, "WebCam", sizeof(stream_type_str));
        break;
    default:
        return;
    }

    string windowName = StringFormat("%s - %s (%d)", rscam->m_sWindowName.c_str(), (char*)stream_type_str, time(0));
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback(windowName, WndMouseCallBack, NULL);
    //py::func* py_update_frame = new py::func("Streaming", "update_frame");

    std::cout << "[WndProc] Streaming: stream_type(" << (int)stream_type << "), windowName(" << windowName << ")" << endl;

    if (WebCam == stream_type)
    {
        VideoCapture capture(0);
        Mat frame;

        while (capture.read(frame))
        {
            capture >> frame;
            CircleDetection(frame, rscam->m_scrbt);
            line(frame, Point(0, frame.rows / 2), Point(frame.cols, frame.rows / 2), Scalar(0, 0, 255), 1, cv::LINE_AA);
            imshow(windowName, frame);
            waitKey(1);
        }
    }
    else
    {
        while (getWindowProperty(windowName, WND_PROP_AUTOSIZE) >= 0)
        {
            if (rscam->m_eState == Status::Ready)
                continue;
            else if (rscam->m_eState != Status::Streaming)
                break;

            frameset* fms = &rscam->m_frameset;
            if (fms == nullptr) continue;

            frame fm;
            int rendType;

            switch (stream_type)
            {
            default:
            case ColorStream:
                fm = fms->get_color_frame();
                rendType = CV_8UC3;
                break;
            case DepthStream:
                fm = fms->get_depth_frame().apply_filter(color_map);
                rendType = CV_8UC3;
                break;
            case LInfraredStream:
                fm = fms->get_infrared_frame(1);
                rendType = CV_8UC1;
                break;
            case RInfraredStream:
                fm = fms->get_infrared_frame(2);
                rendType = CV_8UC1;
                break;
            }
            const int w = fm.as<video_frame>().get_width();
            const int h = fm.as<video_frame>().get_height();

            Mat img(Size(w, h), rendType, (void*)fm.get_data(), Mat::AUTO_STEP);

            /*PyObject* pRes = py_update_frame->call(py::ParseNumpy8UC3(img));
            Py_DECREF(pRes);*/

            //StreamingOnPython(img);

            if (rec)
            {
                static int count_rec = 1;
                static int active_time_rec = time(0);
                char fn_rec[64] = { 0 };
                snprintf(fn_rec, 64, "C:/realsense_rec/%d_%d.png", count_rec, active_time_rec);
                imwrite(fn_rec, img);
                count_rec++;
            }

            //if (stream_type == ColorStream)
            //    CircleDetection(img);
            if (g_pWndPoint != NULL)
            {
                line(img, Point(0, g_pWndPoint->y), Point(img.cols, g_pWndPoint->y), Scalar(0, 0, 255), 1, cv::LINE_4);
                line(img, Point(g_pWndPoint->x, 0), Point(g_pWndPoint->x, img.rows), Scalar(0, 0, 255), 1, cv::LINE_4);

                TgPoint vision = parseVisionCoordinate(*g_pWndPoint, img);
                TgWorld world = parseWorldCoordinate(vision);

                char visionPosition[32];
                char worldPosition[32];
                sprintf_s(visionPosition, "vision(%d, %d)", vision.X(), vision.Y());
                sprintf_s(worldPosition,  "world(%.3f, %.3f)", world.X(),  world.Y());
                cv::putText(img, visionPosition, cv::Point(g_pWndPoint->x + 10, g_pWndPoint->y - 30), cv::FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 255));
                cv::putText(img, worldPosition, cv::Point(g_pWndPoint->x + 10, g_pWndPoint->y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 255));
            }
            else
            {
                line(img, Point(0, img.rows / 2), Point(img.cols, img.rows / 2), Scalar(0, 0, 255), 1, cv::LINE_4);
                line(img, Point(img.cols / 2, 0), Point(img.cols / 2, img.rows), Scalar(0, 0, 255), 1, cv::LINE_4);
            }

            cv::imshow(windowName, img);
            int key = waitKey(1);

            if (key >= 0)
            {
                //cout << "[Key press] No." << key << endl;
                switch (key)
                {
                case 114:
                    rec = !rec;
                    std::cout << "start record" << endl;
                    break;
                case 115:
                    static int count = 1;
                    static int active_time = time(0);
                    char fn[64] = { 0 };
                    snprintf(fn, 64, "C:/realsense_pic/%d_%d.png", active_time, count);
                    bool res = imwrite(fn, img);
                    if (res)
                    {
                        std::cout << "[Save] \"" << fn << "\"(" << count++ << ")" << endl;
                    }
                    break;
                }
            }
        }
    }
    destroyWindow(windowName);
    std::cout << "[WndProc] Thread destroyed: stream_type(" << (int)stream_type << "), windowName(" << windowName << ")" << endl;
    return;
}

rs2::frameset RsCamera::GetFrames()
{
    return m_rsPipeline.wait_for_frames();
}

bool RsCamera::AlignFrames(rs2::frameset& frameset, rs2_stream align_to, const rscam_clipper& clipper)
{
    if (frameset == NULL)
        return false;
    if (align_to == RS2_STREAM_DEPTH)
        return (frameset == NULL);

    // Start to align the frame.
    rs2::align align(align_to);
    frameset = align.process(frameset);
    KeepImageByDepth(frameset, align_to, clipper);
}

void RsCamera::KeepImageByDepth(rs2::frameset& frameset, rs2_stream align_to, const rscam_clipper& clipper)
{
    // This function is copied from: 
    // https://dev.intelrealsense.com/docs/rs-align-advanced
    // ------------------------------------------------------

    rs2::video_frame otherFrame = frameset.first(align_to);
    const rs2::depth_frame& depthFrame = frameset.get_depth_frame();

    if (!otherFrame || !depthFrame || !clipper ||
        clipper.get_distance() == 0)
        return;

    const uint16_t * pDptfm = reinterpret_cast<const uint16_t*>(depthFrame.get_data());
    uint8_t * pOtherfm = reinterpret_cast<uint8_t*>(const_cast<void*>(otherFrame.get_data()));

    int width = otherFrame.get_width();
    int height = otherFrame.get_height();
    int other_bpp = otherFrame.get_bytes_per_pixel();

    for (int y = 0; y < height; y++)
    {
        auto depth_pixel_index = y * width;
        for (int x = 0; x < width; x++, ++depth_pixel_index)
        {
            // Get the depth value of the current pixel
            auto pixels_distance = m_fDepthScale * pDptfm[depth_pixel_index];

            // Check if the depth value is in the range.
            if (pixels_distance <= clipper.get_start() || pixels_distance > clipper.get_end())
            {
                // Calculate the offset in other frame's buffer to current pixel
                auto offset = depth_pixel_index * other_bpp;

                // Set pixel to "background" color (0x999999)
                std::memset(&pOtherfm[offset], 0x99, other_bpp);
            }
        }
    }
}

void RsCamera::SetThreshold(const rscam_clipper& clipper)
{
    m_threshold = clipper;
}

void RsCamera::SetThreshold(float start, float end)
{
    m_threshold = RsCamera_Clipper(start, end);
}

template<typename ... Args>
std::string StringFormat(const char* format, Args ... args)
{
    size_t size = _snprintf(nullptr, 0, format, args ...) + 1;
    char* buf = new char[size];
    _snprintf(buf, size, format, args ...);
    std::string str(buf, size - 1);
    delete[] buf;
    return str;
}


void CircleDetection(cv::Mat& img, ScRobot* scrbt)
{
    using namespace cv;
    Mat process;
    std::vector<Vec3f> circles;

    cvtColor(img, process, COLOR_BGR2GRAY);
    GaussianBlur(process, process, Size(3, 3), 0);
    HoughCircles(process, circles, HOUGH_GRADIENT, 1, process.rows / 8, 150, 135, 0, 0);

    for (std::vector<Vec3f>::iterator iter = circles.begin(); iter != circles.end(); ++iter)
    {
        Point center(cvRound((*iter)[0]), cvRound((*iter)[1]));
        int rad = cvRound((*iter)[2]);
        circle(img, center, rad, Scalar(0, 0, 255), 2);

        if (cvRound((*iter)[1]) > img.rows / 2)
        {
            //system("pause");
            Sleep(21000);
            scrbt->RunNC(430);
            break;
        }
    }
}

void WndMouseCallBack(int event, int x, int y, int flags, void* userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        if (g_pWndPoint != NULL)
        {
            SAFE_DELETE(g_pWndPoint);
        }
        g_pWndPoint = new cv::Point(x, y);
    }
}