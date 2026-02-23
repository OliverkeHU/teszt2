#include "video_progress_widget.h"

#include <QPainter>
#include <QDateTime>
#include <QPalette>
#include <QStyleOption>

VideoProgressWidget::VideoProgressWidget( QWidget* parent, Qt::WindowFlags flags )
: QWidget( parent, flags )
, mCurrentTimeMs( 0 )
, mLengthMs( 0 )
, mCurrentTimeLabel( new QLabel( "", this ) )
, mLengthLabel( new QLabel( "", this ) )
{
    mCurrentTimeLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );
    mLengthLabel->setAlignment( Qt::AlignTop | Qt::AlignRight );
    setObjectName( "VideoProgress" );
}

void VideoProgressWidget::paintEvent( QPaintEvent* /*event*/ )
{
    QStyleOption opt;
    opt.init(this);
    QPainter p( this );
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    int h = height() / 5;
    p.drawRect( 0, 0, width()-1, h );

    if( ( mLengthMs > 0 ) && ( mLengthMs > mCurrentTimeMs ) )
    {
        qreal fact = static_cast<qreal>( width() ) / mLengthMs;
        int progress_width = static_cast<int>( fact * mCurrentTimeMs );
        p.fillRect( 2, 2, progress_width-4, h-3, palette().color( QPalette::Foreground ) );
    }
}

void VideoProgressWidget::resizeEvent( QResizeEvent* /*event*/ )
{
    int w = std::max( 100, width() / 10 );
    int h = static_cast<int>( 2 * ( height() / 3.0 ) );

    mCurrentTimeLabel->setGeometry( 0, height() - h, w, h );
    mLengthLabel->setGeometry( width() - w, height() - h, w, h );
}

void VideoProgressWidget::onTimeChanged( qint64 time_ms )
{
    if( mCurrentTimeMs != time_ms )
    {
        mCurrentTimeMs = time_ms;
        mCurrentTimeLabel->setText( QDateTime::fromMSecsSinceEpoch( mCurrentTimeMs ).toString( "mm:ss" ) );
        mLengthLabel->setText( QDateTime::fromMSecsSinceEpoch( mLengthMs ).toString( "mm:ss" ) );
        repaint();
    }
}

void VideoProgressWidget::onLengthChanged( qint64 length_ms )
{
    if( mLengthMs != length_ms )
    {
        mLengthMs = length_ms;
        onTimeChanged( std::min( mLengthMs, mCurrentTimeMs ) );
        repaint();
    }
}
