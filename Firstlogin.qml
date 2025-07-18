import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "Controler.js" as Controler
import com.example 1.0
import com.startserver 1.0
Page {
    id:loginpage
    property alias loginBut: loginBut
    property alias loginErrorText: loginErrorText
    property var loginService: null //用于使用ServerLogin.h中注册的功能函数 的一个对象
    property bool showRegisterAccount: true  // 根据当前角色以及登陆网络来控制这一变量。它用于控制“注册账号”功能的可见
    property string currentLoginType: "client"
    property alias loginip: usernameField.text
    property string clientSelectedOption: "内网登陆" //默认页面为内网登陆
     background: Rectangle { color: "white" }

    visible: true
    width: windows.width //1100
    height: windows.height //700
    title: "LeYan Netdisk Login"
    // Text { //定位有点问题，看需不需要这一个文本提示
    //     text: qsTr("请选择登陆端口")
    //     font.pixelSize: 20
    //     color: "black"
    //     font.bold: true
    //     anchors {
    //         left: parent.left
    //         top: parent.top
    //         margins: 10
    //     }
    // }
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
                id:_logo_picture
                width: 250
                height: 250
                source: "qrc:/icons/software.png"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: 5
            }

            Text {
                id:_logo_name
                text: "乐研网盘"
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
                    width: 250
                    height: 30
                    // color: "white"
                    color : "deepskyblue";//点击之后变成蓝色
                    radius: 10
                    border.color: "black"
                    border.width: 1

                    Text {
                        id: clientLoginText
                        text: "Client:内网登陆"
                        anchors.centerIn: parent
                        color: "black"
                        font.pointSize: 20
                    }
                    // MouseArea {
                    //     anchors.fill: parent
                    //     onClicked: {
                    //     }
                    // }
                    TapHandler{
                        onTapped: {
                            clientLoginRect.color = "deepskyblue";//点击之后变成蓝色
                            serverLoginRect.color = "white";
                            clientLoginText.text = "Client: 外网登陆"

                            comboBox.visible = true;
                            serverIpField.visible = true;
                            registerAccountText.visible = true;
                            loginBut.visible = true;
                            passwordField.visible = true;
                            usernameField.visible = true;

                            // 只有为client内网登陆时显示注册功能
                            showRegisterAccount = (comboBox.currentIndex === 1)
                            console.log("showRegisterAccount = "+showRegisterAccount+" (false-外网；true-内网)")
                        }
                    }
                }

                ComboBox {
                    id: comboBox
                    visible: true
                    width: clientLoginRect.width
                    model: ["外网登陆", "内网登陆"]//从0开始
                    currentIndex: 1

                    onActivated: function(index) {//选择后会执行
                        currentLoginType="client"
                        clientSelectedOption = model[index];
                        clientLoginText.text = "Client: "+clientSelectedOption;
                        showRegisterAccount = (clientSelectedOption === "内网登陆");
                        // 自动激活Client登录状态
                        if (clientSelectedOption === "内网登陆") {
                            clientLoginRect.color = "deepskyblue";
                            serverLoginRect.color = "white";
                        }
                    }
                }

                Rectangle {
                    id: serverLoginRect
                    width: 250
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
                    TapHandler{
                        onTapped: {
                            serverLoginRect.color = "deepskyblue";
                            clientLoginRect.color = "white";

                            comboBox.visible = false; // 隐藏下拉框
                            comboBox.currentIndex = -1; // 重置下拉框选择
                            clientLoginText.text = "Client登陆"; // 重置文本
                            registerAccountText.visible = true;
                            forgetPassWord.visible = true;
                            loginBut.visible = true;
                            passwordField.visible = true;
                            usernameField.visible = true;

                            currentLoginType="server"
                            showRegisterAccount = true;
                            serverIpField.visible = false;
                        }
                    }
                }
            }

            TextField {
                id: serverIpField
                visible: true
                placeholderText: "服务器IP地址"
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
            }
            // 添加用户名和密码输入框
            TextField {
                id: usernameField
                // visible: false
                placeholderText: "用户账号"
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
            }

            TextField {
                id: passwordField
                placeholderText: "密码"
                // visible: false
                echoMode: TextInput.Password
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // 添加登录按钮
            Button {
                id:loginBut
                text: "登录"
                // visible: false
                width: 370
                anchors.horizontalCenter: parent.horizontalCenter
                background: Rectangle {
                    color: "deepskyblue"
                }
                onClicked: {
                    if(currentLoginType===""){
                        loginErrorText.text="请先选择登陆方式"
                        loginErrorText.visible=true
                    }

                    else if(currentLoginType==="client" && !serverIpField.text){
                        loginErrorText.text="请输入所要登陆Server的Ip"
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
                            Controler.loginRequest(serverIpField.text,usernameField.text,passwordField.text,
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
                        else{//server登陆
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
                    id: forgetPassWord
                    text: "<a href='#'>忘记密码</a>"
                    // visible: false
                    onLinkActivated: {
                        console.log("Forget password clicked")
                        lodercode()
                    }
                }

                // loginpage_.qml 中的注册链接
                Text {
                    id: registerAccountText
                    text: "<a href='#'>注册账号</a>"
                    visible: showRegisterAccount
                    enabled: showRegisterAccount
                    onLinkActivated: {
                        root.loderregister(currentLoginType);
                    }
                }

                // onLinkActivated: {
                //     var type = ""
                //     if (clientLoginRect.color === "deepskyblue" && comboBox.currentIndex === 1) {
                //         type = "client"
                //     } else if (serverLoginRect.color === "deepskyblue") {
                //         type = "server"
                //     }
                //     requestRegister(type)
                // }

            }
        }
    }
}

