#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void timeOut();
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result);
private slots:
    void on_pushButton_clicked();
private:
    Ui::MainWindow *ui;
    QTimer *timer_;
};
#endif // MAINWINDOW_H
