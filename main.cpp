#include <QCoreApplication>
#include <QTextStream>
#include <QThread>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QPrinter>
#include <QPainter>
#include <QHash>


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // As in http://doc.qt.io/qt-4.8/qt-xml-xmlstreamlint-main-cpp.html
    enum ExitCode
    {
        Success = 0,
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

    // Create mapping between page sizes and their string representations
    QHash<QPrinter::PaperSize, QString> paperSizeStrings;
    paperSizeStrings.insert(QPrinter::A0, "A0");
    paperSizeStrings.insert(QPrinter::A1, "A1");
    paperSizeStrings.insert(QPrinter::A2, "A2");
    paperSizeStrings.insert(QPrinter::A3, "A3");
    paperSizeStrings.insert(QPrinter::A4, "A4");
    paperSizeStrings.insert(QPrinter::A5, "A5");
    paperSizeStrings.insert(QPrinter::A6, "A6");
    paperSizeStrings.insert(QPrinter::A7, "A7");
    paperSizeStrings.insert(QPrinter::A8, "A8");
    paperSizeStrings.insert(QPrinter::A9, "A9");
    paperSizeStrings.insert(QPrinter::B0, "B0");
    paperSizeStrings.insert(QPrinter::B1, "B1");
    paperSizeStrings.insert(QPrinter::B2, "B2");
    paperSizeStrings.insert(QPrinter::B3, "B3");
    paperSizeStrings.insert(QPrinter::B4, "B4");
    paperSizeStrings.insert(QPrinter::B5, "B5");
    paperSizeStrings.insert(QPrinter::B6, "B6");
    paperSizeStrings.insert(QPrinter::B7, "B7");
    paperSizeStrings.insert(QPrinter::B8, "B8");
    paperSizeStrings.insert(QPrinter::B9, "B9");
    paperSizeStrings.insert(QPrinter::B10, "B10");
    paperSizeStrings.insert(QPrinter::C5E, "C5E");
    paperSizeStrings.insert(QPrinter::Comm10E, "Comm10E");
    paperSizeStrings.insert(QPrinter::Custom, "Custom");
    paperSizeStrings.insert(QPrinter::DLE, "DLE");
    paperSizeStrings.insert(QPrinter::Executive, "Executive");
    paperSizeStrings.insert(QPrinter::Folio, "Folio");
    paperSizeStrings.insert(QPrinter::Ledger, "Ledger");
    paperSizeStrings.insert(QPrinter::Legal, "Legal");
    paperSizeStrings.insert(QPrinter::Letter, "Letter");
    paperSizeStrings.insert(QPrinter::Tabloid, "Tabloid");

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
        QPrinter printer;
        QList<QString> paperSizes = paperSizeStrings.values();
        qSort(paperSizes);
        foreach (QString size, paperSizes)
        {
            if (size == "Custom")
            {
                out << size << " (set dimensions by yourself)" << endl;
            }
            else
            {
                printer.setPaperSize(paperSizeStrings.key(size));
                QRectF paperRect = printer.paperRect(QPrinter::Millimeter);
                out << size << QString(" (%1 mm x %2 mm)")
                                   .arg(qRound(paperRect.width()))
                                   .arg(qRound(paperRect.height())) << endl;
            }
        }
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
