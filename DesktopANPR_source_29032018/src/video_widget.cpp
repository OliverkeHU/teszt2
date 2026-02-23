#include "video_widget.h"

#include <QMessageBox>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QUrl>
#include <QDebug>

VideoSurfaceProxy::VideoSurfaceProxy( AbstractVideoSurfaceReceiver* receiver, QObject* parent )
: QAbstractVideoSurface( parent )
, mReceiver( receiver )
{}

AbstractVideoSurfaceReceiver* VideoSurfaceProxy::receiver() const
{ return mReceiver; }

void VideoSurfaceProxy::setReceiver( AbstractVideoSurfaceReceiver* receiver )
{ mReceiver = receiver; }

QList<QVideoFrame::PixelFormat> VideoSurfaceProxy::supportedPixelFormats( QAbstractVideoBuffer::HandleType handleType ) const
{
    if( handleType == QAbstractVideoBuffer::NoHandle )
    {
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32
                                                 << QVideoFrame::Format_ARGB32
                                                 << QVideoFrame::Format_ARGB32_Premultiplied
                                                 << QVideoFrame::Format_RGB565
                                                 << QVideoFrame::Format_RGB555
                                                 ;
    }
    else
    { return QList<QVideoFrame::PixelFormat>(); }
}

bool VideoSurfaceProxy::present( const QVideoFrame &frame )
{
    if( mReceiver )
    { return mReceiver->present( frame ); }
    return false;
}



VideoWidget::VideoWidget( QWidget* parent, Qt::WindowFlags flags )
: QWidget( parent, flags )
, mMediaPlayer( new QMediaPlayer( this ) )
, mVideoSurfaceProxy( new VideoSurfaceProxy( this, this ) )
, mVideoControlBar( new VideoControlBar( this ) )
, mVideoProgressWidget( new VideoProgressWidget( this ) )
, mAutoStartOnReady( true )
{
    connect( mMediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &VideoWidget::onMediaStatusChanged );
    connect( mVideoControlBar, &VideoControlBar::playClicked, this, &VideoWidget::onPlayClicked );
    connect( mVideoControlBar, &VideoControlBar::pauseClicked, this, &VideoWidget::onPauseClicked );
    connect( mVideoControlBar, &VideoControlBar::stopClicked, this, &VideoWidget::onStopClicked );
    connect( mMediaPlayer, &QMediaPlayer::durationChanged, mVideoProgressWidget, &VideoProgressWidget::onLengthChanged );
    connect( mMediaPlayer, &QMediaPlayer::positionChanged, mVideoProgressWidget, &VideoProgressWidget::onTimeChanged );

    mMediaPlayer->setVideoOutput( mVideoSurfaceProxy );
    resizeEvent( nullptr );
}

VideoWidget::~VideoWidget()
{
    mMediaPlayer->stop();
    mVideoSurfaceProxy->setReceiver( nullptr );
}

void VideoWidget::paintEvent( QPaintEvent* /*event*/ )
{
    const QImage& curr_frame = mCurrentFrameBundle.frame.isNull() ? QImage( ":/images/sci.jpg" ) : mCurrentFrameBundle.frame;
    QPainter p( this );
    p.fillRect( QRect( 0, 0, width(), height() ), QColor( 0, 0, 0 ) );
    float x_fact = width() / static_cast<float>( curr_frame.width() );
    float y_fact = height() / static_cast<float>( curr_frame.height() );
    QImage scaled_image =  x_fact < y_fact ? curr_frame.scaledToWidth( width(), Qt::SmoothTransformation ) : curr_frame.scaledToHeight( height(), Qt::SmoothTransformation );
    p.drawImage( ( width() - scaled_image.width() ) / 2, ( height() - scaled_image.height() ) / 2, scaled_image );
}

void VideoWidget::resizeEvent( QResizeEvent* /*event*/ )
{
    int bar_height = height() / 8;
    int bar_width = std::min( 200, width() / 4 );
    mVideoControlBar->setGeometry(
        ( width() - bar_width ) / 2
    ,   height() - bar_height
    ,   bar_width
    ,   bar_height
    );

    int progress_height = 40;
    mVideoProgressWidget->setGeometry( 5, 5, width() - 10, progress_height );
}

bool VideoWidget::present( const QVideoFrame& frame_in )
{
    QVideoFrame frame = frame_in;

    if( frame.map(QAbstractVideoBuffer::ReadOnly) )
    {
         mCurrentFrameBundle.frame = QImage( frame.bits()
                                           , frame.width()
                                           , frame.height()
                                           , frame.bytesPerLine()
                                           , QVideoFrame::imageFormatFromPixelFormat( frame.pixelFormat() )
                                           ).copy();
         frame.unmap();
     }

    if( mCurrentFrameBundle.frame.isNull() )
    { return false; }
    else
    {
        mCurrentFrameBundle.timestamp_ms = mMediaPlayer->position();
        emit currentFrameChanged( this );

        repaint();
        return true;
    }
}

VideoFrameBundle VideoWidget::getFrameBundle()
{ return VideoFrameBundle{ mCurrentFrameBundle.frame.copy(), mCurrentFrameBundle.timestamp_ms }; }

void VideoWidget::openVideoFile( const QString& file_name )
{
    mMediaPlayer->stop();
    mAutoStartOnReady = true;
    mMediaPlayer->setMedia( QUrl::fromLocalFile( file_name ) );
}

void VideoWidget::onPlayClicked()
{ mMediaPlayer->play(); }

void VideoWidget::onPauseClicked()
{
    if( mMediaPlayer->state() == QMediaPlayer::PausedState )
    { mMediaPlayer->play(); }
    else
    { mMediaPlayer->pause(); }
}

void VideoWidget::onStopClicked()
{
    if( mMediaPlayer->state() != QMediaPlayer::StoppedState )
    {
        mMediaPlayer->stop();
        mCurrentFrameBundle.frame = QImage();
        emit lprEngineResetNeeded();
        repaint();
    }
}

void VideoWidget::onMediaStatusChanged( QMediaPlayer::MediaStatus status )
{
    qInfo() << "Media status changed: " << status;
    if( status == QMediaPlayer::LoadedMedia )
    {
        qInfo() << "Video available: " << mMediaPlayer->isVideoAvailable();

        if( mAutoStartOnReady )
        {
            mMediaPlayer->play();
            mAutoStartOnReady = false;
        }
    }
    else if( status == QMediaPlayer::InvalidMedia )
    {
        qInfo() << "Invalid media file";
        QMessageBox::information( this
                                , "Opening a file"
                                , "The selected video file cannot be found or is invalid."
                                , QMessageBox::Ok
                                );
    }
}
