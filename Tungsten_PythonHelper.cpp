#include "Tungsten_PythonHelper.h"

namespace py {
    void init()
    {
        Py_Initialize();
        // Set py lib path.
        //PySys_SetPath(Py_DecodeLocale("C:\\Users\\sky88\\source\\repos\\Tungsten\\x64\\Debug", NULL));
        //PyRun_SimpleString("sys.path.append('C:\\\\Users\\\\sky88\\\\source\\\\repos\\\\Tungsten\\\\x64\\\\Debug')");
        PyRun_SimpleString("import sys");
        PyRun_SimpleString("import cv2");
    }

    void close()
    {
        Py_Finalize();
    }

    func::func(const char* module_name, const char* func_name)
    {
        module = PyImport_Import(PyUnicode_FromString(module_name));
        dict = PyModule_GetDict(module);
        function = PyDict_GetItemString(dict, func_name);
    }

    func::~func()
    {
        if (module != NULL)
            Py_DECREF(module);
        if (function != NULL)
            Py_DECREF(function);
        if (dict != NULL)
            Py_DECREF(dict);
        std::cout << "[py::func] destructed." << std::endl;
    }

    PyObject* func::call(PyObject* args)
    {
        if (PyCallable_Check(function))
        {
            PyObject* res = PyObject_CallObject(function, args);
            Py_DECREF(args);
            return res;
        }
        return nullptr;
    }
    
    PyObject* ParseNumpy8UC3(cv::Mat& mat)
    {
        import_array();
        // Dimension of 8U3C type array ( height, width, number of color channels per pixel).
        npy_intp dims[3] = { mat.rows, mat.cols, 3 };
        PyObject* py_array, *pArgs;
        int row = 0;
        float* p = mat.ptr<float>(row);

        py_array = PyArray_SimpleNewFromData(3, dims, NPY_UBYTE, p);
        pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, py_array);
        Py_DECREF(py_array);

        return pArgs;
    }
}