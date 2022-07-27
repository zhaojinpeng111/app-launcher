#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QProcess>
#include<QWindow>
#include<QMenu>
#include<QJsonArray>
#include<QList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void actionTriggered();
    void closeTab(int index);

private:

    void initMenuBar();
    void addSonsOfMenu(QMenu* menu,const QJsonArray& arr);


    qint64 startProcess(QString cmd);
    WId getProcessWid(qint64  pid);
    void addTabOfWindow(WId wid,QString tabName);


    void closeAllProcess();


private:
    Ui::MainWindow *ui;
    QProcess* m_pProcess;
    QWindow*  m_window;
    QList<QProcess*> pids;
};

#endif // MAINWINDOW_H
