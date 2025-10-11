#include "LoginDialog.h"
#include "GameSelectionDialog.h"
#include "Client.h"
#include "chessmainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 主循环：登录 -> 游戏选择 -> 游戏 -> 游戏选择 -> 登录
    while (true) {
        // 1. 显示登录对话框
        LoginDialog login;
        if (login.exec() != QDialog::Accepted) {
            // 用户点击退出
            return 0;
        }

        QString username = login.getCurrentUsername();

        // 2. 游戏选择循环
        bool returnToLogin = false;
        while (!returnToLogin) {
            GameSelectionDialog selection(username);
            int selectionResult = selection.exec();

            std::cout<<selection.getMode().toStdString()<<std::endl;

            // 退出程序
            if (selectionResult == 2) {
                return 0;
            }
            else if (selectionResult == QDialog::Rejected) { // 返回登录
                returnToLogin = true;
            }else if (selectionResult == QDialog::Accepted) {
                QString gameName = selection.getGameName();  // 这里返回的是 "gomoku" 或 "chess"
                QString mode = selection.getMode();          // 这里返回的是 "pvp" 或 "pve"

                if (gameName == "chess") {  // 修改判断条件
                    if (mode == "pvp") {
                        QMessageBox::information(nullptr, "提示", "中国象棋人人对战暂未实现");
                        continue;
                    } else if (mode == "pve") {
                        // 中国象棋人机对战
                        ChessMainWindow chessWindow("中国象棋");  // 可以传递显示名称
                        chessWindow.setWindowTitle("中国象棋 - 人机对战");
                        chessWindow.show();

                        // 创建事件循环等待窗口关闭
                        QEventLoop loop;
                        QObject::connect(&chessWindow, &ChessMainWindow::closed, &loop, &QEventLoop::quit);
                        QObject::connect(&chessWindow, &ChessMainWindow::returnToSelection, &loop, &QEventLoop::quit);
                        loop.exec();
                    }
                }
                else if (gameName == "gomoku") {  // 明确处理五子棋情况
                    // 五子棋
                    Client client(selection.getUserName(), "五子棋", mode);  // 传递显示名称
                    client.show();

                    QEventLoop loop;
                    QObject::connect(&client, &Client::returnToSelection, &loop, &QEventLoop::quit);
                    QObject::connect(&client, &Client::destroyed, &loop, &QEventLoop::quit);
                    loop.exec();
                }
            }

        }
    }

    // QString username = "ktg";
    // QString gameName = "五子棋";
    // QString gameMode = "pvp";
    // Client *client = new Client(username,
    //                             gameName,
    //                             gameMode);

    // // 连接退出信号
    // QObject::connect(client, &Client::returnToSelection, [&]() {
    //     client->deleteLater(); // 安全删除客户端
    //     // 这里会跳出内层循环，重新进入游戏选择
    // });

    // client->show();

    return a.exec();
}
