QT       += core gui
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app
TARGET = arh_anpr_cli

INCLUDEPATH += ..

SOURCES += \
    main.cpp \
    ../license_plate_reader.cpp \
    ../logger.cpp \
    ../runtime_config.cpp \
    ../lpreader/ANPROCR.cpp \
    ../lpreader/BinRLE.cpp \
    ../lpreader/ImageTransform.cpp \
    ../lpreader/LPReader.cpp \
    ../lpreader/LPTracker.cpp \
    ../lpreader/MipMapper.cpp \
    ../lpreader/MotionFilterRLE.cpp \
    ../lpreader/NetImage.cpp \
    ../lpreader/NeuralNet_f16_16.cpp \
    ../lpreader/PoligonDraw.cpp \
    ../lpreader/SharedNetValueBuffer.cpp \
    ../minimal_data/HAData.cpp

HEADERS += \
    ../license_plate_reader.h \
    ../logger.h \
    ../runtime_config.h \
    ../lpreader/ANPROCR.h \
    ../lpreader/BinRLE.h \
    ../lpreader/ErrorVal.h \
    ../lpreader/ImageTransform.h \
    ../lpreader/LPReader.h \
    ../lpreader/LPTracker.h \
    ../lpreader/MipMapper.h \
    ../lpreader/MotionFilterRLE.h \
    ../lpreader/NetImage.h \
    ../lpreader/nettypes.h \
    ../lpreader/NeuralNet_f16_16.h \
    ../lpreader/PoligonDraw.h \
    ../lpreader/SharedNetValueBuffer.h \
    ../minimal_data/HAData.h
