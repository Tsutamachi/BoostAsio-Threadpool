import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "Controler.js" as Controler
import com.example 1.0
import com.startserver 1.0
Page {
    id:loginpage
    property var loginService: null
    property alias loginBut: loginBut
    property alias loginErrorText: loginErrorText
    property bool showRegisterAccount: false  // 控制注册账号链接显示
    property string currentRegisterType: ""   // 记录当前注册类型
    property string currentLoginType: ""

    property string currentLoginMode: ""
     background: Rectangle { color: "white" }
    visible: true
    width: 1100
    height: 700
    // color: "white"
    title: "Baidu Netdisk Login"
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
                            showRegisterAccount = (comboBox.currentIndex === 1) // 只有选项2时显示注册
                            currentLoginMode = "client"
                            serverIpField.visible = true;
                        }
                    }
                }

                ComboBox {
                    id: comboBox
                    visible: false
                    width: clientLoginRect.width
                    model: ["选项1", "选项2"]

                    onActivated: function(index) {
                        currentLoginType="client"
                        var selectedText = model[index];
                        clientLoginText.text = selectedText;
                        showRegisterAccount = (selectedText === "选项2");
                        // 新增：自动激活Client登录状态
                        if (selectedText === "选项2") {
                            clientLoginRect.color = "#457ec9";
                            serverLoginRect.color = "white";
                            currentRegisterType="client"
                            console.log(currentRegisterType)
                        }
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
                            comboBox.visible = false; // 隐藏下拉框
                            comboBox.currentIndex = -1; // 重置下拉框选择
                            clientLoginText.text = "Client登陆"; // 重置文本
                            showRegisterAccount = true;
                            currentLoginMode = "server"
                            currentLoginType="server"
                            serverIpField.visible = false;
                        }
                    }
                }
            }

            TextField {
                id: serverIpField
                visible: false
                placeholderText: "服务器IP地址"
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
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
                background: Rectangle {
                    color: "#457ec9"
                }
                onClicked: {
                    if(currentLoginType===""){

                        loginErrorText.text="请先选择登陆方式"
                        loginErrorText.visible=true
                    }
                    else if (!usernameField.text){
                        loginErrorText.text="请先输入用户名"
                        loginErrorText.visible=true

                    }
                    else if(!passwordField.text){
                        loginErrorText.text="用户密码不可以为空！请输入密码后尝试登陆"
                        loginErrorText.visible=true
                    }
                    else{
                        if(currentLoginType==="client"){
                             console.log(currentRegisterType)
                            Controler.loginRequest(usernameField.text,passwordField.text,
                                                                         function (response) {

                                                                             if (response.error === 0) {
                                                                                 lodermainwindows()
                                                                             }
                                                                             else if(response.error===1009)
                                                                             {
                                                                                 loginErrorText.text = ("登录失败:用户名或密码错误")
                                                                                loginErrorText.visible = true // 显示错误文本

                                                                             }
                                                                             else {
                                                                                 loginErrorText.text = ''+response.message
                                                                                loginErrorText.visible = true // 显示错误文本
                                                                             }
                                                                         })
                        }
                        else{
                            const loginService = Qt.createQmlObject("import com.example 1.0; ServerLogin {}", this)
                            let c=loginService.serverLogin(usernameField.text,passwordField.text)
                            if (c===0){
                                loderserverlogin()
                            }
                            else if(c===1){
                                loginErrorText.text = ("登录失败:server邮箱未找到")
                               loginErrorText.visible = true // 显示错误文本
                            }
                            else if(c===2){
                                loginErrorText.text = ("登录失败:server密码与注册邮箱不匹配")
                               loginErrorText.visible = true // 显示错误文本
                            }
                            else{
                                loginErrorText.text = ("登陆服务器未知错误！")
                               loginErrorText.visible = true // 显示错误文本
                            }
                        }
                      }

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
            // 添加忘记密码链接
            Row {
                            spacing: 30
                            anchors.horizontalCenter: parent.horizontalCenter

                            Text {
                                text: "<a href='#'>忘记密码？</a>"
                                onLinkActivated: {
                                    console.log("Forget password clicked")
                                    lodercode()
                                }
                            }

                            // loginpage.qml 中的注册链接
                            // loginpage.qml 中的注册链接
                            Text {
                                id: registerAccountText
                                text: "<a href='#'>注册账号？</a>"
                                visible: showRegisterAccount
                                enabled: showRegisterAccount
                                onLinkActivated: {
                                    root.loderregister(currentLoginMode);

                                }
                            }

                    // onLinkActivated: {
                    //     var type = ""
                    //     if (clientLoginRect.color === "#457ec9" && comboBox.currentIndex === 1) {
                    //         type = "client"
                    //     } else if (serverLoginRect.color === "#457ec9") {
                    //         type = "server"
                    //     }
                    //     requestRegister(type)
                    // }

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
