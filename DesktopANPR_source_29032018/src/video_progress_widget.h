#ifndef VIDEO_PROGRESS_WIDGET_H
#define VIDEO_PROGRESS_WIDGET_H

#include <QWidget>
#include <QLabel>

class VideoProgressWidget : public QWidget
{
    Q_OBJECT

    public:
        VideoProgressWidget( QWidget* parent = nullptr, Qt::WindowFlags flags = 0 );

    protected:
        void paintEvent( QPaintEvent* event );
        void resizeEvent( QResizeEvent* event );

    private:
        qint64      mCurrentTimeMs;
        qint64      mLengthMs;
        QLabel*     mCurrentTimeLabel;
        QLabel*     mLengthLabel;

    public slots:
        void onTimeChanged( qint64 time_ms );
        void onLengthChanged( qint64 length_ms );
};

#endif // VIDEO_PROGRESS_WIDGET_H
