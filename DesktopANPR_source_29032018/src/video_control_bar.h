#ifndef VIDEO_CONTROL_BAR_H
#define VIDEO_CONTROL_BAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>

class VideoControlBar : public QWidget
{
    Q_OBJECT

    public:
        VideoControlBar( QWidget* parent = nullptr, Qt::WindowFlags flags = 0 );

    private:
        QHBoxLayout*        mLayout;
        QPushButton*        mPlay;
        QPushButton*        mPause;
        QPushButton*        mStop;

    private slots:
        void buttonClicked();

    signals:
        void playClicked();
        void pauseClicked();
        void stopClicked();
};

#endif // VIDEO_CONTROL_BAR_H
