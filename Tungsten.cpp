#include "Tungsten.h"
#include "Tungsten_PythonHelper.h"

int main(int argc, char* argv[]) try
{
    using namespace cv;
    using namespace rs2;
    using namespace std;

    RsCamera* g_rscam = new RsCamera();
    int enableFeatures = ColorStream | DepthStream;

    bool enableFrameAlign = true;
    py::init();

    // Setup rscamera.
    g_rscam->SetResolution(1280, 720);
    g_rscam->SetFeatures(enableFeatures);
    g_rscam->Process();

    SAFE_DELETE(g_rscam);
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