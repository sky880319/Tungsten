#include "Tungsten.h"
#include "Tungsten_PythonHelper.h"

using namespace ts;

int main(int argc, char* argv[]) try
{
    py::init();
    g_scrbt = new ScRobot();
    g_rscam = new RsCamera();
    g_rscam->SetRobot(g_scrbt);
    int enableFeatures = /*ColorStream | DepthStream |*/ WebCam;

    

    /*cv::Mat img = cv::imread("");
    py::func* py_update_frame = new py::func("Streaming", "update_frame");
    PyObject* pRes = py_update_frame->call(py::ParseNumpy8UC3(img));
    Py_DECREF(pRes);*/

    //system("pause");

    // Setup rscamera.
    //g_rscam->SetResolution(640, 480);
    g_rscam->SetFeatures(enableFeatures);
    //g_rscam->Connect();
    
    //g_rscam->Display(DepthStream);

    // Setup Scara Robot.
    g_scrbt->Connect();
    g_rscam->Display(WebCam);
    Sc_StartProc();
    
    //std::thread rs_proc(Rs_StartProc);
    //std::thread rs_errmoniter(ErrorMonter);
    

    //Release All.
    //rs_proc.join();
    SAFE_DELETE(g_rscam);
    SAFE_DELETE(g_scrbt);
    //rs_errmoniter.join();
    py::close();
    system("pause");
    return EXIT_SUCCESS;
}
catch (const rs2::error& e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

namespace ts
{
    void Rs_StartProc()
    {
        g_rscam->Process();
    }

    void ErrorMonter()
    {
        while (g_rscam)
        {
            if (g_rscam->GetStatus() == RsCamera::Status::Error)
            {
                std::cout << "[Stream Process] Stopped reason: (m_eState == Error)." << std::endl;
                g_rscam->Restart();
            }
        }
    }

    void Sc_StartProc()
    {
        world current;

        while (true)
        {
            if (!g_scrbt->RefreshWorldLocation(current))
            {
                Sleep(1000);
                continue;
            }

            float x, y, z, c;
            if (!(std::cin >> x >> y/* >> z >> c*/))
            {
                std::cout << "Input error." << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            std::cout << "Input Position: (" << x << ", " << y /*<< ", " << z << ", " << c*/ << ")" << std::endl;

            world newlocation = current;
            newlocation.setX(x);
            newlocation.setY(y);

            std::cout << "New" << newlocation << std::endl;

            system("pause");

            g_scrbt->Move(newlocation);
        }
    }
}