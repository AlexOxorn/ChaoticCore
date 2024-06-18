//
// Created by alexoxorn on 5/23/24.
//

#include <Python.h>
#include "../common.h"
#include "../chaotic_api_types.h"
#include "../chaotic_api.h"
#include <vector>

void decref(void* f) {
    if (!f)
        return;
    if (Py_IsNone((PyObject*) f))
        return;
    Py_DecRef((PyObject*) f);
}

struct ChaoticGame {
    PyObject_HEAD;
    CHAOTIC_Duel duel;
    PyObject* payloads[4];
};

static PyObject* ProtoBufFile;

static PyObject* messageTypes[100] = {};

static void destruct(ChaoticGame* self) {
    for (auto& p : self->payloads) {
        decref(p);
        p = nullptr;
    }
    if (self->duel) {
        CHAOTIC_DestroyDuel(self->duel);
        self->duel = nullptr;
    }
}

static void ChaoticGame_dealloc(ChaoticGame* self) {
    destruct(self);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject* ChaoticGame_new(PyTypeObject* type, PyObject*, PyObject*) {
    ChaoticGame* self;

    self = (ChaoticGame*) type->tp_alloc(type, 0);
    self->duel = nullptr;
    for (auto& p : self->payloads) {
        p = nullptr;
    }

    return (PyObject*) self;
}

static int ChaoticGame_init(ChaoticGame* self, PyObject* args, PyObject* kwds) {
    static const char* kwlist[] = {"seed",
                                   "columns",
                                   "cardReader",
                                   "scriptReader",
                                   "logHandler",
                                   "cardReaderDone",
                                   "cardReaderPayload",
                                   "scriptReaderPayload",
                                   "logHandlerPayload",
                                   "cardReaderDonePayload",
                                   nullptr};
    CHAOTIC_DuelOptions opts{};
    PyObject* cardReader = nullptr;
    PyObject* scriptReader = nullptr;
    PyObject* logHandler = nullptr;
    PyObject* cardReaderDone = nullptr;
    PyObject* cardReaderPayload = nullptr;
    PyObject* scriptReaderPayload = nullptr;
    PyObject* logHandlerPayload = nullptr;
    PyObject* cardReaderDonePayload = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "ii|OOOOOOOO",
                                     const_cast<char**>(kwlist),
                                     &opts.seed,
                                     &opts.columns,
                                     &cardReader,
                                     &scriptReader,
                                     &logHandler,
                                     &cardReaderDone,
                                     &cardReaderPayload,
                                     &scriptReaderPayload,
                                     &logHandlerPayload,
                                     &cardReaderDonePayload)) {
        return -1;
    }

    if (cardReaderPayload)
        Py_XINCREF(cardReaderPayload);
    if (scriptReaderPayload)
        Py_XINCREF(scriptReaderPayload);
    if (logHandlerPayload)
        Py_XINCREF(logHandlerPayload);
    if (cardReaderDonePayload)
        Py_XINCREF(cardReaderDonePayload);

    if (cardReader) {
        if (!PyCallable_Check(cardReader)) {
            PyErr_SetString(PyExc_TypeError, "cardReader must be callable");
            return -1;
        }

        opts.cardReader = [](void* payload, uint32_t code, CHAOTIC_CardData* data) {
            PyObject* func;
            PyObject* f_payload = nullptr;
            PyArg_ParseTuple((PyObject*) payload, "OO", &func, &f_payload);
            PyObject* arglist = Py_BuildValue("(O,i)", f_payload ? (PyObject*) f_payload : Py_None, code);
            PyObject* result = PyObject_CallObject(func, arglist);
            Py_DecRef(arglist);
            if (result == nullptr) {
//                 fprintf(stderr, "CARD READER FAILED WITH EXCEPTION:\n");
                PyErr_Print();
                return;
            }
            PyArg_ParseTuple(result,
                             "iiiiiiiiiiiiiiiiiii",
                             &data->code,
                             &data->supercode,
                             &data->subcode,
                             &data->supertype,
                             &data->subtype,
                             &data->tribe,
                             &data->courage,
                             &data->power,
                             &data->wisdom,
                             &data->speed,
                             &data->energy,
                             &data->fire,
                             &data->air,
                             &data->earth,
                             &data->water,
                             &data->mugic_ability,
                             &data->loyal,
                             &data->limited,
                             &data->legendary);
            return;
        };
        opts.cardReaderPayload = Py_BuildValue("(O,O)", cardReader, cardReaderPayload ?: Py_None);
        self->payloads[0] = static_cast<PyObject*>(opts.cardReaderPayload);
    }

    CHAOTIC_Duel game;
    int success = CHAOTIC_CreateDuel(&game, opts);
    if (success == 0) {
        self->duel = game;
        return 0;
    }

    decref((PyObject*) opts.cardReaderPayload);
    decref((PyObject*) opts.scriptReaderPayload);
    decref((PyObject*) opts.logHandlerPayload);
    decref((PyObject*) opts.cardReaderDonePayload);

    return -1;
}

static PyObject* ChaoticGame_new_card(ChaoticGame* self, PyObject* args, PyObject* kwds) {
    static const char* kwlist[] = {
            "code", "supertype", "controller", "owner", "location", "sequence", "position", nullptr};
    int code, supertype, controller, owner, location, position;
    PyObject* py_sequence;
    sequence_type c_sequence{0};
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "iiiiiOi",
                                     const_cast<char**>(kwlist),
                                     &code,
                                     &supertype,
                                     &controller,
                                     &owner,
                                     &location,
                                     &py_sequence,
                                     &position)) {
        printf("Bad Argument\n");
        return nullptr;
    }

    if (Py_TYPE(py_sequence) == &PyLong_Type) {
        c_sequence.index = PyLong_AsLong(py_sequence);
    } else if (Py_TYPE(py_sequence) == &PyTuple_Type) {
        PyArg_ParseTuple(py_sequence, "ii", &c_sequence.horizontal, &c_sequence.vertical);
    }

    CHAOTIC_NewCardInfo card_info{static_cast<uint32_t>(code),
                                  static_cast<SUPERTYPE>(supertype),
                                  static_cast<PLAYER>(controller),
                                  static_cast<PLAYER>(owner),
                                  static_cast<LOCATION>(location),
                                  c_sequence,
                                  static_cast<POSITION>(position)};

    CHAOTIC_DuelNewCard(self->duel, card_info);

    Py_IncRef(Py_None);
    return Py_None;
}

static PyObject* ChaoticGame_start_duel(ChaoticGame* self, PyObject*) {
    CHAOTIC_StartDuel(self->duel);
    Py_IncRef(Py_None);
    return Py_None;
}

static PyObject* ChaoticGame_duel_process(ChaoticGame* self, PyObject*) {
    int x = CHAOTIC_DuelProcess(self->duel);
    return PyLong_FromLong(x);
}

static PyObject* ChaoticGame_get_message(ChaoticGame* self, PyObject*) {
    uint32_t len;
    char* head = (char*) CHAOTIC_DuelGetMessage(self->duel, &len);
    char* end = head + len;
    PyObject* list = PyList_New(0);
    while (head < end) {
        int32_t subsize = *((int32_t*) (head));
        head += sizeof(subsize);

        uint8_t message_type = *((uint32_t*) (head));
        head += 1;

        PyObject* pyMessageType = messageTypes[message_type];
        PyObject* pyMessage = PyObject_CallObject(pyMessageType, nullptr);
        PyObject* str = subsize ? PyBytes_FromStringAndSize(head, subsize) : PyBytes_FromString("");
        PyObject* method_name = PyUnicode_FromString("ParseFromString");
        PyObject_CallMethodOneArg(pyMessage, method_name, str);

        PyList_Append(list, pyMessage);
        head += subsize;

        Py_DecRef(str);
        Py_DecRef(method_name);
    }
    return list;
}

static PyObject* ChaoticGame_get_message_group(ChaoticGame* self, PyObject*) {
    PyObject* list = PyList_New(0);
    int res;
    PyObject* pyextend = PyUnicode_FromString("extend");
    while (true) {
        res = CHAOTIC_DuelProcess(self->duel);
        PyObject* sublist = ChaoticGame_get_message(self, nullptr);
        PyObject_CallMethodOneArg(list, pyextend, sublist);
        Py_DecRef(sublist);
        if (res != CHAOTIC_DUEL_STATUS_CONTINUE)
            break;
    }
    Py_DecRef(pyextend);

    PyObject* result = Py_BuildValue("i,O", res, list);
    Py_DecRef(list);
    return result;
}

static PyObject* CHAOTIC_set_response(ChaoticGame* self, PyObject* list) {
    std::vector<int32_t> response;

    PyObject* iter = PyObject_GetIter(list);
    if (!iter) {
        Py_XDECREF(iter);
        Py_IncRef(Py_None);
        return Py_None;
    }

    for (PyObject* item = PyIter_Next(iter); item != nullptr; item = PyIter_Next(iter)) {
        response.push_back((int) PyLong_AsLong(item));
        Py_XDECREF(item);
    }

    CHAOTIC_DuelSetResponse(self->duel, response.data(), response.size() * sizeof(response[0]));

    Py_IncRef(Py_None);
    return Py_None;
}

static PyObject* CHAOTIC_respond_and_get(ChaoticGame* self, PyObject* args) {
    if (PyTuple_Size(args)) {
        CHAOTIC_set_response(self, args);
    }
    return ChaoticGame_get_message_group(self, nullptr);
}

static PyObject* CHAOTIC_print_board(ChaoticGame* self, PyObject* args) {
    CHAOTIC_PrintBoard(self->duel);
    Py_IncRef(Py_None);
    return Py_None;
}

static PyObject* CHAOTIC_enter(ChaoticGame* self, PyObject*) {
    return (PyObject*) self;
}

static PyObject* CHAOTIC_exit(ChaoticGame* self, PyObject*) {
    destruct(self);
    Py_IncRef(Py_None);
    return Py_None;
}

static PyMethodDef ChaoticGame_methods[] = {
        {
         "new_card", (PyCFunction) ChaoticGame_new_card,
         METH_VARARGS | METH_KEYWORDS,
         "Adds a card to the game\nArgs: (code, supertype, controller, owner, location, sequence, position)", },
        {
         "start_duel", (PyCFunction) ChaoticGame_start_duel,
         METH_VARARGS, "Start the Game",
         },
        {
         "process", (PyCFunction) ChaoticGame_duel_process,
         METH_VARARGS, "Processes until messages",
         },
        {
         "messages", (PyCFunction) ChaoticGame_get_message,
         METH_VARARGS, "Get current messages",
         },
        {
         "message_group", (PyCFunction) ChaoticGame_get_message_group,
         METH_VARARGS, "loop and process until a response is needed",
         },
        {
         "respond", (PyCFunction) CHAOTIC_set_response,
         METH_VARARGS, "Set a game response",
         },
        {
         "respond_and_get", (PyCFunction) CHAOTIC_respond_and_get,
         METH_VARARGS, "Respond to the game, and get the next set of messages",
         },
        {
         "print_board", (PyCFunction) CHAOTIC_print_board,
         METH_VARARGS, "Print A minimal board",
         },
        {
         "__enter__", (PyCFunction) CHAOTIC_enter,
         METH_VARARGS, "",
         },
        {
         "__exit__", (PyCFunction) CHAOTIC_exit,
         METH_VARARGS, "",
         },
        {nullptr}  /* Sentinel */
};

static PyTypeObject ChaoticGame_Type = {
        PyVarObject_HEAD_INIT(nullptr, 0)         /*ob_size*/
        "chaotic.ChaoticGame",                    /*tp_name*/
        sizeof(ChaoticGame),                      /*tp_basicsize*/
        0,                                        /*tp_itemsize*/
        (destructor) ChaoticGame_dealloc,         /*tp_dealloc*/
        0,                                        /*tp_print*/
        0,                                        /*tp_getattr*/
        0,                                        /*tp_setattr*/
        0,                                        /*tp_compare*/
        0,                                        /*tp_repr*/
        0,                                        /*tp_as_number*/
        0,                                        /*tp_as_sequence*/
        0,                                        /*tp_as_mapping*/
        0,                                        /*tp_hash */
        0,                                        /*tp_call*/
        0,                                        /*tp_str*/
        0,                                        /*tp_getattro*/
        0,                                        /*tp_setattro*/
        0,                                        /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
        "MyTest objects",                         /* tp_doc */
        0,                                        /* tp_traverse */
        0,                                        /* tp_clear */
        0,                                        /* tp_richcompare */
        0,                                        /* tp_weaklistoffset */
        0,                                        /* tp_iter */
        0,                                        /* tp_iternext */
        ChaoticGame_methods,                      /* tp_methods */
        0,                                        /* tp_members */
        0,                                        /* tp_getset */
        0,                                        /* tp_base */
        0,                                        /* tp_dict */
        0,                                        /* tp_descr_get */
        0,                                        /* tp_descr_set */
        0,                                        /* tp_dictoffset */
        (initproc) ChaoticGame_init,              /* tp_init */
        0,                                        /* tp_alloc */
        ChaoticGame_new,                          /* tp_new */
};

static void init_messages() {
#define SET_MESSAGE(name) messageTypes[x++] = (PyObject_GetAttrString(ProtoBufFile, #name));
    int x = 0;
    SET_MESSAGE(MSG_ShuffleAttackDeck)
    SET_MESSAGE(MSG_ShuffleLocationDeck)
    SET_MESSAGE(MSG_ShuffleAttackHand)
    SET_MESSAGE(MSG_ShuffleMugicHand)
    SET_MESSAGE(MSG_Move)
    SET_MESSAGE(MSG_Draw)
    SET_MESSAGE(MSG_NewTurn)
    SET_MESSAGE(MSG_NewPhase)
    SET_MESSAGE(MSG_ActivateLocation)
    SET_MESSAGE(MSG_SelectMove)
    SET_MESSAGE(MSG_Retry)
    SET_MESSAGE(MSG_CreatureMove)
    SET_MESSAGE(MSG_CombatStart)
    SET_MESSAGE(MSG_SelectAttackCard)
    SET_MESSAGE(MSG_Damage)
    SET_MESSAGE(MSG_WonInitiative)
    SET_MESSAGE(MSG_StrikerChange)
    SET_MESSAGE(MSG_CombatEnd)
    SET_MESSAGE(MSG_Recover)
#undef SET_MESSAGE
}

static struct PyModuleDef chaotic = {PyModuleDef_HEAD_INIT, "chaotic", "__doc__", -1};
PyMODINIT_FUNC PyInit_chaotic() {
    PyObject* m = PyModule_Create(&chaotic);
    if (PyType_Ready(&ChaoticGame_Type) < 0)
        return nullptr;

    Py_IncRef(reinterpret_cast<PyObject*>(&ChaoticGame_Type));
    PyModule_AddObject(m, "Chaotic", (PyObject*) &ChaoticGame_Type);
    ProtoBufFile = PyImport_ImportModule("messages_pb2");
    init_messages();
    return m;
}
