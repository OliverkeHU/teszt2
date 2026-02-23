#ifndef VIDEO_FRAME_BUNDLE_H
#define VIDEO_FRAME_BUNDLE_H

#include <QImage>

struct VideoFrameBundle
{
    QImage     frame;
    qint64     timestamp_ms;
};

class VideoFrameBundleProvider
{
    public:
        virtual ~VideoFrameBundleProvider() {}
        virtual VideoFrameBundle getFrameBundle() = 0;
};

#endif // VIDEO_FRAME_BUNDLE_H
