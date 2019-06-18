/**
Copyright 2019 JasmineGraph Team
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include "Python.h"
#include "Python_C_API.h"
#include <iostream>
#include <fstream>
using namespace std;

void Python_C_API::train(int argc, char *argv[]) {

    Py_Initialize();

    ofstream myfile;
    myfile.open ("logs/API.txt");
    myfile << "in C-Python API Train method" << endl;
    if (argc%2 != 0) {
        fprintf(stderr, "Usage: [--flag name] [args]\n");
    }
    myfile << "at initializer" << endl;

    FILE *file;
    wchar_t *_argv[argc];
    for (int i = 0; i < argc; i++) {
        wchar_t *arg = Py_DecodeLocale(argv[i], NULL);
        _argv[i] = arg;
    }
    myfile << "arg set" << endl;
    PySys_SetArgv(argc, _argv);
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    myfile << "path set" << endl;
    PyList_Append(path, PyUnicode_FromString("./GraphSAGE/graphsage/"));
    file = fopen("./GraphSAGE/graphsage/unsupervised_train.py", "r");
    PyRun_SimpleFile(file, "./GraphSAGE/graphsage/unsupervised_train.py");

//    PyObject *obj = Py_BuildValue("s", "test.py");
//    file = _Py_fopen_obj(obj, "r+");
//    if(file != NULL) {
//        myfile << "start file run" << endl;
//        PyRun_SimpleFile(file, "test.py");
//    }

//    PyList_Append(path, PyUnicode_FromString("."));
//    file = fopen("./test.py", "r");
//    PyRun_SimpleFile(file, "./test.py");
    myfile << "at finalizer" << endl;
    fclose(file);

    Py_Finalize();

}

int Python_C_API::predict(int argc, char *argv[]) {
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    if (argc != 4) {
        fprintf(stderr, "Usage: call [args]\n");
        return 1;
    }

    Py_Initialize();

    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("./GraphSAGE/graphsage/"));
    pName = PyUnicode_DecodeFSDefault("predict");
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "predict");
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(argc);
            for (i = 0; i < argc; ++i) {
                pValue = PyUnicode_FromString(argv[i]);
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
                    fprintf(stderr, "Cannot convert argument\n");
                    return 1;
                }
                /* pValue reference stolen here: */
                PyTuple_SetItem(pArgs, i, pValue);
            }
            PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
        } else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", "predict");
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", "predict");
        return 1;
    }
    Py_Finalize();
    return 0;
}
