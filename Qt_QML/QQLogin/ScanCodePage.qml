import QtQuick
import QtQuick.Controls


Window {
    id: root;
    width: 320;
    height: 480;
    visible: true;
    color: "#E0FFFF";
    title: "扫码登录";
    Rectangle {
        id:textRid;
        width: 90;
        height: 30;
        color: "#E0FFFF";
        x: parent.width / 2 - this.width / 2;
        y: 30;
        Text {
            id: qq9id;
            width: 60;
            height: 20;
            x: parent.width / 2 - this.width / 2;
            // y: parent.height / 2 - this.height / 2;
            text: qsTr("QQ9");
            font.pointSize: 22;
            // font.family: "Arial";
            font.bold: true;
            color: "#EE82EE";
        }
    }
    Rectangle {
        id:imgRid;
        width: 200;
        height: 200;
        color: "#E0FFFF";
        x: parent.width / 2 - this.width / 2;
        y: 80;
        radius: 20;
        clip: true;
        Image {
            id: imgid;
            width: parent.width;
            height: parent.height;
            anchors.centerIn: parent.Center;
            source: "images/qq.png";
        }
    }

    Rectangle {
        id: proid;
        width: 200;
        height: 30;
        x: parent.width / 2 - this.width / 2;
        y: 300;
        color: "#E0FFFF";
        TextEdit {
            color: "#8B8682";
            readOnly: true;
            width: parent.width;
            height: parent.height;
            font.pointSize: 13;
            x: 10;
            y: parent.height / 2 - this.height / 2;
            text:"请使用手机QQ扫码登录";
        }
    }

    Rectangle {
        id:bottomR;
        color: "#E0FFFF";
        width: parent.width * 0.5;
        height: 30;
        x: parent.width / 2- this.width / 2;
        y: proid.y + 100;
        Text {
            id: scanCode;
            height: 20;
            x: 10;
            font.pointSize: 12;
            y: parent.height / 2 - this.height / 2 - 3;
            text: qsTr("账密登录");
            color: "#00F5FF";

            MouseArea {
                anchors.fill: parent;
                cursorShape: Qt.PointingHandCursor;
                onClicked: {
                    // 按下时打开超链接
                    // Qt.openUrlExternally("https:/www.baidu.com");
                    root.visible = false;
                    loginPage.source = "QtQuick_QQLogin.qml";
                }
            }
        }
        Text {
            id: sp;
            font.pointSize: 12;
            x:parent.width / 2;
            text: qsTr("|");
            color: "#DCDCDC";
        }
        Text {
            id: moreSelect;
            height: 20;
            font.pointSize: 12;
            x: parent.width / 2 + 10;
            y: parent.height / 2 - this.height / 2 - 3;
            text: qsTr("注册登录");
            color: "#00F5FF";

            MouseArea {
                anchors.fill: parent;
                cursorShape: Qt.PointingHandCursor;
                onClicked: {
                    // 按下时打开超链接
                    // Qt.openUrlExternally("https:/www.baidu.com");
                    root.visible = false;
                    loginPage.source = "registerPage.qml";
                }
            }
        }
    }

    Loader {
        id: loginPage;
        anchors.fill: parent;
    }
}
