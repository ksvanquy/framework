/****************************************************************************
** Generated QML type registration code
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#if __has_include(<runtime_bridge.h>)
#  include <runtime_bridge.h>
#endif


#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_Framework_Qt6()
{
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    qmlRegisterTypesAndRevisions<framework::ui::RuntimeBridge>("Framework.Qt6", 1);
    QT_WARNING_POP
    qmlRegisterModule("Framework.Qt6", 1, 0);
}

static const QQmlModuleRegistration frameworkQt6Registration("Framework.Qt6", qml_register_types_Framework_Qt6);
