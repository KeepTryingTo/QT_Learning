import QtQuick
import QtQuick.Controls
// import QtQuick.Controls.Basic


//document download: https://wenku.baidu.com/view/d744d682680203d8ce2f24ab.html?_wkts_=1728804889856&bdQuery=qt+quick%E6%96%87%E6%A1%A3

Window {
    id:root;
    width: 320;
    height: 480;
    visible: true;
    title: qsTr("QQ");
    color: "#E0FFFF";

    //log标识
    Rectangle {
        width: 70;
        height: 70;
        id: logPosition;
        color: "white";
        radius: 35;
        x : parent.width / 2 - this.width / 2;
        y : 50;
        Image {
            id: log;
            source: "images/log.jpg";
            anchors.centerIn: parent;
            width: 50;
            height: 50;
        }
    }
    //账号输入框
    Rectangle {
        id: qqid;
        width:parent.width * 0.8;
        height: 45;
        color: "white";
        radius: 10;
        x: parent.width / 2 - this.width / 2;
        y: 150;
        TextInput {
            id:inputqqid;
            text: "输入QQ账号";
            color: "lightgray";
            font.pointSize: 15;
            anchors.centerIn: parent;

            //处理失去焦点事件
            onTextChanged: {
               if(inputqqid.focus === false){
                   inputqqid.focus = true;
               }
            }
        }
        //鼠标点击编辑框事件
        MouseArea {
            anchors.fill: parent;
            onClicked: {
                if(inputqqid.text === "输入QQ账号"){
                    inputqqid.text = "";
                    inputqqid.color = "black";
                    inputqqid.focus = true;
                }else {
                    if(inputqqid.text.trim() === ""){
                        inputqqid.color = "gray";
                    }else{
                        inputqqid.color = "black";
                    }
                }
                if(inputqqid.focus === false){
                    inputqqid.color = "black";
                    inputqqid.focus = true;
                }
            }
        }

        //如果文本框失去焦点事件就还原回原来的文本框样子
        onVisibleChanged: {
            if(!inputqqid.visible && inputqqid.text.trim() === ""){
                inputqqid.text = "输入QQ账号";
                inputqqid.color = "gray";
            }
        }
    }

    Rectangle {
        id: passwordid;
        width:parent.width * 0.8;
        height: 45;
        color: "white";
        radius: 10;
        x: parent.width / 2 - this.width / 2;
        y: 215;
        TextInput {
            id:inputPasswordid;
            text: "输入QQ密码";
            color: "lightgray";
            font.pointSize: 15;
            anchors.centerIn: parent;
        }
        //鼠标点击编辑框事件
        MouseArea {
            anchors.fill: parent;
            onClicked: {
                if(inputPasswordid.text === "输入QQ密码"){
                    inputPasswordid.text = "";
                    inputPasswordid.color = "black";
                    inputPasswordid.focus = true;
                }else {
                    if(inputPasswordid.text.trim() === ""){
                        inputPasswordid.color = "gray";
                    }else{
                        inputPasswordid.color = "black";
                    }
                    inputPasswordid.color = "black";
                    inputPasswordid.focus = true;
                }
            }
        }

        //如果文本框失去焦点事件就还原回原来的文本框样子
        onVisibleChanged: {
            if(!inputPasswordid.visible && inputPasswordid.text.trim() === ""){
                inputPasswordid.text = "输入QQ密码";
                inputPasswordid.color = "gray";
            }
        }
    }

    Rectangle {
        id: yes;
        width: parent.width * 0.85;
        height: 40;
        color: "#E0FFFF";
        // color: "white";
        x: parent.width / 2 - this.width / 2;
        y: 279;
        CheckBox {
            id: checkid;
            width: 10;
            height: 10;
            x: 10;
            y: yes.height / 2 - this.height / 2;
            checked: false;
            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    checkid.checked = !checkid.checked;
                }
            }
        }
        TextEdit {
            x:30;
            font.pointSize: 10;
            color: "gray";
            readOnly: true;
            y: parent.height / 2 - this.height / 2;
            text:"已阅读并同意";
        }

        TextEdit {
            x: 108;
            color: "#00F5FF";
            readOnly: true;
            font.pointSize: 10;
            y: parent.height / 2 - this.height / 2;
            text:"服务协议";

            MouseArea {
                anchors.fill: parent;
                cursorShape: Qt.PointingHandCursor;
                onClicked: {
                    // 按下时打开超链接
                    Qt.openUrlExternally("https:/www.baidu.com");
                }
            }
        }

        TextEdit {
            x:160;
            color: "gray";
            font.pointSize: 10;
            readOnly: true;
            y: parent.height / 2 - this.height / 2;
            text:"和";
        }

        TextEdit {
            x : 173;
            color: "#00F5FF";
            font.pointSize: 10;
            readOnly: true;
            y: parent.height / 2 - this.height / 2;
            text:"QQ隐私保护指引";

            MouseArea {
                anchors.fill: parent;
                cursorShape: Qt.PointingHandCursor;
                onClicked: {
                    // 按下时打开超链接
                    Qt.openUrlExternally("https:/www.baidu.com");
                }
            }
        }
    }

    Rectangle {
        id: login;
        width: parent.width * 0.8;
        height: 45;
        x: parent.width / 2 - this.width / 2;
        y: 330;
        color: "#00F5FF";
        radius: 10;


        TextEdit {
            id: loginC;
            text: "登录";
            width:30;
            height: 20;
            color: "white";
            font.pointSize: 16;
            x: parent.width / 2 - this.width / 2;
            y: parent.height / 2 - this.height / 2 - 5;
        }

        // 鼠标悬停时颜色变化
        MouseArea {
            id: mouseAreaBtn;
            anchors.fill: parent;
            cursorShape: Qt.PointingHandCursor;
            onClicked: {
                login.radius = 10;
                console.log("Button clicked");
            }
            //鼠标悬停时的颜色
            onEntered: {
                login.color = "#98F5FF";
                login.radius = 10;
            }
            //鼠标离开时的颜色
            onExited: {
                login.color = "#00F5FF";
                login.radius = 10;
            }

            onPressed: {
                login.radius = 10;
                login.color = "#00F5FF";  // 按下时颜色变化
            }
            onReleased: {
                login.radius = 10;
                login.color = "#00F5FF";  // 释放时恢复颜色
            }
        }
    }


    Rectangle {
        id:bottomR;
        color: "#E0FFFF";
        width: parent.width * 0.5;
        height: 30;
        x: parent.width / 2- this.width / 2;
        y: login.y + 100;
        Text {
            id: scanCode;
            height: 20;
            x: 10;
            font.pointSize: 12;
            y: parent.height / 2 - this.height / 2 - 3;
            text: qsTr("扫码登录");
            color: "#00F5FF";

            MouseArea {
                anchors.fill: parent;
                cursorShape: Qt.PointingHandCursor;
                onClicked: {
                    // 按下时打开超链接
                    // Qt.openUrlExternally("https:/www.baidu.com");
                    root.visible = false;
                    scanCodePageLoader.source = "ScanCodePage.qml";
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
            text: qsTr("更多选项");
            color: "#00F5FF";

            MouseArea {
                anchors.fill: parent;
                cursorShape: Qt.PointingHandCursor;
                onClicked: {
                    optionsPopup.open();
                }
            }
        }
    }

    Loader {
        id:scanCodePageLoader;
        width: 320;
        height: 480;
        anchors.fill: parent;
    }

    Popup {
        id: optionsPopup;
        width: 80;
        height: 80;
        modal: true;  // 设置为模态窗口
        background: Rectangle {
            color: "white";
            radius: 10;
        }
        x: parent.width / 2 - this.width / 2 + moreSelect.width;
        y: bottomR.y - 85;

        Rectangle {
            color: "white";
            border.color: "gray";
            border.width: 1;

            Column {
                spacing: 5;
                padding: 5;

                // 选项列表
                Button {
                    text: "注册账号";
                    background: Rectangle {
                        color: "white";
                    }
                    onClicked: {
                        console.log("注册账号");
                        scanCodePageLoader.source = "registerPage.qml";
                        optionsPopup.close();  // 选择后关闭弹出窗口
                    }
                }

                Button {
                    text: "忘记密码";
                    background: Rectangle {
                        color: "white";
                    }
                    onClicked: {
                        console.log("忘记密码");
                        Qt.openUrlExternally("https:/www.baidu.com");
                        optionsPopup.close();  // 选择后关闭弹出窗口
                    }
                }
            }
        }
    }
}
