#include <QCoreApplication>
#include <QTextStream>
#include <QThread>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QPrinter>
#include <QPainter>
#include <QFuture>


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

    enum ConversionScenario
    {
        ConvertSingleScreenshot,
        ConvertMultipleScreenshots,
        ConvertAndUnifyMultipleImages,
        ConvertMultiplePhotos
    };

    QTextStream out(stdout);
    QTextStream err(stderr);

    out << "Hello!";
    if (argc < 2)
    {
        err << "Usage:";
        err << "\timgtopdf input.img output.pdf";
//        return ArgumentError;
    }

    // TODO Process options
    bool verbose = false;
    bool listFormatsAndExit = false;
    QStringList fileNames;
    int multiProcessingThreads = QThread::idealThreadCount();

    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.length(); i++)
    {
        QString arg = args.at(i);

        if (arg == "-v" || arg == "--verbose")
            verbose = true;
        else if (arg == "-l" || arg == "--list" || arg == "--list-formats")
            listFormatsAndExit = true;
        else if (arg == "-s" || arg == "--singlethreaded")
            multiProcessingThreads = 1;
        else
            fileNames.append(arg);
    }

    /*
      Some ideas about the options:
      - page size (A4, Letter, etc.)
      - list image formats (method already here)
      - heuristics: calculate image size with requested resolution; if size is greater that the page in any dimension, perform scaling
      - resolution (default one is for screenshots, but what about larger resolutions?); how do we specify resolution? 600 [dpi]
      - derive output PDF file name from image file name
      - building upon previous idea: try to glob input filename and process multiple inputs
      - special option: unify (concatenate all pages to a single PDF file)
      - output directory, in case that we convert images directly from camera
      - center: center images, instead of moving them to the top left corner
      - use multiple processors by default; a special option for single processors
      */

    // Convert image to PDF
    QString inputFileName(QCoreApplication::arguments().at(1));

    const QPrinter::PageSize pageSize = QPrinter::A4;
    const QPrinter::Orientation pageOrientation = QPrinter::Landscape;
    QPrinter::OutputFormat outputFormat = QPrinter::PdfFormat;
    const QPrinter::ColorMode colorMode = QPrinter::Color;

    QStringList expandedFileNames;
    foreach (QString fileName, fileNames)
    {
        const QString parentPath = QFileInfo(fileName).absolutePath();
        QStringList pathStack;
        pathStack.append(parentPath);

        while (!pathStack.isEmpty())
        {
            QDir dir(pathStack.takeLast());
            dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDot | QDir::NoDotDot);
            out << dir.absolutePath();

            foreach (QFileInfo fi, dir.entryInfoList())
            {
                const QString absPath = fi.absoluteFilePath();
                out << "absPath" << absPath;
                if (fi.isFile() && QDir::match(fileName, absPath))
                {
                    expandedFileNames.append(absPath);
                    if (verbose)
                        out << "Found" << absPath;
                }
                else if (fi.isDir())
                    pathStack.append(absPath);
            }
        }
    }
    fileNames = expandedFileNames;

    QString resultingFileName;
    if (fileNames.last().endsWith(".pdf", Qt::CaseInsensitive))
    {
        resultingFileName = fileNames.last();
        fileNames.removeLast();
        outputFormat = QPrinter::PdfFormat;
    }
    else if (fileNames.last().endsWith(".ps", Qt::CaseInsensitive))
    {
        resultingFileName = fileNames.last();
        fileNames.removeLast();
        outputFormat = QPrinter::PostScriptFormat;
    }


    QImage image(inputFileName);
    if (image.isNull())
    {
        err << "Could not load image" << inputFileName;
        return FileFailure;
    }

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFileName(resultingFileName);
    printer.setOutputFormat(outputFormat);
    printer.setColorMode(colorMode);
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
