/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGraphicsView *graphicsView;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QPushButton *btnOpen;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnStart;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *btnStop;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *btnEnd;
    QWidget *layoutWidget1;
    QHBoxLayout *horizontalLayout_3;
    QLabel *labState;
    QLineEdit *stateEdit;
    QWidget *layoutWidget2;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QSlider *progressBar;
    QLabel *labProgress;
    QPushButton *labStart;
    QSlider *videoVolumn;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(539, 446);
        QFont font;
        font.setFamilies({QString::fromUtf8("Times New Roman")});
        MainWindow->setFont(font);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/new/prefix1/resources/images/sound.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        graphicsView = new QGraphicsView(centralwidget);
        graphicsView->setObjectName("graphicsView");
        graphicsView->setGeometry(QRect(20, 10, 501, 291));
        layoutWidget = new QWidget(centralwidget);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(30, 320, 491, 41));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        btnOpen = new QPushButton(layoutWidget);
        btnOpen->setObjectName("btnOpen");
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Times New Roman")});
        font1.setPointSize(12);
        btnOpen->setFont(font1);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/new/prefix1/resources/images/Image2.jpg"), QSize(), QIcon::Normal, QIcon::Off);
        btnOpen->setIcon(icon1);

        horizontalLayout->addWidget(btnOpen);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnStart = new QPushButton(layoutWidget);
        btnStart->setObjectName("btnStart");
        btnStart->setFont(font1);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/new/prefix1/resources/images/Image3.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnStart->setIcon(icon2);

        horizontalLayout->addWidget(btnStart);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        btnStop = new QPushButton(layoutWidget);
        btnStop->setObjectName("btnStop");
        btnStop->setFont(font1);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/new/prefix1/resources/images/Image6.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnStop->setIcon(icon3);

        horizontalLayout->addWidget(btnStop);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        btnEnd = new QPushButton(layoutWidget);
        btnEnd->setObjectName("btnEnd");
        btnEnd->setFont(font1);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/new/prefix1/resources/images/Image5.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnEnd->setIcon(icon4);

        horizontalLayout->addWidget(btnEnd);

        btnStart->raise();
        btnStop->raise();
        btnEnd->raise();
        btnOpen->raise();
        layoutWidget1 = new QWidget(centralwidget);
        layoutWidget1->setObjectName("layoutWidget1");
        layoutWidget1->setGeometry(QRect(30, 400, 491, 41));
        horizontalLayout_3 = new QHBoxLayout(layoutWidget1);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        labState = new QLabel(layoutWidget1);
        labState->setObjectName("labState");
        labState->setFont(font1);

        horizontalLayout_3->addWidget(labState);

        stateEdit = new QLineEdit(layoutWidget1);
        stateEdit->setObjectName("stateEdit");
        stateEdit->setFont(font1);

        horizontalLayout_3->addWidget(stateEdit);

        layoutWidget2 = new QWidget(centralwidget);
        layoutWidget2->setObjectName("layoutWidget2");
        layoutWidget2->setGeometry(QRect(30, 365, 491, 34));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget2);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(layoutWidget2);
        label->setObjectName("label");
        label->setFont(font1);

        horizontalLayout_2->addWidget(label);

        progressBar = new QSlider(layoutWidget2);
        progressBar->setObjectName("progressBar");
        progressBar->setFont(font1);
        progressBar->setMaximum(10);
        progressBar->setValue(0);
        progressBar->setOrientation(Qt::Horizontal);

        horizontalLayout_2->addWidget(progressBar);

        labProgress = new QLabel(layoutWidget2);
        labProgress->setObjectName("labProgress");
        QFont font2;
        font2.setFamilies({QString::fromUtf8("Times New Roman")});
        font2.setPointSize(13);
        labProgress->setFont(font2);
        labProgress->setTextFormat(Qt::MarkdownText);
        labProgress->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(labProgress);

        labStart = new QPushButton(layoutWidget2);
        labStart->setObjectName("labStart");
        labStart->setFont(font1);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/new/prefix1/resources/images/sound.webp"), QSize(), QIcon::Normal, QIcon::Off);
        labStart->setIcon(icon5);
        labStart->setIconSize(QSize(28, 28));

        horizontalLayout_2->addWidget(labStart);

        videoVolumn = new QSlider(layoutWidget2);
        videoVolumn->setObjectName("videoVolumn");
        videoVolumn->setFont(font1);
        videoVolumn->setMaximum(100);
        videoVolumn->setValue(20);
        videoVolumn->setOrientation(Qt::Horizontal);

        horizontalLayout_2->addWidget(videoVolumn);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\350\247\206\351\242\221\346\222\255\346\224\276\345\231\250", nullptr));
        btnOpen->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\346\226\207\344\273\266", nullptr));
        btnStart->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276\350\247\206\351\242\221", nullptr));
        btnStop->setText(QCoreApplication::translate("MainWindow", "\346\232\202\345\201\234\350\247\206\351\242\221", nullptr));
        btnEnd->setText(QCoreApplication::translate("MainWindow", "\345\201\234\346\255\242\346\222\255\346\224\276", nullptr));
        labState->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276\347\212\266\346\200\201", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276\350\277\233\345\272\246", nullptr));
        labProgress->setText(QCoreApplication::translate("MainWindow", "0/0", nullptr));
        labStart->setText(QCoreApplication::translate("MainWindow", "\351\237\263\351\207\217", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
