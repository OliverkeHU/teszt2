#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QPointF>

class ImageWidget : public QWidget
{
    Q_OBJECT

    public:
        ImageWidget( QImage image, QWidget *parent = nullptr, Qt::WindowFlags flags = 0 );
        QImage image() const;
        void setImage( const QImage& image );
        Qt::TransformationMode transformationMode() const;
        void setTransformationMode( Qt::TransformationMode transformationMode );

    signals:
        void imageChanged( QImage image );

    protected:
        void paintEvent( QPaintEvent* event );

    private:
        QImage                  mImage;
        QImage                  mScaledImage;
        QPointF                 mResizeFactors;
        Qt::TransformationMode  mTransformationMode;
        bool                    mForceRecalculation;
};

#endif // IMAGE_WIDGET_H
