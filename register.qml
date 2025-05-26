import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "Controler.js" as Controler
ApplicationWindow {
    id:resgister
    visible: true
    width: 400
    height: registerType === "client" ? 650 : 600  // 动态调整高度
    title: registerType === "client" ? "客户端注册" : "服务端注册"
    color: "white"

    // 接收注册类型参数
    property string registerType: "client"

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: columnLayout.height + 50

        ColumnLayout {
            id: columnLayout
            width: parent.width
            spacing: 15
            anchors.top: parent.top
            anchors.topMargin: 30

            // 公共字段
            Image {
                source: "qrc:/icons/software.png"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100
                Layout.alignment: Qt.AlignHCenter
            }

            TextField {
                id: emailField
                placeholderText: "邮箱"
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15

                TextField {
                    id: codeField
                    placeholderText: "验证码"
                    Layout.fillWidth: true
                }

                Button {
                    text: "获取验证码"
                    Layout.preferredWidth: 100
                    onClicked: {console.log("发送验证码到:", emailField.text)
                        Controler.varifiyRequest(emailField.text,function(res){
                            if(res.error===0){
                                console.log("验证码发送成功！")
                            }
                        })
                    }
                }
            }

            TextField {
                id: usernameField
                placeholderText: "用户名"
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
            }

            TextField {
                id: passwordField
                placeholderText: "密码"
                echoMode: TextInput.Password
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
            }

            TextField {
                id: confirmPasswordField
                placeholderText: "确认密码"
                echoMode: TextInput.Password
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
            }

            // 仅Client注册显示
            TextField {
                id: serverUserField
                visible: registerType === "client"
                placeholderText: "服务器用户名"
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
            }

            Button {
                text: "注册"
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
                onClicked: {
                    Controler.registerRequest(emailField.text,
                                              usernameField.text,
                                              passwordField.text,
                                              confirmPasswordField.text,
                                              serverUserField.text,
                                              codeField.text,
                                              function(res){
                                                  if(res.error===0){
                                                      console.log("验证码发送成功！")
                                                  }
                                              })
                }
            }

            Button {
                text: "返回登录"
                Layout.fillWidth: true
                Layout.leftMargin: 15
                Layout.rightMargin: 15
                onClicked: {
                    // if (typeof parent.parent.parent.backToLogin === "function") {
                    //     parent.parent.parent.backToLogin()
                    // }
                    // loderlogin()
                    removeregister()
                    resgister.close()
                }
            }
        }
    }

    function handleRegister() {
        console.log("注册类型:", registerType)
        console.log("邮箱:", emailField.text)
        console.log("验证码:", codeField.text)
        console.log("用户名:", usernameField.text)
        console.log("密码:", passwordField.text)
        if (registerType === "client") {
            console.log("服务器用户名:", serverUserField.text)
        }
    }
}
