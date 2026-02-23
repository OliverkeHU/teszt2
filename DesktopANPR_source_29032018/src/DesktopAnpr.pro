#-------------------------------------------------
#
# Project created by QtCreator 2018-03-12T09:40:23
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DesktopAnpr
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwidget.cpp \
    video_widget.cpp \
    license_plate_reader.cpp \
    logger.cpp \
    lpreader/ANPROCR.cpp \
    lpreader/BinRLE.cpp \
    lpreader/ImageTransform.cpp \
    lpreader/LPReader.cpp \
    lpreader/LPTracker.cpp \
    lpreader/MipMapper.cpp \
    lpreader/MotionFilterRLE.cpp \
    lpreader/NetImage.cpp \
    lpreader/NeuralNet_f16_16.cpp \
    lpreader/PoligonDraw.cpp \
    lpreader/SharedNetValueBuffer.cpp \
    reading_list.cpp \
    minimal_data/HAData.cpp \
    lpr_thread.cpp \
    video_frame_bundle.cpp \
    image_widget.cpp \
    video_control_bar.cpp \
    video_progress_widget.cpp \
    lpr_config_widget.cpp \
    runtime_config.cpp

HEADERS += \
    mainwidget.h \
    video_widget.h \
    license_plate_reader.h \
    logger.h \
    lpreader/ANPROCR.h \
    lpreader/BinRLE.h \
    lpreader/ErrorVal.h \
    lpreader/HAData.h \
    lpreader/ImageTransform.h \
    lpreader/LPReader.h \
    lpreader/LPTracker.h \
    lpreader/MipMapper.h \
    lpreader/MotionFilterRLE.h \
    lpreader/NetImage.h \
    lpreader/nettypes.h \
    lpreader/NeuralNet_f16_16.h \
    lpreader/PoligonDraw.h \
    lpreader/SharedNetValueBuffer.h \
    reading_list.h \
    minimal_data/data.h \
    minimal_data/data_serializer.h \
    minimal_data/HAData.h \
    minimal_data/serialization/data_serializer_hadata.h \
    lpr_thread.h \
    video_frame_bundle.h \
    image_widget.h \
    video_control_bar.h \
    video_progress_widget.h \
    lpr_config_widget.h \
    runtime_config.h

RESOURCES += \
    resources.qrc
