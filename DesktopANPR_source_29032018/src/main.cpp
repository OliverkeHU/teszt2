#include "mainwidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFile>

#include "runtime_config.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString lprdat_file = QApplication::applicationDirPath() + QDir::separator() + "lpr.lprdat";
    qInfo() << "Dat file: " << lprdat_file;
    if( QFile::exists( lprdat_file ) )
    { RuntimeConfig::setValue( "LprDatFile", lprdat_file ); }
    else
    {
        QMessageBox::critical( nullptr
                             , "Desktop ANPR unable to operate"
                             , "Lpr data file missing\n" + ( lprdat_file )
                             );
        return 0;
    }

    MainWidget w;
    w.show();

    return a.exec();
}
