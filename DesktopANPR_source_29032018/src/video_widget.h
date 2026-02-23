#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

#include <QWidget>
#include <QAbstractVideoSurface>
#include <QMediaPlayer>

#include "video_frame_bundle.h"
#include "video_control_bar.h"
#include "video_progress_widget.h"

class AbstractVideoSurfaceReceiver
{
    public:
        virtual ~AbstractVideoSurfaceReceiver() {}
        virtual bool present(const QVideoFrame &frame) = 0;
};

class VideoSurfaceProxy : public QAbstractVideoSurface
{
    Q_OBJECT

    public:
        VideoSurfaceProxy( AbstractVideoSurfaceReceiver* receiver, QObject* parent );
        AbstractVideoSurfaceReceiver* receiver() const;
        void setReceiver( AbstractVideoSurfaceReceiver* receiver );

    private:
        AbstractVideoSurfaceReceiver*   mReceiver;

    // QAbstractVideoSurface interface
    public:
        QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const override;
        bool present(const QVideoFrame &frame) override;
};

class VideoWidget : public QWidget, public AbstractVideoSurfaceReceiver, public VideoFrameBundleProvider
{
    Q_OBJECT

    public:
        VideoWidget( QWidget* parent = nullptr, Qt::WindowFlags flags = 0 );
        ~VideoWidget();
        bool present(const QVideoFrame &frame) override;
        VideoFrameBundle getFrameBundle() override;
        void openVideoFile( const QString& file_name );

    protected:
        void paintEvent( QPaintEvent* event );
        void resizeEvent( QResizeEvent* event );

    private:
        void initLpr();

        QMediaPlayer*         mMediaPlayer;
        VideoSurfaceProxy*    mVideoSurfaceProxy;
        VideoFrameBundle      mCurrentFrameBundle;
        VideoControlBar*      mVideoControlBar;
        VideoProgressWidget*  mVideoProgressWidget;
        bool                  mAutoStartOnReady;

    public slots:
        void onPlayClicked();
        void onPauseClicked();
        void onStopClicked();

    private slots:
        void onMediaStatusChanged( QMediaPlayer::MediaStatus status );

    signals:
        void currentFrameChanged( VideoFrameBundleProvider* invoker );
        void lprEngineResetNeeded();
};

#endif // VIDEO_WIDGET_H
