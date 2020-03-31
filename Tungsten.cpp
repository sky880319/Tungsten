#include "Tungsten.h"
#include "Tungsten_PythonHelper.h"

using namespace ts;

int main(int argc, char* argv[]) try
{
    py::init();
    g_scrbt = new ScRobot();
    g_rscam = new RsCamera();
    int enableFeatures = ColorStream | DepthStream;

    // Setup Scara Robot.
    g_scrbt->Connect();

    // Setup rscamera.
    g_rscam->SetResolution(1280, 720);
    g_rscam->SetFeatures(enableFeatures);
    std::thread rs_proc(Rs_StartProc);

    //Release All.
    rs_proc.join();
    SAFE_DELETE(g_rscam);
    SAFE_DELETE(g_scrbt);
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

    void Sc_StartProc()
    {

    }
}