#include "video_control_bar.h"
#include <QIcon>

VideoControlBar::VideoControlBar( QWidget* parent, Qt::WindowFlags flags )
: QWidget( parent, flags )
, mLayout( new QHBoxLayout( this ) )
, mPlay( new QPushButton( QIcon( ":images/control_play.png" ), "", this ) )
, mPause( new QPushButton( QIcon( ":images/control_pause.png" ), "", this ) )
, mStop( new QPushButton( QIcon( ":images/control_stop.png" ), "", this ) )
{
    mLayout->addWidget( mPlay );
    mLayout->addWidget( mPause );
    mLayout->addWidget( mStop );

    connect( mPlay, &QPushButton::clicked, this, &VideoControlBar::buttonClicked );
    connect( mPause, &QPushButton::clicked, this, &VideoControlBar::buttonClicked );
    connect( mStop, &QPushButton::clicked, this, &VideoControlBar::buttonClicked );
}

void VideoControlBar::buttonClicked()
{
    QPushButton* sender = dynamic_cast<QPushButton*>( QObject::sender() );
    if( sender == mPlay )
    { emit playClicked(); }
    else if( sender == mPause )
    { emit pauseClicked(); }
    else if( sender == mStop )
    { emit stopClicked(); }
}
