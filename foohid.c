#include <Python.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CFRunLoop.h>

#include "foohid_types.h"

#include <Python.h>

#define FOOHID_SERVICE "it_unbit_foohid"

#define FOOHID_CREATE 0
#define FOOHID_DESTROY 1
#define FOOHID_SEND 2
#define FOOHID_LIST 3
#define FOOHID_NOTIFY 4

CFRunLoopRef run_loop = NULL;
IONotificationPortRef notification_port = NULL;

static PyObject* pyfunc_event_handler = NULL;

static int foohid_connect(io_connect_t *conn) {
    if (!PyEval_ThreadsInitialized()) {
        printf("will PyEval_InitThreads\n");
        PyEval_InitThreads();
    }
        printf("did PyEval_InitThreads\n");

    io_iterator_t iterator;
    io_service_t service;
    kern_return_t ret = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(FOOHID_SERVICE), &iterator);
    if (ret != KERN_SUCCESS) return -1;

    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        ret = IOServiceOpen(service, mach_task_self(), 0, conn);
        IOObjectRelease(service);
        if (ret == KERN_SUCCESS) {
            IOObjectRelease(iterator);
            return 0;
        }
    }

    IOObjectRelease(iterator);
    return -1;
}

static void foohid_close(io_connect_t conn) {
    IOServiceClose(conn);
}

static PyObject *foohid_create(PyObject *self, PyObject *args) {
    char *name;
    Py_ssize_t name_len;
    char *descriptor;
    Py_ssize_t descriptor_len;
    char *serial;
    Py_ssize_t serial_len;
    unsigned int vendor_id;
    unsigned int device_id;

    if (!PyArg_ParseTuple(args, "s#s#s#II", &name, &name_len, &descriptor, &descriptor_len, &serial, &serial_len, &vendor_id, &device_id)) {
        return NULL;
    }

    if (name_len == 0 || descriptor_len == 0 || serial_len == 0) {
        return PyErr_Format(PyExc_ValueError, "invalid values");
    }

    io_connect_t conn;
    if (foohid_connect(&conn)) {
        return PyErr_Format(PyExc_SystemError, "unable to open " FOOHID_SERVICE " service");
    }

    uint32_t input_count = 8;
    uint64_t input[input_count];
    input[0] = (uint64_t) name;
    input[1] = (uint64_t) name_len;
    input[2] = (uint64_t) descriptor;
    input[3] = (uint64_t) descriptor_len;
    input[4] = (uint64_t) serial;
    input[5] = (uint64_t) serial_len;
    input[6] = (uint64_t) vendor_id;
    input[7] = (uint64_t) device_id;

    kern_return_t ret = IOConnectCallScalarMethod(conn, FOOHID_CREATE, input, input_count, NULL, 0);
    foohid_close(conn);

    if (ret != KERN_SUCCESS) {
        return PyErr_Format(PyExc_SystemError, "unable to create device");
    }

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *foohid_send(PyObject *self, PyObject *args) {
    char *name;
    Py_ssize_t name_len;
    char *report;
    Py_ssize_t report_len;

    if (!PyArg_ParseTuple(args, "s#s#", &name, &name_len, &report, &report_len)) {
        return NULL;
    }

    if (name_len == 0 || report_len == 0) {
        return PyErr_Format(PyExc_ValueError, "invalid values");
    }

    io_connect_t conn;
    if (foohid_connect(&conn)) {
        return PyErr_Format(PyExc_SystemError, "unable to open " FOOHID_SERVICE " service");
    }

    uint32_t input_count = 4;
    uint64_t input[input_count];
    input[0] = (uint64_t) name;
    input[1] = (uint64_t) name_len;
    input[2] = (uint64_t) report;
    input[3] = (uint64_t) report_len;

    kern_return_t ret = IOConnectCallScalarMethod(conn, FOOHID_SEND, input, input_count, NULL, 0);
    foohid_close(conn);

    if (ret != KERN_SUCCESS) {
        return PyErr_Format(PyExc_SystemError, "unable to send hid message");
    }

    Py_INCREF(Py_True);
    return Py_True;
}

void callback(void *refcon, IOReturn result, io_user_reference_t* args, uint32_t numArgs) {
    foohid_report *report;

    printf("callback called.\n");
    if (sizeof(io_user_reference_t) * numArgs != sizeof(foohid_report)) {
        printf("unexpected number of arguments.\n");
        return;
    }

    report = (foohid_report *)args;
    printf("received report (%llu bytes).\n", report->size);

    PyGILState_STATE state = PyGILState_Ensure();

    if (pyfunc_event_handler == NULL){
        printf("pyfunc_event_handler == null!!!\n");
        fflush(stdout);    
    } 

    PyObject* arglist = Py_BuildValue("(y#)", report->data, report->size);
    // 一定要 tuple... 不然 PyObject_CallFunctionObjArgs 會錯

    if (arglist == NULL){
        printf("arglist NULL\n"); 
        fflush(stdout);
    }

   PyObject *pyobjresult = PyObject_CallObject(pyfunc_event_handler, arglist);

    if (pyobjresult == NULL){
        printf("pyobjresult NULL\n"); 
        fflush(stdout);
    }
    Py_DECREF(arglist);
    fflush(stdout);
    PyGILState_Release(state);
}


static PyObject *foohid_subscribe(PyObject *self, PyObject *args) {
    // set handler
    // PyObject *result = NULL;
    PyObject *temp;

    char *name;
    Py_ssize_t name_len;

    if (!PyArg_ParseTuple(args, "s#O:set_callback", &name, &name_len, &temp)) {
        return NULL;
    }
    // set handler
    if (!PyCallable_Check(temp)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be a function");
        return NULL;
    }
    Py_XINCREF(temp);                    /* Add a reference to new func */
    Py_XDECREF(pyfunc_event_handler);    /* Dispose of previous callback */
    pyfunc_event_handler = temp;         /* Remember new callback */

    if (name_len == 0) {
        return PyErr_Format(PyExc_ValueError, "invalid values");
    }

    io_connect_t conn;
    if (foohid_connect(&conn)) {        
        return PyErr_Format(PyExc_SystemError, "unable to open " FOOHID_SERVICE " service");
    }

    // from u2f.c
    mach_port_t mnotification_port;
    CFRunLoopSourceRef run_loop_source;
    io_async_ref64_t async_ref;
    kern_return_t ret;

    // Create port to listen for kernel notifications on.
    notification_port = IONotificationPortCreate(kIOMasterPortDefault);
    if (!notification_port) {
        printf("Error getting notification port.\n");
        return PyErr_Format(PyExc_ValueError, "invalid notification_port");
    }

    // Get lower level mach port from notification port.
    mnotification_port = IONotificationPortGetMachPort(notification_port);
    if (!mnotification_port) {
        printf("Error getting mach notification port.\n");
        return PyErr_Format(PyExc_ValueError, "invalid mnotification_port");
    }

    // Create a run loop source from our notification port so we can add the port to our run loop.
    run_loop_source = IONotificationPortGetRunLoopSource(notification_port);
    if (run_loop_source == NULL) {
        printf("Error getting run loop source.\n");
        return PyErr_Format(PyExc_ValueError, "invalid run_loop_source");
    }

    // Add the notification port and timer to the run loop.
    CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopDefaultMode);

    // Params to pass to the kernel.
    async_ref[kIOAsyncCalloutFuncIndex] = (uint64_t)callback;
    async_ref[kIOAsyncCalloutRefconIndex] = 0;

    uint32_t input_count = 2;
    uint64_t input[input_count];
    input[0] = (uint64_t) name;
    input[1] = (uint64_t) name_len;

    ret = IOConnectCallAsyncScalarMethod(conn, FOOHID_NOTIFY, mnotification_port, async_ref, kIOAsyncCalloutCount, input, input_count, NULL, 0);

    foohid_close(conn);

    if (ret != KERN_SUCCESS) {
        return PyErr_Format(PyExc_SystemError, "unable to subscribe hid message");
    }

    run_loop = CFRunLoopGetCurrent();

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *foohid_destroy(PyObject *self, PyObject *args) {
    char *name;
    Py_ssize_t name_len;

    if (!PyArg_ParseTuple(args, "s#", &name, &name_len)) {
        return NULL;
    }

    if (name_len == 0) {
        return PyErr_Format(PyExc_ValueError, "invalid name");
    }

    io_connect_t conn;
    if (foohid_connect(&conn)) {
        return PyErr_Format(PyExc_SystemError, "unable to open " FOOHID_SERVICE " service");
    }

    uint32_t input_count = 2;
    uint64_t input[input_count];
    input[0] = (uint64_t) name;
    input[1] = (uint64_t) name_len;

    kern_return_t ret = IOConnectCallScalarMethod(conn, FOOHID_DESTROY, input, input_count, NULL, 0);
    foohid_close(conn);

    if (ret != KERN_SUCCESS) {
        return PyErr_Format(PyExc_SystemError, "unable to destroy hid device");
    }

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *foohid_list(PyObject *self, PyObject *args) {

    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }

    io_connect_t conn;
    if (foohid_connect(&conn)) {
        return PyErr_Format(PyExc_SystemError, "unable to open " FOOHID_SERVICE " service");
    }

    uint32_t output_count = 2;
    uint64_t output[2] = {0, 0};

    uint16_t buf_len = 4096;
    char *buf = malloc(buf_len);
    if (!buf) {
        return PyErr_Format(PyExc_MemoryError, "unable to allocate memory");
    }

    uint64_t input[2];

    for(;;) {
        input[0] = (uint64_t) buf;
        input[1] = (uint64_t) buf_len;
        kern_return_t ret = IOConnectCallScalarMethod(conn, FOOHID_LIST, input, 2, output, &output_count);
        foohid_close(conn);
        if (ret != KERN_SUCCESS) {
            free(buf);
            return PyErr_Format(PyExc_SystemError, "unable to list hid devices");
        }
        // all is fine
        if (output[0] == 0) {
            PyObject *ret = PyTuple_New(output[1]);
            uint64_t i;
            char *ptr = buf;
            for(i = 0; i < output[1]; i++) {
                #if PY_MAJOR_VERSION >= 3
                    PyTuple_SetItem(ret, i, PyUnicode_FromString(ptr));
                #else
                    PyTuple_SetItem(ret, i, PyString_FromString(ptr));
                #endif
                ptr += strlen(ptr) + 1;
            }
            free(buf);
            return ret;
        }
        // realloc memory
        buf_len = output[0];
        char *tmp = realloc(buf, buf_len);
        if (!tmp) {
            free(buf);
            return PyErr_Format(PyExc_MemoryError, "unable to allocate memory");
        }
        buf = tmp;
    }
}

static PyMethodDef foohidMethods[] = {
    {"create", foohid_create, METH_VARARGS, "create a new foohid device"},
    {"destroy", foohid_destroy, METH_VARARGS, "destroy a foohid device"},
    {"send", foohid_send, METH_VARARGS, "send a hid message to a foohid device"},
    {"list", foohid_list, METH_VARARGS, "list the currently available foohid devices"},
    {"subscribe", foohid_subscribe, METH_VARARGS, "subscribe foohid devices"},

    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
    static struct PyModuleDef foodhidModule = {
        PyModuleDef_HEAD_INIT,
        "foohid",
        "python wrapper for foohid",
        -1,
        foohidMethods
    };
    PyMODINIT_FUNC PyInit_foohid(void) {
        return PyModule_Create(&foodhidModule);
    }
#else
    PyMODINIT_FUNC initfoohid(void) {
        Py_InitModule3("foohid", foohidMethods, "python wrapper for foohid");
    }
#endif