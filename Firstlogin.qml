import QtQuick 2.15
import QtQuick.Controls 2.15
import com.example

Page {
    signal loginButClicked
     property alias loginBut: loginBut
     background: Rectangle { color: "white" }
    visible: true
    width: 1100
    height: 700
    // color: "white"
    title: "Baidu Netdisk Login"
    CClient{
        id:_client
    }
    Text {
        text: qsTr("请选择登陆端口")
        font.pixelSize: 20
        color: "black"
        font.bold: true
        anchors {
            left: parent.left
            top: parent.top
            margins: 10
        }
    }
    Rectangle {
        width: parent.width * 0.8
        height: parent.height * 0.8
        color: "white"
        anchors.centerIn: parent
        Column {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5

            Image {
                width: 250
                height: 250
                source: "qrc:/icons/software.png"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: 5
            }

            Text {
                text: "乐享网盘"
                font.pointSize: 30
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: 5
            }

            Row {
                spacing: 30
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    id: clientLoginRect
                    width: 200
                    height: 30
                    color: "white"
                    radius: 10
                    border.color: "black"
                    border.width: 1

                    Text {
                        id: clientLoginText
                        text: "Client登陆"
                        anchors.centerIn: parent
                        color: "black"
                        font.pointSize: 20
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            clientLoginRect.color = "#457ec9";
                            serverLoginRect.color = "white";

                            comboBox.visible = true;
                        }
                    }
                }

                ComboBox {
                    id: comboBox
                    visible: false
                    width: clientLoginRect.width
                    model: ["选项1", "选项2"]

                    onActivated: function(index) {
                        var selectedText = model[index];
                        clientLoginText.text = selectedText;
                    }
                }

                Rectangle {
                    id: serverLoginRect
                    width: 200
                    height: 30
                    color: "white"
                    radius: 10
                    border.color: "black"
                    border.width: 1
                    Text {
                        text: "Server登陆"
                        font.pointSize: 20
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            serverLoginRect.color = "#457ec9";
                            clientLoginRect.color = "white";
                        }
                    }
                }
            }

            // 添加用户名和密码输入框
            TextField {
                id: usernameField
                placeholderText: "用户账号"
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
            }

            TextField {
                id: passwordField
                placeholderText: "密码"
                echoMode: TextInput.Password
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // 添加登录按钮
            Button {
                id:loginBut
                text: "登录"
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    loginButClicked()
                    _client.useremail= usernameField.text
                    _client.password=passwordField.text
                    _client.on_get_login_clicked()
                    // console.log("Username:", usernameField.text)
                    // console.log("Password:", passwordField.text)
                    // 这里可以添加登录逻辑
                }
            }

            // 添加忘记密码链接
            Text {
                text: "<a href='#'>忘记密码？</a>"
                anchors.horizontalCenter: parent.horizontalCenter
                onLinkActivated: {
                    console.log("Forget password clicked")
                    // 这里可以添加链接跳转逻辑
                }
            }
        }
    }
    // Rectangle {
    //     id: confirmRect
    //     width: 100
    //     height: 40
    //     color: "#457ec9"
    //     radius: 10

    //     anchors {
    //         bottom: parent.bottom
    //         right: parent.right
    //         margins: 20
    //     }

    //     Text {
    //         text: "确定"
    //         anchors.centerIn: parent
    //         color: "white"
    //         font.bold: true
    //     }

    //     MouseArea {
    //         anchors.fill: parent
    //         onClicked: {
    //             // 这里可以添加确定按钮的点击事件处理逻辑
    //             console.log("确定按钮被点击");
    //         }
    //     }
    // }
}
