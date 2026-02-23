#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QImage>
#include <QDebug>

#include "../minimal_data/HAData.h"
#include "../lpreader/LPReader.h"
#include "../lpreader/MipMapper.h"
#include "../lpreader/NetImage.h"

static void print_json_error(const QString& msg)
{
    QByteArray out = QString("{\"ok\":false,\"error\":%1}\n")
            .arg(QString("\"%1\"").arg(msg.toHtmlEscaped().replace("\"","\\\"")))
            .toUtf8();
    fwrite(out.constData(), 1, (size_t)out.size(), stdout);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("arh_anpr_cli");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("ARH DesktopANPR headless CLI wrapper (JSON stdout)");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption datOpt(QStringList() << "d" << "dat", "Path to lpr.lprdat", "path");
    QCommandLineOption imgOpt(QStringList() << "i" << "image", "Path to input image (jpg/png/bmp)", "path");
    QCommandLineOption minCharOpt(QStringList() << "minchar", "Minimum character height in pixels (default 32)", "n", "32");
    QCommandLineOption maxCharOpt(QStringList() << "maxchar", "Maximum character height in pixels (default 128)", "n", "128");
    parser.addOption(datOpt);
    parser.addOption(imgOpt);
    parser.addOption(minCharOpt);
    parser.addOption(maxCharOpt);

    parser.process(a);

    const QString datPath = parser.value(datOpt);
    const QString imgPath = parser.value(imgOpt);

    if(datPath.isEmpty() || imgPath.isEmpty())
    {
        print_json_error("Missing --dat or --image argument");
        return 2;
    }

    if(!QFileInfo::exists(datPath))
    {
        print_json_error("lpr.lprdat not found: " + datPath);
        return 3;
    }
    if(!QFileInfo::exists(imgPath))
    {
        print_json_error("image not found: " + imgPath);
        return 4;
    }

    // Load image
    QImage img(imgPath);
    if(img.isNull())
    {
        print_json_error("Unable to load image: " + imgPath);
        return 5;
    }

    // Convert to 8-bit grayscale
    QImage gray = img.convertToFormat(QImage::Format_Grayscale8);

    // Load data
    CHADataManager lprdataman;
    CHAData *root = lprdataman.GetRoot();
    if(!lprdataman.LoadFromFile(datPath.toUtf8().constData()))
    {
        print_json_error("Unable to load dat file");
        return 6;
    }

    // Init reader
    CLPReader reader;
    if(!reader.Init(root))
    {
        print_json_error("CLPReader init failed");
        return 7;
    }

    bool okMin=false, okMax=false;
    int minChar = parser.value(minCharOpt).toInt(&okMin);
    int maxChar = parser.value(maxCharOpt).toInt(&okMax);
    if(!okMin) minChar = 32;
    if(!okMax) maxChar = 128;

    CMipMapper mip;
    CNetImage net;

    // Map grayscale buffer
    net.CreateMap((unsigned int)gray.width(), (unsigned int)gray.height(), (unsigned int)gray.bytesPerLine(), gray.bits());
    mip.SetSrcImage(&net);
    reader.SetMipMapper(&mip);
    reader.SetDetectionArea(nullptr);
    reader.SetCharSizes((unsigned int)minChar, (unsigned int)maxChar);

    // Run
    reader.ProcessImage();

    // Emit JSON
    // NOTE: plates_ contains geometry + fulltext_ (best guess). score is not exposed; we set 1.0.
    QByteArray out;
    out += "{\"ok\":true,\"plates\":[";
    for(size_t i=0; i<reader.plates_.size(); i++)
    {
        const auto &p = reader.plates_[i];
        if(i>0) out += ",";
        QString txt = QString::fromLatin1(p.fulltext_).trimmed();

        out += "{\"text\":";
        out += QString("\"%1\"").arg(txt.toHtmlEscaped().replace("\"","\\\"")).toUtf8();
        out += ",\"score\":1.0,\"quad\":[";
        for(int k=0; k<4; k++)
        {
            if(k>0) out += ",";
            // frame_ points are float in image coordinates
            out += "[";
            out += QByteArray::number(p.frame_[k].x_);
            out += ",";
            out += QByteArray::number(p.frame_[k].y_);
            out += "]";
        }
        out += "]}";
    }
    out += "]}\n";

    fwrite(out.constData(), 1, (size_t)out.size(), stdout);
    return 0;
}
