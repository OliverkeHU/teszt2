#ifndef READING_LIST_H
#define READING_LIST_H

#include <QListWidget>
#include <QGridLayout>
#include <QLabel>

#include "license_plate_reader.h"
#include "image_widget.h"

class ReadingListItem : public QWidget
{
    Q_OBJECT

    public:
        ReadingListItem( const AnprResult& reading, QWidget* parent = nullptr, Qt::WindowFlags flags = 0 );

    private:
        QString         mText;
        AnprDirection   mDirection;
        qint64          mTimestamp;

        QLabel*         mTextLabel;
        ImageWidget*    mDirectionImage;
        QLabel*         mDirectionLabel;
        QLabel*         mTimestampLabel;
        QGridLayout*    mLayout;
};

class ReadingList : public QListWidget
{
    Q_OBJECT

    public:
        ReadingList( QWidget* parent = nullptr );

    public slots:
        void onLicencsePlateDetected( AnprResult detection_info );
};

#endif // READING_LIST_H
