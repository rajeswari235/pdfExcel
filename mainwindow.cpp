#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(350,200);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_upload_clicked()
{
    totalQuantity=0;
    totalAmount=0.0;
    struct SummaryData
    {
        int totalQty = 0;
        double totalAmount = 0.0;
    };
    QMap<QString, SummaryData> summaryMap;

    QString pdfPath = QFileDialog::getOpenFileName(
        this, "Select PDF File", "", "PDF Files (*.pdf)");

    if (pdfPath.isEmpty())
        return;

    QString defaultName = QString("ExcelData_%1.xlsx")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QString fullPath = QFileDialog::getSaveFileName(
                this,
                "Save Sensor Data",
                desktopPath + "/" + defaultName,
                "Excel Files (*.xlsx)"
    );
    if (fullPath.isEmpty())
        return;

    // Qt EXE directory
    QString pythonExe = "python";
      QString scriptPath =
          QCoreApplication::applicationDirPath() + "/dist/extract_pdf_table.exe";
      qDebug()<<QCoreApplication::applicationDirPath()<<"******************";

    QProcess process;

    // running script
   // process.start(pythonExe, QStringList() << scriptPath << pdfPath);
    process.start(scriptPath,QStringList()<<pdfPath);

    process.waitForFinished(-1);

    QString output = process.readAllStandardOutput();
    QString error  = process.readAllStandardError();


    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Python Error", error);
        return;
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        QMessageBox::warning(this, "No Data",
                             "No table data found in PDF.");
        return;
    }

    //Header Format
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format headerFormat1;
    headerFormat1.setFontBold(true);
    headerFormat1.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    headerFormat1.setVerticalAlignment(QXlsx::Format::AlignTop);
   // headerFormat1.setFontSize(12);

    QXlsx::Format wrapFormat;
    wrapFormat.setTextWrap(true);
    wrapFormat.setVerticalAlignment(QXlsx::Format::AlignTop);
    wrapFormat.setBorderStyle(QXlsx::Format::BorderThin);


    QXlsx::Format mergeFormat;
    mergeFormat.setFontBold(true);
    mergeFormat.setPatternBackgroundColor("yellow");

    QXlsx::Format amountFormat;
    amountFormat.setFontBold(true);
    amountFormat.setNumberFormat("#,##0.00");
    amountFormat.setBorderStyle(QXlsx::Format::BorderThin);


    // Create Excel (QXlsx)

    QXlsx::Document xlsx;
    xlsx.mergeCells("A1:B1");
    xlsx.mergeCells("A2:B2");
    xlsx.mergeCells("A3:B3");

    int excelRow = 1;

    // 🔹 Handle header info
    if (lines.first().startsWith("HEADER|||")) {
        QStringList header = lines.takeFirst().split("|||");

        //xlsx.write(excelRow, 1, header.value(1),headerFormat1); excelRow++;
        xlsx.write("A1", header.value(1),headerFormat1); excelRow++;
        xlsx.write("A2",header.value(2),headerFormat1); excelRow++;
        xlsx.write("A3", header.value(3),headerFormat1); excelRow++;

        excelRow += 2;  // empty row
    }

    xlsx.write(excelRow, 1, "SINo",headerFormat);
    xlsx.write(excelRow, 2, "Part Number",headerFormat);
    xlsx.write(excelRow, 3, "Description",headerFormat);
    xlsx.write(excelRow, 4, "Customer Part No",headerFormat);
    xlsx.write(excelRow, 5, "Quantity",headerFormat);
    xlsx.write(excelRow, 6, "Unit Price(INR)",headerFormat);
    xlsx.write(excelRow, 7, "Amount in INR",headerFormat);
    excelRow++;


    xlsx.setColumnWidth(1, 1, 8);   // Index
    xlsx.setColumnWidth(2, 4, 25);
    xlsx.setColumnWidth(5, 7, 15);




    //int row = 2;
    for (const QString &line : lines) {
        QStringList parts = line.split("|||");
        qDebug()<<"total parts"<<parts.size();
        if (parts.size() < 1)
            continue;

        for (int col = 0; col < parts.size()-1; ++col) {
            bool ok;
            QString value = parts[col].trimmed();
              // Remove currency symbols & separators
              value.remove("₹");
              value.remove("INR");
              value.remove(",");
              double number = value.toDouble(&ok);
              if (ok) {
                  xlsx.write(excelRow, col+1, number,wrapFormat);   // ✅ numeric
              } else {
                  xlsx.write(excelRow, col+1, value,wrapFormat);    // fallback text
              }
        }
        excelRow++;
        QString customerPartNo=parts[3].trimmed();
        bool qtyOk, amtOk;


        int quantity = parts[4].trimmed().toInt(&qtyOk);

        QString amountStr=parts[6].trimmed();
        amountStr.remove("₹");
        amountStr.remove("INR");
        amountStr.remove(",");
        double amount=amountStr.toDouble(&amtOk);
        if (!qtyOk || !amtOk)
            continue;
        totalQuantity+=quantity;
        totalAmount+=amount;
        //Aggregate

        summaryMap[customerPartNo].totalQty+=quantity;
        summaryMap[customerPartNo].totalAmount+=amount;

    }
    xlsx.write(excelRow-1,4,"Total",amountFormat);
    xlsx.write(excelRow-1, 5,totalQuantity,amountFormat);
    xlsx.write(excelRow-1,7,totalAmount,amountFormat);

    int summaryStartCol=9;
    int summaryRow=12;
    xlsx.write(summaryRow,summaryStartCol,"SI No",headerFormat);
    xlsx.write(summaryRow, summaryStartCol+1,     "Customer Part No", headerFormat);
    xlsx.write(summaryRow, summaryStartCol + 2, "Total Quantity",   headerFormat);
    xlsx.write(summaryRow, summaryStartCol + 3, "Total Amount (INR)", headerFormat);

    summaryRow++;
    xlsx.setColumnWidth(summaryStartCol, summaryStartCol,15);
    xlsx.setColumnWidth(summaryStartCol + 1, summaryStartCol + 3, 25);

    xlsx.write(11,9,"MERGED Excel",mergeFormat);
    int types=1;
    double totalINR=0.0;
    for (auto it = summaryMap.begin(); it != summaryMap.end(); ++it) {
        xlsx.write(summaryRow,summaryStartCol,types,wrapFormat);
        xlsx.write(summaryRow, summaryStartCol+1,     it.key(), wrapFormat);
        xlsx.write(summaryRow, summaryStartCol + 2, it.value().totalQty,wrapFormat);
        xlsx.write(summaryRow, summaryStartCol + 3, it.value().totalAmount,wrapFormat);
        totalINR+=it.value().totalAmount;
        types++;
        summaryRow++;
    }
    xlsx.write(summaryRow,summaryStartCol,"",amountFormat);
    xlsx.write(summaryRow,summaryStartCol+2,"",amountFormat);
    xlsx.write(summaryRow,summaryStartCol+1,"Total INR",amountFormat);
    xlsx.write(summaryRow,summaryStartCol+3,totalINR,amountFormat);




    if (xlsx.saveAs(fullPath)) {
        QMessageBox::information(this, "Success",
                                 "Excel file generated successfully!");
    } else {
        QMessageBox::critical(this, "Error",
                              "Failed to save Excel file.");
    }
}
