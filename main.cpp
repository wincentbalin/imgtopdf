#include <QCoreApplication>
#include <QTextStream>
#include <QImage>
#include <QImageReader>
#include <QPrinter>
#include <QPainter>


void printSupportedFormats(QTextStream &out)
{
    out << "Supported formats:";
    foreach(QByteArray format, QImageReader::supportedImageFormats())
        out << format;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // As in http://doc.qt.io/qt-4.8/qt-xml-xmlstreamlint-main-cpp.html
    enum ExitCode
    {
        Success,
        ParseFailure,
        ArgumentError,
        WriteError,
        FileFailure
    };

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (argc < 3)
    {
        err << "Usage:";
        err << "\timgtopdf input.img output.pdf";
        return ArgumentError;
    }

    // TODO Process options

    /*
      Some ideas about the options:
      - page size (A4, Letter, etc.)
      - list image formats (method already here)
      - resolution (default one is for screenshots, but what about photographs?);
        how do we specify resolution? 600 [dpi], 300x300 [dpi], 8m [egapixel];
        in the last case calculate resolution and orientation depending on image's resolution and page size
      - fit images to page; default mode is to draw directly; in direct relation to previous option
      - should we probably let the utility determine resolution automatically?
      - derive output PDF file name from image file name
      - building upon previous idea: try to glob input filename and process multiple inputs
      - special option: unify (concatenate all pages to a single PDF file)
      - output directory, in case that we convert images directly from camera
      - center: center images, instead of moving them to the top left corner
      */

    // Convert image to PDF
    const QString inputFileName(QCoreApplication::arguments().at(1));
    const QString outputFileName(QCoreApplication::arguments().at(2));
    const QPrinter::PageSize pageSize = QPrinter::A4;
    const QPrinter::Orientation pageOrientation = QPrinter::Landscape;

    QImage image(inputFileName);
    if (image.isNull())
    {
        err << "Could not load image" << inputFileName;
        return FileFailure;
    }

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFileName(outputFileName);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setColorMode(QPrinter::Color);
    printer.setPaperSize(pageSize);
    printer.setOrientation(pageOrientation);

    bool result;
    QPainter painter;

    result = painter.begin(&printer);
    if (!result)
    {
        err << "Could not start conversion";
        return FileFailure;
    }

    painter.drawImage(QPoint(0, 0), image);

    result = painter.end();
    if (!result)
    {
        err << "Could not end conversion";
        return FileFailure;
    }


    return Success;
}
