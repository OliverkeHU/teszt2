#include "reading_list.h"

#include <QDateTime>

const char* anprDirectionToString( AnprDirection dir )
{
    switch( dir )
    {
        case AnprDirection::UNDEFINED: return "Undefined";
        case AnprDirection::APPROACHING: return "Approaching";
        case AnprDirection::MOVING_AWAY: return "Moving away";
    }
    return "--INVALID--";
}

QImage anprDirectionToDirectionIcon( AnprDirection dir )
{
    switch( dir )
    {
        case AnprDirection::APPROACHING: return QImage( ":/images/dir_appr.png" );
        case AnprDirection::MOVING_AWAY: return QImage( ":/images/dir_mova.png" );
        case AnprDirection::UNDEFINED:
        default: return QImage( ":/images/dir_und.png" );
    }
}

ReadingListItem::ReadingListItem( const AnprResult& reading, QWidget* parent, Qt::WindowFlags flags )
: QWidget( parent, flags )
, mText( QString::fromStdString( reading.text ) )
, mDirection( reading.direction )
, mTimestamp( reading.timestamp )
, mTextLabel( new QLabel( QString::fromStdString( reading.text ), this ) )
, mDirectionImage( new ImageWidget( anprDirectionToDirectionIcon( reading.direction ), this ) )
, mDirectionLabel( new QLabel( anprDirectionToString( reading.direction ), this ) )
, mTimestampLabel( new QLabel( QDateTime::fromMSecsSinceEpoch( mTimestamp ).toString( "mm:ss.zzz" ) , this ) )
, mLayout( new QGridLayout( this ) )
{
    mTextLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
    mTextLabel->setObjectName( "LpLabel" );
    mDirectionLabel->setAlignment( Qt::AlignCenter );
    mDirectionLabel->setObjectName( "LpDirTxt" );
    mTimestampLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
    mTimestampLabel->setObjectName( "LpTs" );
    if( mTimestamp < 0 ) // still image assumed
    { mTimestampLabel->setText( "n.a." ); }

    // grid layout, 3 rows, 3 columns
    mLayout->addWidget( mTextLabel, 0, 0, 2, 2 );
    mLayout->addWidget( mDirectionImage, 0, 2, 2, 1 );
    mLayout->addWidget( mDirectionLabel, 2, 2, 1, 1 );
    mLayout->addWidget( mTimestampLabel, 2, 0, 1, 2 );
    mLayout->setSpacing( 0 );

    setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum ) );
}

ReadingList::ReadingList( QWidget* parent )
: QListWidget( parent )
{}

void ReadingList::onLicencsePlateDetected( AnprResult detection_info )
{
    ReadingListItem* item_widget = new ReadingListItem( detection_info );
    insertItem( 0, "" );
    auto new_item = itemAt( 0, 0 );
    new_item->setSizeHint( QSize( 150, 80 ) );
    setItemWidget( new_item, item_widget );
}
