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
                        if (!emailField.text){
                            loginErrorText.text="请输入邮箱后才能进行验证码的获取"
                            loginErrorText.visible=true
                        }
                        else if(!isValidEmailFormat(emailField.text)){
                            loginErrorText.text="邮箱格式不正确"
                            loginErrorText.visible=true
                        }
                        else{
                            loginErrorText.text=""
                            Controler.varifiyRequest(emailField.text,function(res){
                                if(res.error===0){
                                    verifysend.text="验证码已经发送到邮箱请注意接收\n并在三分钟内完成注册"
                                    verifysend.visible=true
                                }
                            })
                        }

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
                    if(!emailField.text){
                        loginErrorText.text="邮箱不能为空！"
                        loginErrorText.visible=true
                    }
                    else if(!codeField.text){
                        loginErrorText.text="验证码不能为空"
                        loginErrorText.visible=true
                    }
                    else if(!usernameField){
                        loginErrorText.text="用户名不能为空！"
                        loginErrorText.visible=true
                    }
                    else if(!passwordField.text){
                        loginErrorText.text="密码不能为空"
                        loginErrorText.visible=true
                    }

                    else if(confirmPasswordField.text!==passwordField.text){
                        loginErrorText.text="确认密码和密码不符"
                        loginErrorText.visible=true
                    }
                    else if(!serverUserField.text){
                        loginErrorText.text="要连接的服务器名不能为空！"
                        loginErrorText.visible=true
                    }
                    else{
                        Controler.registerRequest(emailField.text,usernameField.text,passwordField.text,
                                                  confirmPasswordField.text,serverUserField.text,codeField.text,function(res){
                                                      if(res.error===0){
                                                          console.log("注册成功！")
                                                      }
                                                  })
                    }

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
            Text {
                        id: loginErrorText
                        text: ""
                        color: "red"
                        font.pointSize: 10
                        visible: false
                        anchors {
                            // top: loginBut.bottom // 位于按钮下方
                            horizontalCenter: parent.horizontalCenter // 水平居中
                            topMargin: 10 // 间距
                        }
                    }
            Text {
                        id: verifysend
                        text: ""
                        color: "green"
                        font.pointSize: 10
                        visible: false
                        anchors {
                            // top: loginBut.bottom // 位于按钮下方
                            horizontalCenter: parent.horizontalCenter // 水平居中
                            topMargin: 10 // 间距
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
    function isValidEmailFormat(email) {
            // 正则表达式：验证基本的邮箱格式
            // 允许的格式：username@domain.com
            // 用户名部分可以包含字母、数字、点、加号、减号和下划线
            // 域名部分可以包含多级子域名，最后一部分必须是2-6个字母
        const emailRegex = /^(([^<>()\[\]\\.,;:\s@"]+(\.[^<>()\[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/

            return emailRegex.test(email)
        }
}

