#include "Tungsten_PythonHelper.h"

namespace py 
{
    void init()
    {
        Py_SetPythonHome((wchar_t*)L"C:\\Program Files\\Python37");
        Py_Initialize();
        // Set py lib path.
        
        /*PySys_SetPath(Py_DecodeLocale("C:\\Source\\Tungsten\\x64\\Debug", NULL));*/
        //PyRun_SimpleString("sys.path.append('C:\\\\Users\\\\sky88\\\\source\\\\repos\\\\Tungsten\\\\x64\\\\Debug')");
        
        //PyRun_SimpleString("import os; print(os.__file__)");
        //PyRun_SimpleString("import sys, numpy, cv2");
        //PyRun_SimpleString("sys.path = []");
        //PyRun_SimpleString("sys.path.append(r'C:\\Source\\Tungsten\\x64\\Debug')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\python36.zip')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\DLLs')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\Scripts')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\Library\\bin')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\win32')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\win32\\lib')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\Pythonwin')");
        //PyRun_SimpleString("sys.path.append('./')");
        //PyRun_SimpleString("sys.path.append('')");
        //PyRun_SimpleString("sys.path.append(r'C:\\Users\\U0611205\\.ipython')");

        //PyRun_SimpleString("sys.path.append(os.getcwd())");
        /*PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch\\python36.zip", NULL));
        PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch\\DLLs", NULL));
        PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib", NULL));
        PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch", NULL));
        PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages", NULL));
        PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\win32", NULL));
        PySys_SetPath(Py_DecodeLocale("C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\win32\\lib", NULL));
        PySys_SetPath(Py_DecodeLocale("sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\Pythonwin", NULL));*/
        //PyRun_SimpleString("print(sys.path)");
    }

    void close()
    {
        Py_Finalize();
    }

    func::func(const char* module_name, const char* func_name)
    {
        module = nullptr;
        dict = nullptr;
        function = nullptr;
        result = nullptr;
            
        gstate = PyGILState_Ensure();
        module = PyImport_Import(PyUnicode_FromString(module_name));
        if (module == nullptr)
        {
            PyErr_Print();
            return;
        }
        dict = PyModule_GetDict(module);
        if (dict == nullptr)
        {
            PyErr_Print();
            return;
        }
        function = PyDict_GetItemString(dict, func_name);
        if (function == nullptr)
        {
            PyErr_Print();
            return;
        }
    }

    func::~func()
    {
        Py_XDECREF(module);
        Py_XDECREF(function);
        Py_XDECREF(dict);
        std::cout << "[py::func] destructed." << std::endl;
        PyGILState_Release(gstate);
    }

    PyObject* func::call(PyObject* args)
    {
        if (function == nullptr)
        {
            return nullptr;
        }

        if (PyCallable_Check(function))
        {
            // Todo: fix counting error.

            //PyGILState_STATE gstate = PyGILState_Ensure();
            result = PyObject_CallObject(function, args);
            //PyGILState_Release(gstate);
            Py_XDECREF(args);

            if (result == nullptr)
            {
                PyErr_Print();
                return NULL;
            }

            Py_XDECREF(result);
            return result;
        }
        
        return NULL;
    }
    
    PyObject* py::ParseNumpy8UC3(cv::Mat& proc)
    {
        PyGILState_STATE gstate = PyGILState_Ensure();
        import_array();
        PyGILState_Release(gstate);
        // Dimension of 8U3C type array ( height, width, number of color channels per pixel).
        npy_intp dims[3] = { proc.rows, proc.cols, 3 };
        PyObject* py_array, *pArgs;
        int row = 0;
        float* p = proc.ptr<float>(row);

        py_array = PyArray_SimpleNewFromData(3, dims, NPY_UBYTE, p);
        pArgs = PyTuple_New(1);
        int res = PyTuple_SetItem(pArgs, 0, py_array);

        return pArgs;
    }

    bool py::ParsePointVector(PyObject* obj, std::vector<cv::Point>* out)
    {
        PyArrayObject* pResArray;
        import_array();
        PyArray_OutputConverter(obj, &pResArray);

        if (pResArray == NULL)
            return false;

        out->resize(PyArray_SHAPE(pResArray)[0]);
        std::memcpy(out->data(), PyArray_DATA(pResArray), sizeof(int) * PyArray_SIZE(pResArray));

        return (out->size() > 0);
    }
}