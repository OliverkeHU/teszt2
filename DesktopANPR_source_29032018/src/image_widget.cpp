#include "image_widget.h"

#include <QPainter>

ImageWidget::ImageWidget( QImage image, QWidget *parent, Qt::WindowFlags flags )
: QWidget( parent, flags )
, mImage( image )
, mTransformationMode( Qt::SmoothTransformation )
, mForceRecalculation( true )
{}

QImage ImageWidget::image() const
{ return mImage; }

void ImageWidget::setImage( const QImage& image )
{
    mImage = image;
    mForceRecalculation = true;
    repaint();
    emit imageChanged( mImage );
}

Qt::TransformationMode ImageWidget::transformationMode() const
{ return mTransformationMode; }

void ImageWidget::setTransformationMode( Qt::TransformationMode transformationMode )
{
    if( transformationMode != mTransformationMode )
    {
        mTransformationMode = transformationMode;
        repaint();
    }
}

void ImageWidget::paintEvent( QPaintEvent* /*event*/ )
{
    if( !mImage.isNull() )
    {
        QPointF resize_factors( width() / static_cast<qreal>( mImage.width() )
                              , height() / static_cast<qreal>( mImage.height() )
                              );

        if( mForceRecalculation || ( resize_factors != mResizeFactors ) )
        {
            mResizeFactors = resize_factors;
            mScaledImage = ( resize_factors.x() < resize_factors.y() ) ? mImage.scaledToWidth( width(), mTransformationMode ) : mImage.scaledToHeight( height(), mTransformationMode );
            mForceRecalculation = false;
        }

        QPainter p( this );
        p.drawImage( ( width() - mScaledImage.width() ) / 2, ( height() - mScaledImage.height() ) / 2, mScaledImage );
    }
}
