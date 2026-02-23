#include "lpr_thread.h"

#include <QMutexLocker>
#include <QDebug>

#include "runtime_config.h"

static const unsigned long STOP_TIMEOUT_MSECS = 2000u;

LprThread::LprThread( QObject* parent )
: QThread( parent )
, mStopFlag( false )
, mMinCharSize( 10 )
, mMaxCharSize( 60 )
{ qRegisterMetaType<AnprResult>(); }

LprThread::~LprThread()
{
    mStopFlag = true;
    mNoJobCondition.wakeAll();
    if( !wait( STOP_TIMEOUT_MSECS ) )
    {
        qWarning() << "Lpr thread timed out in destructor!";
        terminate();
    }
}

void LprThread::setCharSizes( int min_char_size_px, int max_char_size_px )
{
    mMinCharSize = min_char_size_px;
    mMaxCharSize = std::max( mMinCharSize, max_char_size_px );
}

int LprThread::minCharSizePx() const
{ return mMinCharSize; }

int LprThread::maxCharSizePx() const
{ return mMaxCharSize; }

void LprThread::run()
{
    initLpr();
    mStopFlag = false;
    while( !mStopFlag )
    {
        QMutexLocker l( &mJobMutex );
        if( mCurrentJob.frame.isNull() )
        { mNoJobCondition.wait( &mJobMutex ); }

        if( mStopFlag )
        { break; }

        if( mCurrentJob.frame.isNull() )
        {
            qWarning() << "Lpr thread found null image";
            continue;
        }

        { // Get Y plane from QImage
            int px_count = mCurrentJob.frame.width() * mCurrentJob.frame.height();
            mYImageBuffer.resize( px_count );
            QRgb* pixels = reinterpret_cast<QRgb*>( mCurrentJob.frame.bits() );
            QRgb* pixels_end = pixels + px_count;
            uint8_t* y_plane = mYImageBuffer.data();
            while( pixels != pixels_end ) { *y_plane++ = qGray( *pixels++ ); }
        }

        { // Process
            AnprImage img;
            img.data = mYImageBuffer.data();
            img.width = static_cast<size_t>( mCurrentJob.frame.width() );
            img.height = static_cast<size_t>( mCurrentJob.frame.height() );
            img.scanline = static_cast<size_t>( mCurrentJob.frame.width() );

            auto result = mLpr.Process( img, mCurrentJob.timestamp_ms );
            if( result )
            {
                for( const auto& item : *result )
                { emit licensePlateDetected( item ); }
            }
        }
        mCurrentJob.frame = QImage();
    }
}

void LprThread::initLpr()
{
    bool lpr_init_result = mLpr.Initialize( RuntimeConfig::value( "LprDatFile" ).toString().toStdString() );
    qInfo() << "Lpr init: " << ( lpr_init_result ? "Ok" : "Failed" );

    if( lpr_init_result )
    {
        AnprConfiguration config;
        config.enableEventResend = false;
        config.minimumCharacterSize = mMinCharSize;
        config.maximumCharacterSize = mMaxCharSize;
        config.directionFilter = AnprDirection::ANY;
        mLpr.Configure( config );
    }
    else
    { emit criticalErrorOccured( "Desktop ANPR unable to operate", "ANPR engine initialization failed!" ); }
}

void LprThread::onFrameReady( VideoFrameBundleProvider* invoker )
{
    if( mJobMutex.tryLock( 0 ) && invoker )
    {
        mCurrentJob = invoker->getFrameBundle();
        mNoJobCondition.wakeAll();
        mJobMutex.unlock();
    }
}

void LprThread::onImageReady( const QImage& image )
{
    if( mJobMutex.tryLock( 0 ) && !image.isNull() )
    {
        VideoFrameBundle frameBundle = VideoFrameBundle{ image, -1 };
        mCurrentJob = frameBundle;
        mNoJobCondition.wakeAll();
        mJobMutex.unlock();
    }
}
