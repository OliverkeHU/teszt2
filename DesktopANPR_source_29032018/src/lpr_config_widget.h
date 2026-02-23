#ifndef LPR_CONFIG_WIDGET_H
#define LPR_CONFIG_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QSpinBox>
#include <QFormLayout>
#include <QPushButton>

class LprConfigWidget : public QWidget
{
    Q_OBJECT

    public:
        LprConfigWidget( int min_char_size, int max_char_size, QWidget* parent = nullptr, Qt::WindowFlags flags = 0 );
        int minimumCharacterSize() const;
        int maximumCharacterSize() const;

    private:
        QSlider*     mMaxSlider;
        QSpinBox*    mMaxSpinBox;
        QSlider*     mMinSlider;
        QSpinBox*    mMinSpinBox;
        QPushButton* mOkButton;
        QFormLayout* mLayout;
        int          mMinimumCharacterSize;
        int          mMaximumCharacterSize;

    private slots:
        void onMinChanged( int new_val );
        void onMaxChanged( int new_val );
        void onOkClicked();

    signals:
        void dismissed();
};

#endif // LPR_CONFIG_WIDGET_H
