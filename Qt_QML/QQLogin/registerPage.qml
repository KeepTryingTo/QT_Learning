import QtQuick
import QtQuick.Controls
// import QtQuick.Controls.Basic


Window {
    width: 500;
    height: 500;
    visible: true;
    color: "#E0FFFF";
    title: "注册账号";
    Rectangle {
        id:textRid;
        width: 90;
        height: 30;
        color: "#E0FFFF";
        y: 30;
        Text {
            id: qq9id;
            width: 60;
            height: 20;
            x: parent.width / 2 - this.width / 2;
            // y: parent.height / 2 - this.height / 2;
            text: qsTr("QQ9");
            font.pointSize: 15;
            // font.family: "Arial";
            font.bold: true;
            color: "#EE82EE";
        }
    }

    Rectangle {
        id:titleRid;
        width: 150;
        height: 30;
        color: "#E0FFFF";
        // color: "white";
        x: parent.width / 2 - this.width / 2;
        y: 100;
        Text {
            id: titleTid;
            width: 60;
            height: 20;
            text: qsTr("欢迎注册QQ");
            font.pointSize: 18;
            color: Qt.black;
        }
    }

    //账号输入框
    Rectangle {
        id: nameid;
        width:parent.width * 0.5;
        height: 36;
        color: "white";
        radius: 7;
        x: parent.width / 2 - this.width / 2;
        y: 150;
        TextInput {
            id:inputqqid;
            text: "输入昵称";
            color: "lightgray";
            font.pointSize: 10;
            x: 10;
            y: 8;
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
                if(inputqqid.text === "输入昵称"){
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
                inputqqid.text = "输入昵称";
                inputqqid.color = "gray";
            }
        }
    }

    Rectangle {
        id: passwordid;
        width:parent.width * 0.5;
        height: 36;
        color: "white";
        radius: 7;
        x: parent.width / 2 - this.width / 2;
        y: 200;
        TextInput {
            id:inputPasswordid;
            text: "输入QQ密码";
            color: "lightgray";
            font.pointSize: 10;
            x: 10;
            y: 8;
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
        id: columnid;
        width: parent.width * 0.5;
        height: 36;
        x: parent.width / 2 - this.width / 2;
        y: 250;
        color: "white";
        radius: 7;
        Row {
            id: rowId;
            anchors.fill: parent;
            height: columnid.height;
            // 国家代码选择框
            ComboBox {
                id: countryCodeComboBox;
                width: columnid.width * 0.3;
                height: columnid.height;
                model: ["+86", "+1", "+65","+7","+46"]; // 示例国际区号
                currentIndex: 0;
                background: Rectangle{
                    color:"transparent";
                }
            }
            // 手机号码输入框
            TextInput {
                id: phoneNumberField;
                width: columnid.width * 0.7;
                height: columnid.height;
                padding: 10;
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{10,15}$/  // 10-15 位的数字
                }
                Text {
                    text: "输入手机号码";
                    color: "lightgray";  // 占位符颜色
                    anchors.verticalCenter: parent.verticalCenter;  // 垂直居中
                    x: 10;
                    anchors.left: parent.left;
                    visible: phoneNumberField.text === "";  // 仅在输入框为空时显示
                    padding: 10;  // 可以根据需要调整
                }
            }
        }
    }

    Rectangle {
        id: yes;
        width: parent.width * 0.5;
        height: 20;
        color: "#E0FFFF";
        // color: "white";
        x: parent.width / 2 - this.width / 2 - 10;
        y: 310;
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
        width: parent.width * 0.5;
        height: 35;
        x: parent.width / 2 - this.width / 2;
        y: 350;
        color: "#00F5FF";
        radius: 7;


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
        id:labelRId;
        width: parent.width * 0.7;
        height: 30;
        color: "#E0FFFF";
        x: parent.width / 2 - this.width / 2 + 15;
        y: 430;
        Text {
            id: labelTid;
            width: parent.width;
            height: 20;
            text: qsTr("Copyright C 1998-2024 Tencent All Rights Reserved.");
            font.pointSize: 10;
            color: "#CDC5BF";
        }
    }

}
