import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.register 1.0
// import QtQuick.Controls.Validator 2.15
import "Controler.js" as Controler
Page {
    id: register

    property  alias ipregister: serverUserField.text//?
    property alias loginErrorText: loginErrorText

    property string registerType: "client"

    title: registerType === "client" ? "客户端注册" : "服务端注册"
    background: Rectangle {color: "white"}
    // 接收注册类型参数
    ColumnLayout {
        id: mainLayout
        anchors.centerIn: parent  // 整体内容居中
        width: Math.min(parent.width * 0.8, 400)  // 最大宽度400px，自适应窗口
        spacing: 15

        // 公共字段
        Image {
            source: "qrc:/icons/software.png"
            Layout.preferredWidth: 150
            Layout.preferredHeight: 150
            Layout.alignment: Qt.AlignHCenter
        }
        TextField {
            id: emailField
            placeholderText: "邮箱"
            // visible:registerType === "client"
            Layout.fillWidth: true

        }
        // 仅Client注册显示
        TextField {
            id: serverUserField
            visible: registerType === "client"
            placeholderText: "服务器用户名"
            Layout.fillWidth: true

        }

        RowLayout {
            Layout.fillWidth: true
            visible:registerType === "client"

            TextField {
                id: codeField
                placeholderText: "验证码"
                Layout.fillWidth: true
            }

            Button {
                id:getConfirmBtn
                text: "获取验证码"
                enabled: true
                Layout.preferredWidth: 100
                background: Rectangle {color: getConfirmBtn.enabled?"#457ec9":"gray"}
                onClicked: {
                    console.log("发送验证码到:", emailField.text)
                    if (!emailField.text){
                        loginErrorText.text="请输入邮箱后才能进行验证码的获取"
                        loginErrorText.visible=true
                        return
                    }
                    else if(!isValidEmailFormat(emailField.text)){
                        loginErrorText.text="邮箱格式不正确"
                        loginErrorText.visible=true
                        return
                    }
                    else if(!serverUserField.text){
                        loginErrorText.text="请先指定服务器进行验证码获取服务！"
                        loginErrorText.visible=true
                        return
                    }
                    else{
                        loginErrorText.text=""
                        Controler.verifyEmailRequest(emailField.text,serverUserField.text,function(res){
                            if(res.error===0){
                                verifysend.text="验证码已经发送到邮箱请注意接收\n并在三分钟内完成注册"
                                countdownTimer.remainingSeconds = 180
                                getConfirmBtn.text = countdownTimer.remainingSeconds
                                countdownTimer.start()
                                verifysend.visible=true
                            }
                            else if(res.error===1002){
                                loginErrorText.text="服务器端邮箱验证服务未启动，请通知服务器启动！"
                                loginErrorText.visible=true
                            }
                            else{
                                loginErrorText.text=""+res.error
                                loginErrorText.visible=true
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
        }

        TextField {
            id: passwordField
            placeholderText: "密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true

        }

        TextField {
            id: confirmPasswordField
            placeholderText: "确认密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true

        }



        Button {
            text: "注册"
            Layout.fillWidth: true
            background: Rectangle {
                color: "#457ec9"
            }
            onClicked: {
                if(!usernameField.text){
                    loginErrorText.text="用户名不能为空！"
                    loginErrorText.visible=true
                    return
                }
                else if(!passwordField.text){
                    loginErrorText.text="密码不能为空"
                    loginErrorText.visible=true
                    return
                }
                else if(!isValidpassword(passwordField.text)){
                    loginErrorText.text="密码不符合规则密码长度必须大于或等于8位\n,并且必须包含数字，大小写字母"
                    loginErrorText.visible=true
                    return
                }
                else if(confirmPasswordField.text!==passwordField.text){
                    loginErrorText.text="确认密码和密码不符"
                    loginErrorText.visible=true
                    return
                }


                if (registerType === "client"){//client注册
                    if(!emailField.text){
                        loginErrorText.text="邮箱不能为空！"
                        loginErrorText.visible=true
                        return
                    }
                    else if(!codeField.text){
                        loginErrorText.text="验证码不能为空"
                        loginErrorText.visible=true
                        return
                    }
                    else if(!serverUserField.text){
                        loginErrorText.text="要连接的服务器名不能为空！"
                        loginErrorText.visible=true
                        return
                    }
                    Controler.registerRequest(emailField.text,usernameField.text,passwordField.text,
                                              confirmPasswordField.text,serverUserField.text,codeField.text,function(res){
                                                  if(res.error===0){
                                                      verifysend.text="注册成功"
                                                      lodermainwindows()
                                                  }
                                                  else if(res.error===1003){
                                                      loginErrorText.text="验证码已过期请重新获取"
                                                      loginErrorText.visible=true
                                                  }
                                                  else if(res.error===1004){
                                                      loginErrorText.text="验证码不匹配"
                                                      loginErrorText.visible=true
                                                  }
                                                  else if (res.error===1005){
                                                      loginErrorText.text="邮箱已经被注册或这个用户名已被占用！"
                                                      loginErrorText.visible=true
                                                  }
                                                  else{
                                                      loginErrorText.text="邮箱已经被注册或这个用户名已被占用！"
                                                      loginErrorText.visible=true
                                                  }
                                              })
                }
                else{//server注册
                    const registerService = Qt.createQmlObject("import com.register 1.0; ServerRegister {}", this)
                    let c=registerService.registerServer(usernameField.text,passwordField.text,emailField.text)
                    if(c===0){
                        loderserverlogin()
                    }
                    else if(c===1){
                        loginErrorText.text="邮箱已经被注册"
                        loginErrorText.visible=true
                    }
                    else{
                        loginErrorText.text="用户名已被使用"
                        loginErrorText.visible=true
                    }
                }
            }
        }

        Button {
            text: "返回登录"
            Layout.fillWidth: true
            background: Rectangle {
                color: "#457ec9"
            }
            onClicked: {
                // if (typeof parent.parent.parent.backToLogin === "function") {
                //     parent.parent.parent.backToLogin()
                // }
                loderlogin()
                // removeregister()
                // resgister.close()
            }
        }
        Text {
            id: loginErrorText
            text: ""
            color: "red"
            font.pointSize: 10
            visible: false
            // anchors {
            //     // top: loginBut.bottom // 位于按钮下方
            //     horizontalCenter: parent.horizontalCenter // 水平居中
            //     topMargin: 10 // 间距
            // }
            Layout.fillWidth: true // 让文本组件填满可用宽度
            horizontalAlignment: Text.AlignHCenter // 文本内容水平居中
            Layout.topMargin: 10 // 使用布局边距代替锚点边距
        }
        Text {
            id: verifysend
            text: ""
            color: "green"
            font.pointSize: 10
            visible: false
            // anchors {
            //     // top: loginBut.bottom // 位于按钮下方
            //     horizontalCenter: parent.horizontalCenter // 水平居中
            //     topMargin: 10 // 间距
            // }
            Layout.fillWidth: true // 让文本组件填满可用宽度
            horizontalAlignment: Text.AlignHCenter // 文本内容水平居中
            Layout.topMargin: 10 // 使用布局边距代替锚点边距
        }
    }
    // 倒计时计时器
    Timer {
        id: countdownTimer
        interval: 1000//1s间隔
        repeat: true
        running: false//只定义，还没开始倒计时
        property int remainingSeconds: 180
        onTriggered: {
            remainingSeconds--
            getConfirmBtn.enabled = false
            if (remainingSeconds <= 0) {
                countdownTimer.stop()
                getConfirmBtn.enabled = true
                getConfirmBtn.text = "获取验证码"
                remainingSeconds = 180
            } else {
                getConfirmBtn.text = "(" + remainingSeconds + "秒)"
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

    function isValidpassword(password) {
        // 包括大小写以及字符数不少于8个
        const passwordRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d).{8,}$/

        return passwordRegex.test(password)
    }
}

