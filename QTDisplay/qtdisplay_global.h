#ifndef QTDISPLAY_GLOBAL_H
#define QTDISPLAY_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef QTDISPLAY_LIB
# define QTDISPLAY_EXPORT Q_DECL_EXPORT
#else
# define QTDISPLAY_EXPORT Q_DECL_IMPORT
#endif

#endif // QTDISPLAY_GLOBAL_H
