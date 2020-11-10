#ifndef MACROS_H
#define MACROS_H
#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define M_FLUSH flush
#else
    #define M_FLUSH Qt::flush
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define M_LoadAtomic(atom) atom.load()
#else
    #define M_LoadAtomic(atom) atom.loadRelaxed()
#endif

using AtomicBool = QAtomicInteger<unsigned short>; // 16 bit only (faster than using 8 bit variable...)

#endif // MACROS_H
