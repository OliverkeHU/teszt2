#ifndef LPR_THREAD_H
#define LPR_THREAD_H

#include <vector>

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "license_plate_reader.h"
#include "video_frame_bundle.h"

Q_DECLARE_METATYPE( AnprResult )

class LprThread : public QThread
{
    Q_OBJECT

    public:
        LprThread( QObject* parent = nullptr );
        ~LprThread();
        void setCharSizes( int min_char_size_px, int max_char_size_px );
        int minCharSizePx() const;
        int maxCharSizePx() const;

    protected:
        void run();

    private:
        void initLpr();

        VideoFrameBundle     mCurrentJob;
        std::vector<uint8_t> mYImageBuffer;
        QMutex               mJobMutex;
        QWaitCondition       mNoJobCondition;
        bool                 mStopFlag;
        LicensePlateReader   mLpr;
        int                  mMinCharSize;
        int                  mMaxCharSize;

    public slots:
        void onFrameReady( VideoFrameBundleProvider* invoker );
        void onImageReady( const QImage& image );

    signals:
        void licensePlateDetected( AnprResult detection_info );
        void criticalErrorOccured( const QString& title, const QString& message );
};

#endif // LPR_THREAD_H
