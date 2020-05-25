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

        PyGILState_STATE gstate;
        std::mutex m_Mutex;
    };

    PyObject* ParseNumpy8UC3(cv::Mat& mat);
}

#define Py_END_ALLOW_THREADS
#endif