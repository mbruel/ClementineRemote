QT += core network quick

CONFIG += c++17

# possible to remove the Connection Thread as Sockets are Async
# cf https://forum.qt.io/topic/120468/qabstractlistmodel-populated-in-a-worker-thread-not-the-gui-one
DEFINES  += __USE_CONNECTION_THREAD__

INCLUDEPATH += $$PWD/../protobuf-3.13.0/src
DEPENDPATH += $$PWD/../protobuf-3.13.0/src

CONFIG(debug, debug|release) :{
    DEFINES += __DEBUG__
}

#For linux
linux {
    LIBS += -L$$PWD/../protobuf-3.13.0/lib/x86_64/ -lprotobuf
}

macx{
    LIBS += -L$$PWD/../protobuf-3.13.0/lib/macx/ -lprotobuf

    PRE_TARGETDEPS += $$PWD/../protobuf-3.13.0/lib/macx/libprotobuf.a
}

win32{
    LIBS += -L$$PWD/../protobuf-3.13.0/lib/win64/ -lprotobuf
}

android {
#    include(/home/bruel/android/android_openssl/openssl.pri)
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

#    LIBS += -L$$PWD/../protobuf-3.13.0/lib/arm64/ -lprotobuf


#    PRE_TARGETDEPS += $$PWD/../protobuf-3.13.0/lib/arm64/libprotobuf.a

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/values/libs.xml \
        android/src/fr/mbruel/ClementineRemote/ClementineRemote.java


    include(/home/bruel/android/android_openssl/openssl.pri)

    ANDROID_EXTRA_LIBS += \
        $$PWD/../protobuf-3.13.0/lib/armv7a/libprotobuf.so \
        $$PWD/../protobuf-3.13.0/lib/arm64/libprotobuf.so

    contains(ANDROID_TARGET_ARCH, armeabi-v7a) {
        LIBS += -L$$PWD/../protobuf-3.13.0/lib/armv7a/ -lprotobuf
    }

    contains(ANDROID_TARGET_ARCH, arm64-v8a) {
        LIBS += -L$$PWD/../protobuf-3.13.0/lib/arm64/ -lprotobuf
    }
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        ClementineRemote.cpp \
        ConnectionWorker.cpp \
        model/PlaylistModel.cpp \
        model/RadioStreamModel.cpp \
        model/RemoteFileModel.cpp \
        model/RemoteSongModel.cpp \
        main.cpp \
        protobuf/remotecontrolmessages.pb.cc

RESOURCES += \
    qml/qml.qrc \
    data/data.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ClementineRemote.h \
    ConnectionWorker.h \
    model/PlaylistModel.h \
    model/RadioStreamModel.h \
    model/RemoteFileModel.h \
    model/RemoteSongModel.h \
    player/RemoteFile.h \
    player/RemotePlaylist.h \
    player/RemoteSong.h \
    player/Stream.h \
    utils/Macro.h \
    utils/Singleton.h \
    protobuf/remotecontrolmessages.pb.h

DISTFILES += \
    protobuf/remotecontrolmessages.proto
