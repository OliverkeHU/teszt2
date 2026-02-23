#include "lpr_config_widget.h"

LprConfigWidget::LprConfigWidget( int min_char_size, int max_char_size, QWidget* parent, Qt::WindowFlags flags )
: QWidget( parent, flags )
, mMaxSlider( new QSlider( this ) )
, mMaxSpinBox( new QSpinBox( this ) )
, mMinSlider( new QSlider( this ) )
, mMinSpinBox( new QSpinBox( this ) )
, mOkButton( new QPushButton( "Ok", this ) )
, mLayout( new QFormLayout( this ) )
, mMinimumCharacterSize( min_char_size )
, mMaximumCharacterSize( max_char_size )
{
    mMaxSlider->setOrientation( Qt::Horizontal );
    mMinSlider->setOrientation( Qt::Horizontal );
    mMaxSlider->setRange( 20, 256 );
    mMaxSpinBox->setRange( 20, 256 );
    mMinSlider->setRange( 10, 246 );
    mMinSpinBox->setRange( 10, 246 );
    mMaxSlider->setValue( mMaximumCharacterSize );
    mMaxSpinBox->setValue( mMaximumCharacterSize );
    mMinSlider->setValue( mMinimumCharacterSize );
    mMinSpinBox->setValue( mMinimumCharacterSize );

    mLayout->addRow( "Minimum character size (pixels)", mMinSlider );
    mLayout->addRow( "", mMinSpinBox );
    mLayout->addRow( "Maximum character size (pixels)", mMaxSlider );
    mLayout->addRow( "", mMaxSpinBox );
    QWidget* dummy = new QWidget( this );
    dummy->setMinimumHeight( 10 );
    mLayout->addRow( "", dummy );
    mLayout->addRow( "", mOkButton );

    connect( mMinSlider, &QSlider::valueChanged, this, &LprConfigWidget::onMinChanged );
    connect( mMinSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( onMinChanged(int) ) );
    connect( mMaxSlider, &QSlider::valueChanged, this, &LprConfigWidget::onMaxChanged );
    connect( mMaxSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( onMaxChanged(int) ) );
    connect( mOkButton, &QPushButton::clicked, this, &LprConfigWidget::onOkClicked );
}

int LprConfigWidget::minimumCharacterSize() const
{ return mMinimumCharacterSize; }

int LprConfigWidget::maximumCharacterSize() const
{ return mMaximumCharacterSize; }

void LprConfigWidget::onMinChanged( int new_val )
{
    mMinimumCharacterSize = new_val;
    if( QObject::sender() != mMinSlider )
    { mMinSlider->setValue( new_val ); }
    if( QObject::sender() != mMinSpinBox )
    { mMinSpinBox->setValue( new_val ); }
}

void LprConfigWidget::onMaxChanged( int new_val )
{
    mMaximumCharacterSize = new_val;
    if( QObject::sender() != mMaxSlider )
    { mMaxSlider->setValue( new_val ); }
    if( QObject::sender() != mMaxSpinBox )
    { mMaxSpinBox->setValue( new_val ); }
}

void LprConfigWidget::onOkClicked()
{
    hide();
    close();
    emit dismissed();
}
