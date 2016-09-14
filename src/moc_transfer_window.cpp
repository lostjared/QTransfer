/****************************************************************************
** Meta object code from reading C++ file 'transfer_window.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "transfer_window.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'transfer_window.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ConnectWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   26,   26,   26, 0x0a,
      27,   26,   26,   26, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ConnectWindow[] = {
    "ConnectWindow\0onConnect()\0\0onSelectDir()\0"
};

void ConnectWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ConnectWindow *_t = static_cast<ConnectWindow *>(_o);
        switch (_id) {
        case 0: _t->onConnect(); break;
        case 1: _t->onSelectDir(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ConnectWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ConnectWindow::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ConnectWindow,
      qt_meta_data_ConnectWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ConnectWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ConnectWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ConnectWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ConnectWindow))
        return static_cast<void*>(const_cast< ConnectWindow*>(this));
    return QDialog::qt_metacast(_clname);
}

int ConnectWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
static const uint qt_meta_data_ListenWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      13,   24,   24,   24, 0x0a,
      25,   24,   24,   24, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ListenWindow[] = {
    "ListenWindow\0onListen()\0\0onSelectFile()\0"
};

void ListenWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ListenWindow *_t = static_cast<ListenWindow *>(_o);
        switch (_id) {
        case 0: _t->onListen(); break;
        case 1: _t->onSelectFile(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ListenWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ListenWindow::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ListenWindow,
      qt_meta_data_ListenWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ListenWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ListenWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ListenWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ListenWindow))
        return static_cast<void*>(const_cast< ListenWindow*>(this));
    return QDialog::qt_metacast(_clname);
}

int ListenWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
static const uint qt_meta_data_TransferWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   27,   27,   27, 0x0a,
      28,   27,   27,   27, 0x0a,
      39,   27,   27,   27, 0x0a,
      49,   27,   27,   27, 0x0a,
      60,   27,   27,   27, 0x0a,
      77,   27,   27,   27, 0x0a,
      94,   27,   27,   27, 0x0a,
     114,  155,   27,   27, 0x0a,
     158,   27,   27,   27, 0x0a,
     175,   27,   27,   27, 0x0a,
     193,   27,   27,   27, 0x0a,
     214,  155,   27,   27, 0x0a,
     256,   27,   27,   27, 0x0a,
     274,  301,   27,   27, 0x0a,
     307,   27,   27,   27, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_TransferWindow[] = {
    "TransferWindow\0onConnect()\0\0onListen()\0"
    "onAbout()\0onCancel()\0onShowInFinder()\0"
    "onConConnected()\0onConDisconnected()\0"
    "onConError(QAbstractSocket::SocketError)\0"
    "se\0onConReadyRead()\0onListConnected()\0"
    "onListDisconnected()\0"
    "onListError(QAbstractSocket::SocketError)\0"
    "onListReadyRead()\0onListBytesWritten(qint64)\0"
    "bytes\0onNewConnection()\0"
};

void TransferWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TransferWindow *_t = static_cast<TransferWindow *>(_o);
        switch (_id) {
        case 0: _t->onConnect(); break;
        case 1: _t->onListen(); break;
        case 2: _t->onAbout(); break;
        case 3: _t->onCancel(); break;
        case 4: _t->onShowInFinder(); break;
        case 5: _t->onConConnected(); break;
        case 6: _t->onConDisconnected(); break;
        case 7: _t->onConError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 8: _t->onConReadyRead(); break;
        case 9: _t->onListConnected(); break;
        case 10: _t->onListDisconnected(); break;
        case 11: _t->onListError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 12: _t->onListReadyRead(); break;
        case 13: _t->onListBytesWritten((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 14: _t->onNewConnection(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData TransferWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject TransferWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_TransferWindow,
      qt_meta_data_TransferWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &TransferWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *TransferWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *TransferWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TransferWindow))
        return static_cast<void*>(const_cast< TransferWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int TransferWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
