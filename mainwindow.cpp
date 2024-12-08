#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <sstream>
#include <QtConcurrent>
#include <QMessageBox>
#if _WIN32
#include "inject.h"
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &MainWindow::timeOut);
    timer_->start();
}

void MainWindow::timeOut()
{
#ifdef _WIN32
    if (InjectMediaSdk())
        ui->label_status->setText("installed");
    else
        ui->label_status->setText("waiting for install");
#endif
}

MainWindow::~MainWindow()
{
    timer_->stop();
    delete timer_;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{

}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType == "windows_generic_MSG")
    {
#if _WIN32
        MSG *msg = reinterpret_cast<MSG *>(message);
        if (msg->message == WM_COPYDATA)
        {
            *result = 0;
            COPYDATASTRUCT *data = reinterpret_cast<COPYDATASTRUCT *>(msg->lParam);
            QString recevice((const char *)data->lpData);
            if (data->dwData == 0)
                ui->lineEdit_URL->setText(recevice);
            else if (data->dwData == 1)
                ui->lineEdit_key->setText(recevice);
            return true;
        }
#endif
    }

    return QWidget::nativeEvent(eventType, message, result);
}
