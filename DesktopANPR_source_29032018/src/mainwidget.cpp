#include "mainwidget.h"

#include <QFile>
#include <QAction>
#include <QIcon>
#include <QFileDialog>
#include <QMessageBox>

MainWidget::MainWidget( QWidget *parent )
: QWidget( parent )
, mLprThread( new LprThread( this ) )
, mLayout( new QHBoxLayout( this ) )
, mRightWidget( new QWidget( this ) )
, mRightLayout( new QVBoxLayout( mRightWidget ) )
, mVideoWidget( new VideoWidget( this ) )
, mImageWidget( new ImageWidget( QImage(), this ) )
, mReadingList( new ReadingList( this ) )
, mToolBar( new QToolBar( this ) )
, mLprConfigWidget( nullptr )
, mTypeOfNextFileToOpen( SourceFileType::VIDEO )
{
    connect( mVideoWidget, &VideoWidget::currentFrameChanged, mLprThread, &LprThread::onFrameReady );
    connect( mVideoWidget, &VideoWidget::lprEngineResetNeeded, this, &MainWidget::onLprEngineResetNeeded );
    connect( mImageWidget, &ImageWidget::imageChanged, mLprThread, &LprThread::onImageReady );
    connect( mLprThread, &LprThread::licensePlateDetected, mReadingList, &ReadingList::onLicencsePlateDetected, Qt::QueuedConnection );
    connect( mLprThread, &LprThread::criticalErrorOccured, this, &MainWidget::onCriticalError, Qt::QueuedConnection );

    mLayout->addWidget( mVideoWidget, 3 );
    mLayout->addWidget( mImageWidget, 3 );
    mImageWidget->hide();
    mLayout->addWidget( mRightWidget, 1 );

    mRightLayout->addWidget( mToolBar, 3 );
    mRightLayout->addWidget( mReadingList, 7 );

    mToolBar->setMovable( false );
    mToolBar->setFloatable( false );
    mToolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );

    QAction* open_action = mToolBar->addAction( QIcon( ":/images/open_file_vid.png" ), "Open video" );
    QAction* open_action_file = mToolBar->addAction( QIcon( ":/images/open_file_img.png" ), "Open image" );
    connect( open_action, &QAction::triggered, this, &MainWidget::onOpenVideoTriggered );
    connect( open_action_file, &QAction::triggered, this, &MainWidget::onOpenImageTriggered );

    resize( 1024, 768 );
    mLprThread->start();

    { // Load stylesheet
        QFile styleFile( ":/styles/main.qss" );
        if( styleFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            setStyleSheet( styleFile.readAll() );
            styleFile.close();
        }
        else
        { qInfo() << "NO STYLE"; }
    }
}

MainWidget::~MainWidget()
{}

void MainWidget::activateImageWidget()
{
    mVideoWidget->onStopClicked();
    mVideoWidget->hide();
    mImageWidget->show();
}

void MainWidget::activateVideoWidget()
{
    mImageWidget->hide();
    mVideoWidget->show();
}

void MainWidget::onCriticalError( const QString& title, const QString& message )
{
    QMessageBox::critical( nullptr, title, message );
    close();
}

void MainWidget::onLprEngineResetNeeded()
{
    if( mLprThread )
    {
        disconnect( mVideoWidget, &VideoWidget::currentFrameChanged, mLprThread, &LprThread::onFrameReady );
        disconnect( mImageWidget, &ImageWidget::imageChanged, mLprThread, &LprThread::onImageReady );
        disconnect( mLprThread, &LprThread::licensePlateDetected, mReadingList, &ReadingList::onLicencsePlateDetected );
        mLprThread->deleteLater();
    }

    mReadingList->clear();

    mLprThread = new LprThread( this );
    connect( mVideoWidget, &VideoWidget::currentFrameChanged, mLprThread, &LprThread::onFrameReady );
    connect( mImageWidget, &ImageWidget::imageChanged, mLprThread, &LprThread::onImageReady );
    connect( mLprThread, &LprThread::licensePlateDetected, mReadingList, &ReadingList::onLicencsePlateDetected, Qt::QueuedConnection );
    mLprThread->start();
}

void MainWidget::onOpenVideoTriggered()
{
    mNextFileToOpen = QFileDialog::getOpenFileName( this
                                                  , "Open video file"
                                                  , QDir::homePath()
                                                  , "Video files (*.mp4 *.MP4 *.mkv *.MKV *.mpg *.MPG *.mpeg *.MPEG *.avi *.AVI)"
                                                  , nullptr
                                                  , QFileDialog::ReadOnly
                                                  );
    if( mNextFileToOpen.isNull() ) // cancel clicked
    { return; }

    activateVideoWidget();
    mTypeOfNextFileToOpen = SourceFileType::VIDEO;

    mLprConfigWidget = new LprConfigWidget( mLprThread->minCharSizePx(), mLprThread->maxCharSizePx(), this, Qt::Dialog );
    mLprConfigWidget->setWindowModality( Qt::ApplicationModal );
    QPoint pos = mapToGlobal( QPoint( ( width() / 4 ), ( height() / 5 )  ) );
    mLprConfigWidget->setGeometry( pos.x(), pos.y(), 2 * ( width() / 4 ), std::min( 200, 3 * ( height() / 5 ) ) );
    connect( mLprConfigWidget, &LprConfigWidget::dismissed, this, &MainWidget::onLprConfigDismissed );
    mLprConfigWidget->show();
}

void MainWidget::onOpenImageTriggered()
{
    mNextFileToOpen = QFileDialog::getOpenFileName( this
                                                  , "Open image file"
                                                  , QDir::homePath()
                                                  , "Image files (*.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.gif *.GIF *.bmp *.BMP)"
                                                  , nullptr
                                                  , QFileDialog::ReadOnly
                                                  );
    if( mNextFileToOpen.isNull() ) // cancel clicked
    { return; }

    activateImageWidget();
    mTypeOfNextFileToOpen = SourceFileType::STILL_IMAGE;

    mLprConfigWidget = new LprConfigWidget( mLprThread->minCharSizePx(), mLprThread->maxCharSizePx(), this, Qt::Dialog );
    mLprConfigWidget->setWindowModality( Qt::ApplicationModal );
    QPoint pos = mapToGlobal( QPoint( ( width() / 4 ), ( height() / 5 )  ) );
    mLprConfigWidget->setGeometry( pos.x(), pos.y(), 2 * ( width() / 4 ), std::min( 200, 3 * ( height() / 5 ) ) );
    connect( mLprConfigWidget, &LprConfigWidget::dismissed, this, &MainWidget::onLprConfigDismissed );
    mLprConfigWidget->show();
}

void MainWidget::onLprConfigDismissed()
{
    onLprEngineResetNeeded();
    mLprThread->setCharSizes( mLprConfigWidget->minimumCharacterSize(), mLprConfigWidget->maximumCharacterSize() );
    mLprConfigWidget->deleteLater();
    mLprConfigWidget = nullptr;

    switch( mTypeOfNextFileToOpen )
    {
        case SourceFileType::VIDEO:
            mVideoWidget->openVideoFile( mNextFileToOpen );
            break;

        case SourceFileType::STILL_IMAGE:
            mImageWidget->setImage( QImage( mNextFileToOpen ) );
            break;

        default: // Invalid source type
            qWarning() << "Internal error: source file type has not been set correctly!";
            break;
    }
}
