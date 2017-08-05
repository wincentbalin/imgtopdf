#include <QCoreApplication>
#include <QTextStream>
#include <QThread>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QPrinter>
#include <QPainter>


QString paperSizeToString(QPrinter::PaperSize size)
{
    switch (size)
    {
    case QPrinter::A0: return "A0"; break;
    case QPrinter::A1: return "A1"; break;
    case QPrinter::A2: return "A2"; break;
    case QPrinter::A3: return "A3"; break;
    case QPrinter::A4: return "A4"; break;
    case QPrinter::A5: return "A5"; break;
    case QPrinter::A6: return "A6"; break;
    case QPrinter::A7: return "A7"; break;
    case QPrinter::A8: return "A8"; break;
    case QPrinter::A9: return "A9"; break;
    case QPrinter::B0: return "B0"; break;
    case QPrinter::B1: return "B1"; break;
    case QPrinter::B2: return "B2"; break;
    case QPrinter::B3: return "B3"; break;
    case QPrinter::B4: return "B4"; break;
    case QPrinter::B5: return "B5"; break;
    case QPrinter::B6: return "B6"; break;
    case QPrinter::B7: return "B7"; break;
    case QPrinter::B8: return "B8"; break;
    case QPrinter::B9: return "B9"; break;
    case QPrinter::B10: return "B10"; break;
    case QPrinter::C5E: return "C5E"; break;
    case QPrinter::Comm10E: return "Comm10E"; break;
    case QPrinter::Custom: return "Custom"; break;
    case QPrinter::DLE: return "DLE"; break;
    case QPrinter::Executive: return "Executive"; break;
    case QPrinter::Folio: return "Folio"; break;
    case QPrinter::Ledger: return "Ledger"; break;
    case QPrinter::Legal: return "Legal"; break;
    case QPrinter::Letter: return "Letter"; break;
    case QPrinter::Tabloid: return "Tabloid"; break;
    }

    return "Impossible";
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

    QStringList helpOptions;
    helpOptions << "-?" << "-h" << "--help";
    if (argc < 2 ||
        (argc == 2 && helpOptions.contains(QCoreApplication::arguments().at(1))))
    {
        err << "Usage:";
        err << "\timgtopdf input.img output.pdf";
        return ArgumentError;
    }

    // TODO Process options
    bool verbose = false;
    bool listImageFormatsAndExit = false;
    bool listPageSizesAndExit = false;
    QStringList filePatterns;
    int multiProcessingThreads = QThread::idealThreadCount();

    const QPrinter::PaperSize paperSize = QPrinter::A4;
    const QPrinter::Orientation pageOrientation = QPrinter::Landscape;
    QPrinter::OutputFormat outputFormat = QPrinter::PdfFormat;
    const QPrinter::ColorMode colorMode = QPrinter::Color;

    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.length(); i++)
    {
        QString arg = args.at(i);

        if (arg == "-v" || arg == "--verbose")
            verbose = true;
        else if (arg == "--list-image-formats")
            listImageFormatsAndExit = true;
        else if (arg == "--list-page-sizes")
            listPageSizesAndExit = true;
        else if (arg == "-s" || arg == "--singlethreaded")
            multiProcessingThreads = 1;
        else if (arg.startsWith("-"))
            err << "Unknown option: " << arg << endl;
        else
            filePatterns.append(arg);
    }

    if (listImageFormatsAndExit)
    {
        out << "Supported image formats:" << endl;
        foreach(QByteArray format, QImageReader::supportedImageFormats())
            out << format << endl;
        return Success;
    }

    if (listPageSizesAndExit)
    {
        out << "Supported page sizes:" << endl;
        for (int size = QPrinter::A4; size <= QPrinter::NPaperSize; size++)  // <= is needed for QPrinter::Custom
            out << paperSizeToString(static_cast<QPrinter::PaperSize>(size)) << endl;
        return Success;
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
      - center images by default
      - use multiple processors by default; a special option for single processors
      */


    // Collect input file names
    QStringList fileNames;
    foreach (QString filePattern, filePatterns)
    {
        const QFileInfo fi(filePattern);
        const QString absPath = fi.absoluteFilePath();

        if (fi.isFile() && QImageReader(absPath).canRead())
        {
            if (verbose)
                out << "Found " << absPath << endl << flush;
            fileNames.append(absPath);
        }
        else if (fi.isDir() && fi.isExecutable())
        {
            QDir dir(filePattern);
            dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDot | QDir::NoDotDot);

            foreach (QFileInfo entryInfo, dir.entryInfoList())
            {
                const QString entryInfoAbsPath = entryInfo.absoluteFilePath();
                if (entryInfo.isFile() && QImageReader(entryInfoAbsPath).canRead())
                {
                    if (verbose)
                        out << "Found " << entryInfoAbsPath << endl;
                    fileNames.append(entryInfoAbsPath);
                }
            }
        }
    }

    // Extract single output filename, if supplied
    QString singleOutputFileName;
    if (!filePatterns.isEmpty())
    {
        if (filePatterns.last().endsWith(".pdf", Qt::CaseInsensitive))
        {
            singleOutputFileName = filePatterns.takeLast();
            outputFormat = QPrinter::PdfFormat;
        }
        else if (filePatterns.last().endsWith(".ps", Qt::CaseInsensitive))
        {
            singleOutputFileName = filePatterns.takeLast();
            outputFormat = QPrinter::PostScriptFormat;
        }
    }
    if (verbose && !singleOutputFileName.isEmpty())
        out << "Output file is " << singleOutputFileName << flush;

    // Check for at least one input file
    if (fileNames.isEmpty())
    {
        err << "No input files" << endl;
        return ParseFailure;
    }

    // Convert images to PDF
    foreach (QString inputFileName, fileNames)
    {
        if (verbose)
            out << "Processing " << inputFileName << flush;

        QImage image(inputFileName);
        if (image.isNull())
        {
            err << "Could not load image " << inputFileName;
            continue;
        }

        QPrinter printer(QPrinter::ScreenResolution);
        printer.setOutputFileName(singleOutputFileName);
        printer.setOutputFormat(outputFormat);
        printer.setPaperSize(paperSize);
        printer.setOrientation(pageOrientation);
        int r = printer.resolution();

        switch (image.format())
        {
        case QImage::Format_Mono:
        case QImage::Format_MonoLSB:
            printer.setColorMode(QPrinter::GrayScale);
            break;

        default:
            printer.setColorMode(QPrinter::Color);
            break;
        }

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
    }

    return Success;
}
