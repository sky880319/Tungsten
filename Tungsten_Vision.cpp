#include "Tungsten_Vision.h"
#include <fstream>
#include "Tungsten_PythonHelper.h"
#pragma warning(disable: 26444)
#pragma warning(disable: 4996)

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

    m_eState = Status::Stopped;
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
    if (m_eState != Status::Stopped)
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
        m_eState != Status::Stopped)
    {
        return false;
    }

    m_Features = enable_features;
    return true;
}

bool RsCamera::Initialize()
{
    m_sWindowName = StringFormat("Intel RealSense Depth Camera D435i - Streaming (%dx%d, 30fps)", m_iRes_width, m_iRes_height);

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
    if (m_eState == Status::Streaming)
        m_rsPipeline.stop();

    /*for (std::vector<rscam_point*>::iterator iter = m_vecPoint.begin(); iter != m_vecPoint.end(); ++iter)
    {
        SAFE_DELETE(*iter);
    }

    for (std::vector<rscam_rectangle*>::iterator iter = m_vecRect.begin(); iter != m_vecRect.end(); ++iter)
    {
        SAFE_DELETE(*iter);
    }*/

    for (std::vector<std::thread>::iterator iter = m_wndProc.begin(); iter != m_wndProc.end(); ++iter)
    {
        //(*iter).join();
        //SAFE_DELETE(iter);
    }

    /*m_vecPoint.clear();
    m_vecRect.clear();*/
    //m_wndProc.clear();

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
            cout << (Connect() ? "[Process] Connection successed." : "[Process] Connection failed.") << endl;
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
        /*else if (GetOptions(cmd, "/clear", out))
        {
            if (0 == strcmp(out, "allpoint"))
            {
                for (std::vector<rscam_point*>::iterator iter = m_vecPoint.begin(); iter != m_vecPoint.end(); ++iter)
                {
                    SAFE_DELETE(*iter);
                }
                m_vecPoint.clear();
            }
            else if (0 == strcmp(out, "allrect"))
            {
                for (std::vector<rscam_rectangle*>::iterator iter = m_vecRect.begin(); iter != m_vecRect.end(); ++iter)
                {
                    SAFE_DELETE(*iter);
                }
                m_vecRect.clear();
            }
        }
        else if (GetOptions(cmd, "/get", out))
        {
            int n = atoi(out);
            if (n > 0 &&
                n <= m_vecPoint.size())
            {
                float x, y;
                x = m_vecPoint[n - 1]->x;
                y = m_vecPoint[n - 1]->y;
                rs2::frameset fms = GetFrames();
                fms = AlignFrames(fms, RS2_STREAM_COLOR);
                cout << "(" << x << "," << y << ") " << fms.get_depth_frame().get_distance(x, y);
            }
        }
        else if (0 == strcmp(cmd, "/draw-info"))
        {
            cout << "Point: " << m_vecPoint.size() << endl;
            cout << "Rectangle: " << m_vecRect.size() << endl;
        }
        else if (GetOptions(cmd, "/export", out))
        {
            if (Status::Ready != m_eState)
                continue;

            rs2::frameset fms = GetFrames();
            rs2::depth_frame dptfm = AlignFrames(fms, RS2_STREAM_COLOR).get_depth_frame();
            std::string filename;

            if (0 == strlen(out))
            {
                filename = "export.json";
            }
            else
            {
                (filename = out) += ".json";
            }

            fstream fs;
            fs.open(filename.c_str(), ios::out);
            if (!fs)
            {
                cout << "[Export] Open file failed." << endl;
                continue;
            }

            fs << "{\n";

            for (int i = 0; i < m_vecRect.size(); i++)
            {
                int rp1x, rp1y, rp2x, rp2y;
                rp1x = m_vecRect[i]->p1.x;
                rp1y = m_vecRect[i]->p1.y;
                rp2x = m_vecRect[i]->p2.x;
                rp2y = m_vecRect[i]->p2.y;

                if ((rp1y - rp2y) == 0 ||
                    (rp1x - rp2x) == 0)
                {
                    continue;
                }

                fs << ((i == 0) ? "" : ",\n") << "\"Rectangle" << i << "\":{\n";

                 Let p1 always at up-left side, p2 always at down-right side.
                if ((rp1y - rp2y) > 0)
                    rp1y ^= rp2y ^= rp1y ^= rp2y;
                if ((rp1x - rp2x) > 0)
                    rp1x ^= rp2x ^= rp1x ^= rp2x;

                fs << "    \"Position1\":" << "[" << rp1x << "," << rp1y << "],\n";
                fs << "    \"Position2\":" << "[" << rp2x << "," << rp2y << "],\n";
                fs << "    \"Distance\":[";

                int count = 0;
                for (std::vector<rscam_point*>::iterator piter = m_vecPoint.begin(); piter != m_vecPoint.end(); ++piter)
                {
                     Find the points which is in the area of rectangle.
                    if ((*piter)->x >= rp1x &&
                        (*piter)->x <= rp2x &&
                        (*piter)->y >= rp1y &&
                        (*piter)->y <= rp2y)
                    {
                        float dist = dptfm.get_distance((*piter)->x, (*piter)->y);
                        fs << ((count == 0) ? "" : ",") << dist;
                        count++;
                    }
                }

                fs << "]\n    }";
            }

            fs << "\n}";
            fs.close();
            cout << "[Export] Success: \"" << filename << "\"" << endl;
        }*/
        else if (0 == strcmp(cmd, "/exit"))
        {
            break;
        }
        else
        {
            cout << "[Process] Can not found the command." << endl;
        }

        //Sleep(10);
    }

    for (std::vector<std::thread>::iterator iter = m_wndProc.begin(); iter != m_wndProc.end(); ++iter)
    {
        (*iter).join();
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
    return true;

exit_error:
    m_eState = Status::Error;
    return false;
}

bool RsCamera::Disconnect()
{
    if (Uninitialize())
    {
        m_eState = Status::Stopped;
    }

    m_eState = Status::Error;
    return false;
}

bool RsCamera::Display(Features stream_type)
{
    if (!(m_Features & stream_type))
    {
        return false;
    }

    if (m_eState != Status::Ready)
    {
        if (!Connect())
        {
            return false;
        }
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

    /*char clipdist_mode = 'n';*/
    /*float clipdist_near = 0.25f;
    float clipdist_far = 0.25f;
    rscam_clipper clipper(clipdist_near, clipdist_far);*/

    string windowName = rscam->m_sWindowName + " " + (char)(stream_type + 64);

    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
    //cv::setMouseCallback(windowName, WndMouseCallBack, NULL);

    py::init();
    py::func* py_update_frame = new py::func("Streaming", "update_frame");

    while (getWindowProperty(windowName, WND_PROP_AUTOSIZE) >= 0)
    {
        if (rscam->m_eState != Status::Ready && rscam->m_eState != Status::Streaming)
            return;

        //rscam->m_eState = Status::Streaming;
        frameset fms = rscam->GetFrames();
        fms = rscam->AlignFrames(fms, RS2_STREAM_COLOR, rscam->m_threshold);
        if (!fms) continue;

        frame fm;
        int rendType;

        switch (stream_type)
        {
        default:
        case ColorStream:
            fm = fms.get_color_frame();
            rendType = CV_8UC3;
            break;
        case DepthStream:
            fm = fms.get_depth_frame().apply_filter(color_map);
            rendType = CV_8UC3;
            break;
        case LInfraredStream:
            fm = fms.get_infrared_frame(1);
            rendType = CV_8UC1;
            break;
        case RInfraredStream:
            fm = fms.get_infrared_frame(2);
            rendType = CV_8UC1;
            break;
        }
        const int w = fm.as<video_frame>().get_width();
        const int h = fm.as<video_frame>().get_height();

        Mat img(Size(w, h), rendType, (void*)fm.get_data(), Mat::AUTO_STEP);

        PyObject* pRes = py_update_frame->call(py::ParseNumpy8UC3(img));
        Py_DECREF(pRes);
        
        
        //StreamingOnPython(img);
        //CircleDetection(img);

        // Draw rect & point
        //thread drawAll(DrawAll, fms, img, rscam->m_vecPoint, rscam->m_vecRect);
        //drawAll.join();

        imshow(windowName, img);
        int key = waitKey(1);

        if (key >= 0)
        {
            //cout << "[Key press] No." << key << endl;
            switch (key)
            {
            case 32:
                //features++; // Press "Space" Key to Change Displaying Stream Type.
                break;
            //case 110: // 'n'
            //    clipdist_mode = 'n';
            //    cout << "[clipdist] Change mode to set near value\n";
            //    break;
            //case 102: // 'f'
            //    clipdist_mode = 'f';
            //    cout << "[clipdist] Change mode to set far value\n";
            //    break;
            //case 43:  // '+'
            //    if ('n' == clipdist_mode)
            //    {
            //        clipdist_near += .001f;
            //    }
            //    else if ('f' == clipdist_mode)
            //    {
            //        clipdist_far += .001f;
            //    }
            //    cout << "[clipdist] Change range to " << clipdist_near << " ~ " << clipdist_far << " m" << endl;
            //    clipper.set(clipdist_near, clipdist_far);
            //    break;
            //case 45:  // '-'
            //    if ('n' == clipdist_mode)
            //    {
            //        clipdist_near = (clipdist_near <= .001f) ? (clipdist_near <= 0) ? -0.001f : 0.f : clipdist_near - .001f;
            //    }
            //    else if ('f' == clipdist_mode)
            //    {
            //        clipdist_far = (clipdist_far <= .001f) ? (clipdist_far <= 0) ? -0.001f : 0.f : clipdist_far - .001f;
            //    }
            //    cout << "[clipdist] Change range to " << clipdist_near << " ~ " << clipdist_far << " m" << endl;
            //    clipper.set(clipdist_near, clipdist_far);
            //    break;
            //case 116:
            //    cout << "[Tester] " << ((g_bIsTester = !g_bIsTester) ? "Enable" : "Disable") << endl;
            //    break;
            //case 114:
            //    cout << "[RandTest] " << ((g_bRandMode = !g_bRandMode) ? "Enable" : "Disable") << endl;
            //    break;
            }
        }
    }

    //rscam->m_eState = Status::Ready;
    py::close();
    return;
}

rs2::frameset RsCamera::GetFrames()
{
    return m_rsPipeline.wait_for_frames();
}

rs2::frameset RsCamera::AlignFrames(rs2::frameset & frameset, rs2_stream align_to, const rscam_clipper & clipper)
{
    if (align_to == RS2_STREAM_DEPTH)
        return frameset;

    // Start to align the frame.
    rs2::align align(align_to);
    rs2::frameset fms = align.process(frameset);
    KeepImageByDepth(fms, align_to, clipper);
    return fms;
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

//void WndMouseCallBack(int event, int x, int y, int flags, void* userdata)
//{
//    if (g_rscam == NULL)
//        return;
//
//    if (event == cv::EVENT_LBUTTONDOWN)
//    {
//        printf("[Watchpoint] Add: (%d, %d)\n", x, y);
//        g_rscam->m_vecPoint.push_back(new RsCamera::rscam_point(x, y));
//    }
//    else if (event == cv::EVENT_FLAG_RBUTTON)
//    {
//        printf("[Rectangle] ");
//        if (NULL == firstPoint)
//        {
//            firstPoint = new RsCamera::rscam_point(x, y);
//            std::cout << "First point: ";
//            printf("(%d, %d)\n", x, y);
//        }
//        else
//        {
//            RsCamera::rscam_point* secondPoint = &RsCamera::rscam_point(x, y);
//            RsCamera::rscam_rectangle* rect = new RsCamera::rscam_rectangle(*firstPoint, *secondPoint);
//            g_rscam->m_vecRect.push_back(rect);
//            std::cout << "Second point: ";
//            printf("(%d, %d)\n", x, y);
//
//            if (g_bIsTester)
//            {
//                int rp1x, rp1y, rp2x, rp2y;
//                rp1x = rect->p1.x;
//                rp1y = rect->p1.y;
//                rp2x = rect->p2.x;
//                rp2y = rect->p2.y;
//
//                if ((rp1y - rp2y) != 0 ||
//                    (rp1x - rp2x) != 0)
//                {
//                    // Let p1 always at up-left side, p2 always at down-right side.
//                    if ((rp1y - rp2y) > 0)
//                        rp1y ^= rp2y ^= rp1y ^= rp2y;
//                    if ((rp1x - rp2x) > 0)
//                        rp1x ^= rp2x ^= rp1x ^= rp2x;
//
//                    int padding_x = 10;
//                    int padding_y = 10;
//
//                    rp1x += padding_x;
//                    rp2x -= padding_x;
//                    rp1y += padding_y;
//                    rp2y -= padding_y;
//
//                    int interval_x = (rp2x - rp1x) / 5;
//                    int interval_y = (rp2y - rp1y) / 5;
//
//                    if (g_bRandMode)
//                    {
//                        srand((unsigned)time(NULL));
//
//                        for (int i = 0; i < 5; i++)
//                        {
//                            for (int j = 0; j < 5; j++)
//                            {
//                                int wpx = interval_x * rand() / RAND_MAX + rp1x + interval_x * i;
//                                int wpy = interval_y * rand() / RAND_MAX + rp1y + interval_y * j;
//
//                                printf("[Watchpoint] Add: (%d, %d)\n", wpx, wpy);
//                                g_rscam->m_vecPoint.push_back(new RsCamera::rscam_point(wpx, wpy));
//                            }
//                        }
//                        std::cout << "[Tester] Create random watchpoint success." << std::endl;
//                    }
//                    else
//                    {
//                        for (int wpx = rp1x; wpx <= rp2x; wpx += interval_x)
//                        {
//                            for (int wpy = rp1y; wpy <= rp2y; wpy += interval_y)
//                            {
//                                printf("[Watchpoint] Add: (%d, %d)\n", wpx, wpy);
//                                g_rscam->m_vecPoint.push_back(new RsCamera::rscam_point(wpx, wpy));
//                            }
//                        }
//                        std::cout << "[Tester] Create array watchpoint success." << std::endl;
//                    }
//                }
//            }
//
//            SAFE_DELETE(firstPoint);
//        }
//    }
//}

//float DrawDstText(cv::Mat img, rs2::depth_frame dptfm, RsCamera::rscam_point p)
//{
//    char text[16];
//    float dst = dptfm.get_distance(p.x, p.y);
//    sprintf_s(text, "D=%.3f", dst);
//    cv::Scalar color(0, 255, 255);
//    if (dst == 0)
//    {
//        color = cv::Scalar(0, 0, 255);
//    }
//
//    cv::circle(img, cv::Point(p.x, p.y), 2, color);
//    cv::putText(img, text, cv::Point(p.x + 6, p.y + 2), cv::FONT_HERSHEY_SIMPLEX, 0.4, color);
//
//    return dst;
//}
//
//void DrawRectangle(cv::Mat img, RsCamera::rscam_rectangle rect)
//{
//    char text[16];
//    sprintf_s(text, "(%d, %d)", rect.p1.x, rect.p1.y);
//    cv::putText(img, text, cv::Point(rect.p1.x - 50, rect.p1.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255));
//    sprintf_s(text, "(%d, %d)", rect.p2.x, rect.p1.y);
//    cv::putText(img, text, cv::Point(rect.p2.x - 50, rect.p1.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255));
//    sprintf_s(text, "(%d, %d)", rect.p1.x, rect.p2.y);
//    cv::putText(img, text, cv::Point(rect.p1.x - 50, rect.p2.y + 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255));
//    sprintf_s(text, "(%d, %d)", rect.p2.x, rect.p2.y);
//    cv::putText(img, text, cv::Point(rect.p2.x - 50, rect.p2.y + 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255));
//    cv::line(img, cv::Point(rect.p1.x, rect.p1.y), cv::Point(rect.p2.x, rect.p1.y), cv::Scalar(255, 255, 255));
//    cv::line(img, cv::Point(rect.p1.x, rect.p1.y), cv::Point(rect.p1.x, rect.p2.y), cv::Scalar(255, 255, 255));
//    cv::line(img, cv::Point(rect.p2.x, rect.p2.y), cv::Point(rect.p2.x, rect.p1.y), cv::Scalar(255, 255, 255));
//    cv::line(img, cv::Point(rect.p2.x, rect.p2.y), cv::Point(rect.p1.x, rect.p2.y), cv::Scalar(255, 255, 255));
//}
//
//void DrawAll(rs2::frameset fms, cv::Mat img, rsc_points points, rsc_rectangles rects)
//{
//    using namespace std;
//    using namespace rs2;
//
//    lock_guard<mutex> mLock(mtx);
//
//    depth_frame dptfm = fms.get_depth_frame();
//    if (dptfm)
//    {
//        for (rsc_points::iterator iter = points.begin(); iter != points.end(); ++iter)
//        {
//            if (*iter)
//            {
//                DrawDstText(img, dptfm, **iter);
//            }
//        }
//    }
//    for (rsc_rectangles::iterator iter = rects.begin(); iter != rects.end(); ++iter)
//    {
//        if (*iter)
//        {
//            DrawRectangle(img, **iter);
//        }
//    }
//}

//void ContourDetection(cv::Mat & img)
//{
//    using namespace cv;
//    Mat process;
//    std::vector<std::vector<Point>> contours;
//
//    cvtColor(img, process, COLOR_BGR2GRAY);
//    //GaussianBlur(a, a, Size(3, 3), 0);
//    findContours(process, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
//    drawContours(img, contours, -1, Scalar(0, 0, 255), 2);
//}
//
//void CircleDetection(cv::Mat & img)
//{
//    using namespace cv;
//    Mat process;
//    std::vector<Vec3f> circles;
//
//    cvtColor(img, process, COLOR_BGR2GRAY);
//    GaussianBlur(process, process, Size(3, 3), 0);
//    HoughCircles(process, circles, HOUGH_GRADIENT, 1, process.rows / 8, 150, 40, 0, 0);
//
//    for (std::vector<Vec3f>::iterator iter = circles.begin(); iter != circles.end(); ++iter)
//    {
//        Point center(cvRound((*iter)[0]), cvRound((*iter)[1]));
//        int rad = cvRound((*iter)[2]);
//        circle(img, center, rad, Scalar(0, 0, 255), 2);
//    }
//}

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