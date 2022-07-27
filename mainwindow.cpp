#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <Windows.h>
#include <WinUser.h>
#include <stdint.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QHBoxLayout>

typedef struct EnumHWndsArg
{
    std::vector<HWND> *vecHWnds;
    DWORD dwProcessId;
} EnumHWndsArg, *LPEnumHWndsArg;

HANDLE GetProcessHandleByID(int nID) //通过进程ID获取进程句柄
{
    return OpenProcess(PROCESS_ALL_ACCESS, FALSE, nID);
}

DWORD GetProcessIDByName(const wchar_t *pName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot)
    {
        return NULL;
    }
    PROCESSENTRY32 pe = {sizeof(pe)};
    for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe))
    {
        if (wcscmp(pe.szExeFile, pName) == 0)
        {

            CloseHandle(hSnapshot);
            return pe.th32ProcessID;
        }
        // printf("%-6d %s\n", pe.th32ProcessID, pe.szExeFile);
    }
    CloseHandle(hSnapshot);
    return 0;
}
BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam)
{
    EnumHWndsArg *pArg = (LPEnumHWndsArg)lParam;
    DWORD processId;

    GetWindowThreadProcessId(hwnd, &processId);
    char className[65536];
    RealGetWindowClassA(hwnd, className, 1000);
    QString cls(className);
    HWND parent = GetParent(hwnd);
    //    qDebug()<<"processId: "<<processId<<" >>>>>  processId by winId:"<<pArg->dwProcessId;;

    if (processId == pArg->dwProcessId && cls != "IME" && parent == NULL)
    {
        //        char title[65536];
        //        char className[65536];
        //        GetWindowTextA(hwnd, title, 1000);
        //        RealGetWindowClassA(hwnd, className, 1000);
        //        qDebug()<<"title: "<<title;
        //        qDebug()<<"className: "<<className;
        pArg->vecHWnds->push_back(hwnd);
        // printf("%p\n", hwnd);
    }
    return TRUE;
}

void GetHWndsByProcessID(DWORD processID, std::vector<HWND> &vecHWnds)
{
    EnumHWndsArg wi;
    wi.dwProcessId = processID;
    wi.vecHWnds = &vecHWnds;
    EnumWindows(lpEnumFunc, (LPARAM)&wi);
    qDebug() << vecHWnds;
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("abcd中文");
    this->setWindowIcon(QIcon(":/new/Cow.png"));

    initMenuBar();

    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
}

MainWindow::~MainWindow()
{
    delete ui;

    closeAllProcess();
}

void MainWindow::actionTriggered()
{
    QAction *act = (QAction *)sender();
    QString path = act->property("path").toString();
    QString name = act->property("name").toString();
    qint64 pid = startProcess(path);
    WId wid = getProcessWid(pid);
    addTabOfWindow(wid, name);
}

void MainWindow::initMenuBar()
{
    QFile file("./config.json");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray bytes = file.readAll();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError)
            return;
        QJsonArray arr = doc.array();

        for (int i = 0; i < arr.count(); i++)
        {
            QJsonObject obj = arr.at(i).toObject();
            QString name = obj.value("name").toString();

            if (obj.value("son").toArray().count() == 0)
            {
                QAction *act = new QAction(name);
                QString path = obj.value("appPath").toString();
                act->setProperty("path", path);
                act->setProperty("name", name);
                ui->menuBar->addAction(act);
                connect(act, SIGNAL(triggered(bool)), this, SLOT(actionTriggered()));
            }
            else
            {
                QMenu *menu = new QMenu(name);

                addSonsOfMenu(menu, obj.value("son").toArray());
                ui->menuBar->addMenu(menu);
            }
        }
        file.close();
    }
    else
    {
        qDebug() << file.errorString();
    }
}

void MainWindow::addSonsOfMenu(QMenu *menu, const QJsonArray &arr)
{
    for (int i = 0; i < arr.count(); i++)
    {
        QJsonObject obj = arr.at(i).toObject();
        QString name = obj.value("name").toString();

        if (obj.value("son").toArray().count() == 0)
        {
            QAction *act = new QAction(name);
            QString path = obj.value("appPath").toString();
            act->setProperty("path", path);
            act->setProperty("name", name);
            menu->addAction(act);
            connect(act, SIGNAL(triggered(bool)), this, SLOT(actionTriggered()));
        }
        else
        {
            QMenu *menu = new QMenu(name);

            addSonsOfMenu(menu, obj.value("son").toArray());
            menu->addMenu(menu);
        }
    }
}

qint64 MainWindow::startProcess(QString cmd)
{
    QProcess *process = new QProcess(this);
    process->setProgram(cmd);
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args)
                                               {
    args->startupInfo->wShowWindow = SW_HIDE;
    args->startupInfo->dwFlags = /*STARTF_USESHOWWINDOW |*/ STARTF_USESTDHANDLES; });
    process->start();
    process->waitForStarted(5000);
    Sleep(3500);
    this->pids.append(process);
    return process->processId();
}

WId MainWindow::getProcessWid(qint64 pid)
{
    if (pid != 0)
    {
        std::vector<HWND> vecHWnds;
        GetHWndsByProcessID((DWORD)pid, vecHWnds);

        if (vecHWnds.size() > 0)
        {
            HWND &w = vecHWnds[0];
            SetWindowLongA(w, -16, 0x10000000);
            return (WId)w;
        }
    }
    return 0;
}

void MainWindow::addTabOfWindow(WId wid, QString tabName)
{
    if (wid == 0)
        return;
    QWindow *m_window = QWindow::fromWinId(wid);
    QWidget *m_widget = QWidget::createWindowContainer(m_window, this, Qt::Window);
    ui->tabWidget->addTab(m_widget, tabName);
    ui->tabWidget->setCurrentWidget(m_widget);
}

void MainWindow::closeTab(int index)
{
    if (index == -1)
        return;

    QWidget *tabItem = ui->tabWidget->widget(index);
    // Removes the tab at position index from this stack of widgets.
    // The page widget itself is not deleted.
    ui->tabWidget->removeTab(index);

    this->pids[index]->kill();
    this->pids.takeAt(index);
    delete (tabItem);
    tabItem = nullptr;
}

void MainWindow::closeAllProcess()
{
    QList<QProcess *> processList = this->findChildren<QProcess *>();
    for (int i = 0; i < processList.count(); i++)
    {
        if (processList[i]->state() == QProcess::Running)
        {
            processList[i]->close();
        }
    }
}
