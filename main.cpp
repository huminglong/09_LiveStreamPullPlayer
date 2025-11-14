/**
 * @file main.cpp
 * @brief 应用程序入口，初始化 Qt 并显示主窗口。
 * @mainfunctions
 *   - main
 * @mainclasses
 *   - MainWindow
 */

#include "mainwindow.h"

#include <QApplication>

 /**
  * @brief Qt 应用程序入口，负责创建 QApplication 和 MainWindow。
  * @param argc 命令行参数数量。
  * @param argv 命令行参数数组。
  * @return Qt 事件循环退出码。
  */
int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
