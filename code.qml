import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15  // 添加布局模块
//找回密码
Page {
    id: code
    title: "找回密码"

    // 不再设置固定宽高，让页面自适应父容器
    // width: 400
    // height: 600

    // 使用 background 属性设置背景色
    background: Rectangle { color: "white" }

    // 主内容区域，使用 ColumnLayout 确保组件间距和居中
    ColumnLayout {
        id: mainLayout
        anchors.centerIn: parent  // 整体内容居中
        width: Math.min(parent.width * 0.8, 400)  // 最大宽度400px，自适应窗口
        spacing: 20
        Layout.margins: 20  // 添加边距

        // 标题区域
        Column {
            Layout.alignment: Qt.AlignHCenter  // 水平居中
            spacing: 10

            Image {
                id: logo
                source: "qrc:/icons/software.png"
                width: 100
                height: 100
                Layout.alignment: Qt.AlignHCenter  // 图像自身居中
            }

            Text {
                text: "乐享网盘"
                font.pixelSize: 24
                color: "black"
                horizontalAlignment: Text.AlignHCenter  // 文本居中
            }
        }

        // 输入区域
        ColumnLayout {
            spacing: 20
            Layout.fillWidth: true  // 填充可用宽度

            TextField {
                id: usernameTextField
                placeholderText: "邮箱"
                Layout.fillWidth: true  // 宽度填满父容器
            }

            RowLayout {
                Layout.fillWidth: true  // 行宽填满父容器
                spacing: 10

                TextField {
                    id: verificationCodeInputField
                    placeholderText: "输入验证码"
                    Layout.fillWidth: true  // 占70%宽度
                }

                Button {
                    id: getCodeButton
                    text: "获取验证码"
                    Layout.preferredWidth: parent.width * 0.3  // 占30%宽度
                    enabled: true
                    background: Rectangle {color: getCodeButton.enabled?"#457ec9":"gray"}
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onPressed: {
                        console.log("发送验证码请求")
                        getCodeButton.enabled = false
                        countdownTimer.remainingSeconds = 60
                        countdownTimer.start()
                    }
                }
            }

        // 按钮区域
        Button {
            id: loginButton
            text: "确认"
            Layout.fillWidth: true  // 宽度填满父容器
            background: Rectangle {
                color: "#457ec9"
            }
            contentItem: Text {
                text: parent.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onPressed: {
                // 登录按钮按下时的处理逻辑
            }
        }

        // 底部链接
        Text {
            text: "短信快捷登录"
            color: "#457ec9"
            horizontalAlignment: Text.AlignHCenter  // 文本居中
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter  // 底部居中
            TapHandler{
                onTapped:{
                    root.removeregister()  // 返回上一页
                }
            }
        }
    }
    }

    // 倒计时计时器
    Timer {
        id: countdownTimer
        interval: 1000//1s间隔
        repeat: true
        running: false//只定义，还没开始倒计时
        property int remainingSeconds: 60
        onTriggered: {
            remainingSeconds--
            if (remainingSeconds <= 0) {
                countdownTimer.stop()
                getCodeButton.enabled = true
                getCodeButton.text = "获取验证码"
                remainingSeconds = 60
            } else {
                getCodeButton.text = "(" + remainingSeconds + "秒)"
            }
        }
    }
}
