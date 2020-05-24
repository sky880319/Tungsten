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
    if (!g_scrbt->Connect())
    {
        SAFE_DELETE(g_rscam);
        SAFE_DELETE(g_scrbt);
        PyEval_RestoreThread(_save);
        py::close();
        return 0;
    }
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
        //TgWorld current;

        while (true)
        {
            /*if (!g_scrbt->RefreshWorldLocation(current))
            {
                Sleep(1000);
                continue;
            }*/

            ProcessObjectQueue(objQueue/*, current*/);
        }

        //while (true)
        //{
        //    if (!g_scrbt->RefreshWorldLocation(current))
        //    {
        //        Sleep(1000);
        //        continue;
        //    }

        //    /*for (int i = 0; i < 50; i++)
        //    {
        //        g_scrbt->VelocityTest();
        //    }


        //    system("pause");*/

        //    ProcessObjectQueue(objQueue, current);

        //    //float x, y, z, c;
        //    //if (!(std::cin >> x >> y/* >> z >> c*/))
        //    //{
        //    //    std::cout << "Input error." << std::endl;
        //    //    std::cin.clear();
        //    //    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        //    //    continue;
        //    //}
        //    //std::cout << "Input Position: (" << x << ", " << y /*<< ", " << z << ", " << c*/ << ")" << std::endl;

        //    //TgWorld newlocation = current;
        //    //newlocation.setX(x);
        //    //newlocation.setY(y);

        //    //std::cout << "New" << newlocation << std::endl;

        //    //system("pause");

        //    //g_scrbt->Move(newlocation);
        //}
    }

    void ProcessObjectQueue(ObjectQueue* objQueue/*, const TgWorld& current*/)
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

        //Sleep(4000);

        // Solutuon 1:
        //TgWorld exp(-200.f, val->vision_point.Y(), val->vision_point.Z() + 10, 0.f);

        //float t_ms = ((exp.X() - val->vision_point.X()) / OBJECT_SPEED - std::sqrt(std::pow(current.X() - exp.X(),2) + std::pow(current.Y() - exp.Y(), 2)) / ROBOT_SPEED) * 1000 - 1250;
        //if (t_ms < 0)
        //{
        //    std::cout << "[Tungsten Alarm] Robot will delay." << std::endl;
        //}
        //time_pt time;

        //while (true)
        //{
        //    time = std::chrono::steady_clock::now();
        //    if (val->getDuration<std::chrono::milliseconds>(time) >= t_ms)
        //    {
        //        /*time_pt now = std::chrono::steady_clock::now();
        //        if (!g_scrbt->Move(val->getExceptLocation(now)))
        //        {
        //            std::cout << "[TgMotion] Failed to moving scara. (Reason: reached the upper limit.)" << std::endl;
        //        }*/
        //        g_scrbt->Move(exp);
        //        break;
        //    }
        //    Sleep(10);
        //}

        //Solution 2:

        TgWorld exc;
        int delay;
        time_pt time;
        time_pt start = std::chrono::steady_clock::now();;

        if (evalObjectTracking(*val/*, current*/, exc, delay))
        {
            while (true)
            {
                time = std::chrono::steady_clock::now();
                if (/*val->getDuration<std::chrono::milliseconds>(time)*/std::chrono::duration_cast<std::chrono::milliseconds>(time - start).count() >= delay)
                {
                    g_scrbt->SetProc(exc);
                    break;
                }
            }
        }
        else
        {
            std::cout << "[TgMotion] Failed to moving scara. (Reason: Object missing.)" << std::endl;
        }

        std::cout << "[TgObjQueue] A ROI object has been processed for " << (float)val->getDuration<std::chrono::milliseconds>(time) / 1000 << "s, delay: " << (float)delay / 1000 << "s." << std::endl <<
            "  - POP -    Queue(" << objQueue->size() << "), oID(" << val->oid << "), " << val->vision_point << std::endl <<
            "Move to: " << exc << std::endl << std::endl;
        
        SAFE_DELETE(val);
    }

    // An object tracker will evaluate a delay time and a except location to move the robot to suck the object.
    // [IN]  trace_obj: trace object.
    // [IN]  current_location: Robot current world location.
    // [OUT] except_location: Return a loction to except robot arrived.
    // [OUT] delay: Return a delay time(ms) to except robot start moving.
    bool evalObjectTracking(TgObject& trace_obj/*, const TgWorld& current_location*/, TgWorld& except_location, int& delay)
    {
        delay = 0;

        // Try to evaluation a time to expect delay time will more than 0ms.
        int delay_flag = 0;
        TgWorld current_location;

        g_scrbt->RefreshWorldLocation(current_location);

        while (delay <= 0)
        {
            except_location = trace_obj.getExceptLocation(std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_flag++ * 100));

            float z = except_location.Z() + 10;
            except_location.setZ((z < 128.f) ? 128.f : z);

            float exp_x = except_location.X();
            float exp_y = except_location.Y();
            float exp_z = except_location.Z();

            // "except_location" must be in the safe area, if over the upper limit, the object probably missing.
            if (exp_x * UNIT_TRANSFORM > WORKING_LIMIT_X2)
            {
                return false;
            }

            // "except_location" must over the lower limit that the robot can track.
            if (exp_x * UNIT_TRANSFORM >= WORKING_LIMIT_X1)
            {
                float except_curr_x = trace_obj.getExceptLocation(std::chrono::steady_clock::now()).X();
                float t_detect2exp = (exp_x > except_curr_x) ? ((exp_x - except_curr_x) / OBJECT_SPEED) : ((except_curr_x - exp_x) / OBJECT_SPEED);
                float t_robot2exp = std::sqrt(std::pow(current_location.X() - exp_x, 2) + std::pow(current_location.Y() - exp_y, 2) + std::pow(current_location.Z() - exp_z, 2)) / ROBOT_SPEED;

                delay = (t_detect2exp - t_robot2exp) * 1000 - 392;
            }
        }

        // Reserved space for sucker.
        
        return true;
    }
}