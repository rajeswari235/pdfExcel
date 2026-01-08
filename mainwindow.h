#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "xlsxdocument.h"
#include <QStandardPaths>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_pushButton_upload_clicked();
    QString classifyComponent(const QString &desc);

private:
    Ui::MainWindow *ui;

    int totalQuantity=0;
    double totalAmount=0.0;


};

#endif // MAINWINDOW_H
