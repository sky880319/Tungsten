#include "Tungsten.h"
#include "Tungsten_PythonHelper.h"

#ifdef _SUCK_MODE_
    #include "Tungsten/TungstenProgram_SuckObject.h"
#elif defined _PUSH_MODE_
    #include "Tungsten/TungstenProgram_PushObject.h"
#endif

//#define _TEST_MODE_

using namespace ts;

int main(int argc, char* argv[]) try
{
    py::init();
    Py_BEGIN_ALLOW_THREADS

    int session = 0;
    g_scrbt = new ScRobot(session);
    g_rscam = new RsCamera();
    g_rscam->SetRobot(g_scrbt);

    TgObject_Type mode = TgObject_Type::Push;

#ifdef _SUCK_MODE_
    g_scrbt->m_prog = new TgProg_SuckObject(NC_SUCK_PROC, session);
#elif defined _PUSH_MODE_
    g_scrbt->m_prog = new TgProg_PushObject(NC_PUSH_PROC, session);
#endif

    ObjectQueue tgObjQueue;
    g_rscam->SetQbjQueue(&tgObjQueue);

    int enableFeatures = ColorStream | DepthStream /*| WebCam*/;

    // Setup Scara Robot.
#ifndef _TEST_MODE_
    if (!g_scrbt->Connect())
    {
        SAFE_DELETE(g_rscam);
        SAFE_DELETE(g_scrbt);
        PyEval_RestoreThread(_save);
        py::close();
        return 0;
    }
#endif
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
    SAFE_DELETE(g_scrbt->m_prog);
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
#ifndef _TEST_MODE_
        g_scrbt->RunProc();
        g_scrbt->SetProc(TgWorld(HOME_X, HOME_Y, 180.0f, 0.f));
#endif

        while (true)
        {
#ifndef _TEST_MODE_
            if (COND_PROG_END)
            {
                std::cout << "[Tungsten Alarm] NC program not running." << std::endl;
                break;
            }

            if (!COND_CUST_R_VAL(FEATURE_LOAD_NCFILE, g_scrbt->m_prog->getProgNum()))
            {
                std::cout << "[Tungsten Alarm] NC program number not match." << std::endl;
                break;
            }
#endif

        //while (true)

            ProcessObjectQueue(objQueue);
        }
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

        TgWorld exc;
        int delay = 0;
        time_pt time = std::chrono::steady_clock::now();
        time_pt start = std::chrono::steady_clock::now();

        if (evalObjectTracking(*val, exc, delay))
        {
            while (true)
            {
                time = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(time - start).count() >= delay)
                {
#ifndef _TEST_MODE_
                    g_scrbt->SetProc(exc);
#endif
                    break;
                }
            }
        }
        else
        {
            std::cout << "[TgMotion] Failed to moving scara. (Reason: Object missing.)" << std::endl;
        }


        std::cout << "[TgObjQueue] A ROI object has been processed for " << (float)val->getDuration<std::chrono::milliseconds>(time) / 1000 << "s, delay: " << (float)delay / 1000 << "s." << std::endl <<
                     "  - POP -    Queue(" << objQueue->size() << "), oID(" << val->oid << "), Type(" << val->type << "), Side(" << val->side << ")" << std::endl <<
                     "             Center: " << val->vision_point << std::endl <<
                     "             Box_p1: " << val->box_pt1 << std::endl <<
                     "             Box_p2: " << val->box_pt2 << std::endl <<
                     "             Except: " << exc << std::endl << std::endl;
    
        SAFE_DELETE(val);
    }

    // An object tracker will evaluate a delay time and a except location to move the robot to suck the object.
    // [IN]  trace_obj: trace object.
    // [IN]  current_location: Robot current world location.
    // [OUT] except_location: Return a loction to except robot arrived.
    // [OUT] delay: Return a delay time(ms) to except robot start moving.
    bool evalObjectTracking(TgObject& trace_obj, TgWorld& except_location, int& delay)
    {
        delay = 0;

#ifdef _SUCK_MODE_
        if (trace_obj.type == TgObject_Type::Suck)
        {
            if (g_scrbt->m_prog->getProgNum() != NC_SUCK_PROC)
            {
                std::cout << "[Tungsten Alarm] Object type not match. (Current: " << g_scrbt->m_prog->getProgNum() << ")" << std::endl;
                return false;
            }

            // Try to evaluation a time to expect delay time will more than 0ms.
            int delay_flag = 0;
            TgWorld current_location;

#ifndef _TEST_MODE_
            g_scrbt->RefreshWorldLocation(current_location);
#else
            current_location = TgWorld(HOME_X, HOME_Y, 180.0f, 0.f);
#endif

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

            return true;
        }
#elif defined _PUSH_MODE_
        if (trace_obj.type == TgObject_Type::Push)
        {
            if (g_scrbt->m_prog->getProgNum() != NC_PUSH_PROC)
            {
                std::cout << "[Tungsten Alarm] Object type not match. (Current: " << g_scrbt->m_prog->getProgNum() << ")" << std::endl;
                return false;
            }

            int delay_flag = 0;
            TgWorld current_location;

#ifndef _TEST_MODE_
            g_scrbt->RefreshWorldLocation(current_location);
#else
            current_location = TgWorld(HOME_X, HOME_Y, 180.0f, 0.f);
#endif

            while (delay <= 0)
            {
                except_location = trace_obj.getExceptLocation(std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_flag++ * 100));
                except_location.setZ(90.f);
                except_location.setX(except_location.X() - 200.f);

                float target_y = 0;

                switch (trace_obj.side)
                {
                case Side::Left:
                    except_location.setY(trace_obj.box_pt1.Y() + 20.f);
                    target_y = 416.2f;
                    break;
                case Side::Right:
                    except_location.setY(trace_obj.box_pt2.Y() - 30.f);
                    target_y = 357.0f;
                    break;
                default:
                    return false;
                }

                float exp_x = except_location.X();
                float exp_y = except_location.Y();
                float exp_z = except_location.Z();

                // "except_location" must over the lower limit that the robot can track.
                if (exp_x * UNIT_TRANSFORM >= WORKING_LIMIT_X1)
                {
                    float except_curr_x = trace_obj.getExceptLocation(std::chrono::steady_clock::now()).X() - 200.f;
                    float t_detect2exp = (exp_x > except_curr_x) ? ((exp_x - except_curr_x) / OBJECT_SPEED) : ((except_curr_x - exp_x) / OBJECT_SPEED);
                    float t_robot2exp = std::sqrt(std::pow(current_location.X() - exp_x, 2) + std::pow(current_location.Y() - exp_y, 2)) / ROBOT_SPEED;
                    float t_robotdown = ((current_location.Z() > exp_z) ? current_location.Z() - exp_z : exp_z - current_location.Z()) / ROBOT_SPEED;
                    float t_push_dist = ((target_y > except_location.Y()) ? target_y - except_location.Y() : except_location.Y() - target_y) / ROBOT_SPEED;

                    delay = (t_detect2exp - (t_robotdown + t_robot2exp + t_push_dist)) * 1000 - 800;

                    //std::cout << "CurX: " << except_curr_x << ", " << except_location << " Delay: " << delay << "ms." << std::endl;
                }

                // "except_location" must be in the safe area, if over the upper limit, the object probably missing.
                if (exp_x * UNIT_TRANSFORM > WORKING_LIMIT_X2)
                {
                    //except_location.setX(29.f - exp_x - (float)WORKING_LIMIT_X2 / UNIT_TRANSFORM);
                    return false;
                }
            }

            return true;
        }    
#endif
        
        return false;
    }
}