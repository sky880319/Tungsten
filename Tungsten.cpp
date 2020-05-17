#include "Tungsten.h"
#include "Tungsten_PythonHelper.h"

using namespace ts;

int main(int argc, char* argv[]) try
{
    py::init();
    Py_BEGIN_ALLOW_THREADS
    g_scrbt = new ScRobot();
    g_rscam = new RsCamera();
    g_rscam->SetRobot(g_scrbt);

    ObjectQueue tgObjQueue;
    g_rscam->SetQbjQueue(&tgObjQueue);

    int enableFeatures = ColorStream | DepthStream /*| WebCam*/;

    // Setup Scara Robot.
    /*if (g_scrbt->Connect())
    {
        Sc_StartProc();
    }*/
    std::thread sc_proc(Sc_StartProc, &tgObjQueue);

    // Setup rscamera.
    g_rscam->SetThreadSafe(&g_mutex, &g_objQueue_cond);
    g_rscam->SetResolution(640, 480);
    g_rscam->SetFeatures(enableFeatures);
    g_rscam->Connect();
    g_rscam->Display(ColorStream);

    std::thread rs_proc(Rs_StartProc);
    std::thread rs_errmoniter(ErrorMonter);

    //Release All.
    rs_proc.join();
    sc_proc.join();
    SAFE_DELETE(g_rscam);
    SAFE_DELETE(g_scrbt);
    rs_errmoniter.join();
    system("pause");
    Py_END_ALLOW_THREADS
    py::close();
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

    void Sc_StartProc(ObjectQueue* objQueue)
    {
        TgWorld current;

        while (true)
        {
            ProcessObjectQueue(objQueue);
            //if (!g_scrbt->RefreshWorldLocation(current))
            //{
            //    Sleep(1000);
            //    continue;
            //}

            //float x, y, z, c;
            //if (!(std::cin >> x >> y/* >> z >> c*/))
            //{
            //    std::cout << "Input error." << std::endl;
            //    std::cin.clear();
            //    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            //    continue;
            //}
            //std::cout << "Input Position: (" << x << ", " << y /*<< ", " << z << ", " << c*/ << ")" << std::endl;

            //TgWorld newlocation = current;
            //newlocation.setX(x);
            //newlocation.setY(y);

            //std::cout << "New" << newlocation << std::endl;

            //system("pause");

            //g_scrbt->Move(newlocation);
        }
    }

    void ProcessObjectQueue(ObjectQueue* objQueue)
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        while (objQueue->empty())
        {
            // release lock as long as the wait and reaquire it afterwards.
            g_objQueue_cond.wait(lock);
        }
        TgObject* val = objQueue->front();
        objQueue->pop();
        lock.unlock();

        Sleep(5000);
        std::cout << "[TgObjQueue] A ROI object has been processed for " << (float)(clock() - val->time) / 1000 << "s. Queue(" << objQueue->size() << ")" << std::endl <<
                     "  - POP -    oID(" << val->oid << "), Time(" << val->time << "), " << val->vision_point << std::endl << std::endl;
        
        SAFE_DELETE(val);
    }
}