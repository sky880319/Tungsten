#ifndef _TUNGSTEN_PYTHONHELPER_H
#define _TUNGSTEN_PYTHONHELPER_H
#pragma once

#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <opencv2/opencv.hpp>
#include <mutex>

namespace py {
    void init();
    void close();

    struct func
    {
    public:
        func(const char* module_name, const char* func_name);
        ~func();

        PyObject* call(PyObject* args);

    private:
        PyObject* module;
        PyObject* function;
        PyObject* dict;
        PyObject* result;

        PyGILState_STATE gstate;
        std::mutex m_Mutex;
    };

    struct obj_info
    {
        int area;
        cv::Point center;
        cv::Point box_pt1;
        cv::Point box_pt2;

        obj_info()
        {
            area = 0;
            center = cv::Point();
            box_pt1 = cv::Point();
            box_pt2 = cv::Point();
        }

        obj_info(const obj_info& ref)
        {
            *this = ref;
        }

        obj_info& operator=(const obj_info& ref)
        {
            area = ref.area;
            center = ref.center;
            box_pt1 = ref.box_pt1;
            box_pt2 = ref.box_pt2;
            return *this;
        }

        //cv::Point getCenterInt()
        //{
        //    return cv::Point(center/*(int)center.x, (int)center.y*/);
        //}

        //cv::Rect getRectInt()
        //{
        //    return cv::Rect(box_pt1, box_pt2);
        //}
    };

    PyObject* ParseNumpy8UC3(cv::Mat& proc);
    bool ParsePointVector(PyObject* obj, std::vector<obj_info>* out);
}
#endif