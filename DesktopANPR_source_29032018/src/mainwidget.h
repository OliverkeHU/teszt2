#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolBar>

#include "video_widget.h"
#include "reading_list.h"
#include "lpr_thread.h"
#include "lpr_config_widget.h"

class MainWidget : public QWidget
{
    Q_OBJECT

    public:
        MainWidget(QWidget *parent = 0);
        ~MainWidget();

    private:
        enum class SourceFileType
        {
            VIDEO
        ,   STILL_IMAGE
        };


        void activateImageWidget();
        void activateVideoWidget();

        LprThread*          mLprThread;
        QHBoxLayout*        mLayout;
        QWidget*            mRightWidget;
        QVBoxLayout*        mRightLayout;
        VideoWidget*        mVideoWidget;
        ImageWidget*        mImageWidget;
        ReadingList*        mReadingList;
        QToolBar*           mToolBar;
        LprConfigWidget*    mLprConfigWidget;
        QString             mNextFileToOpen;
        SourceFileType      mTypeOfNextFileToOpen;

    public slots:
        void onCriticalError( const QString& title, const QString& message );

    private slots:
        void onLprEngineResetNeeded();
        void onOpenVideoTriggered();
        void onOpenImageTriggered();
        void onLprConfigDismissed();
};

#endif // MAINWIDGET_H
