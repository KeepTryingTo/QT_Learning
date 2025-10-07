#include "flightshooting.h"
#include "startmenu.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 创建主窗口但不立即显示
    FlightShooting w;

    // 创建开始菜单
    StartMenu menu;
    menu.setWindowTitle("小蜜蜂游戏");
    menu.setModal(true);

    // 连接菜单信号
    QObject::connect(&menu, &StartMenu::startGame, &w, &FlightShooting::show);
    QObject::connect(&menu, &StartMenu::exitGame, &a, &QApplication::quit);
    // QObject::connect(&w, &FlightShooting::gameOver, &menu, &StartMenu::show);

    // 显示菜单，根据返回值决定是否继续执行
    if (menu.exec() == QDialog::Accepted) {
        // 用户点击了开始游戏，继续运行应用程序
        return a.exec();
    } else {
        // 用户点击了退出或关闭窗口，直接退出程序
        return 0;
    }
}
