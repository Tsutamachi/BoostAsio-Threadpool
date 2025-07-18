import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
//Client的主页面
Page {
    id: window
    width: 1100
    height: 700
    visible: true
    title: "LeYan Netdisk Clone"
    background: Rectangle { color: "#ffffff" }
    // 顶部栏
    Rectangle {
        id: topBar
        width: parent.width
        height: 40 // 可以根据需要调整高度
        color: "white" // 顶部栏的背景颜色
        border.color: "#cccccc" // 灰色边框
        border.width: 1 // 边框宽度为1像素
        Image {
            id: logoImage
            source: "qrc:/icons/software.png"
            height: parent.height // 可以根据需要调整图片高度
            width: 50
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            // 可以在这里添加其他顶部栏元素，如标题或按钮
        }
        Text {
                   id: titleText
                   text: "乐享网盘"
                   font.pixelSize: 20 // 可以根据需要调整字体大小
                   color: "#457ec9" // 文字颜色
                   anchors.left: logoImage.right
                   anchors.leftMargin: 10 // 标题与图片的间距
                   anchors.verticalCenter: parent.verticalCenter
               }
        Rectangle {
                id: rightImageBackground
                color: "#457ec9" // 设置您想要的背景颜色
                radius: width / 2 // 使矩形变为圆形
                anchors.right: topBar.right // 右侧锚点与顶部栏右侧对齐
                anchors.verticalCenter: topBar.verticalCenter // 垂直居中
                anchors.rightMargin: 10 // 与顶部栏右侧的间距
                width: 30
                height: 30

                Image {
                    id: rightImage
                    source: "qrc:/icons/login.png" // 替换为你的图片路径
                    anchors.centerIn: parent // 使图片居中于背景矩形
                    width: parent.width * 0.8 // 图片宽度为背景矩形的80%
                    height: parent.height * 0.8 // 图片高度为背景矩形的80%
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    id: imageMouseArea
                    anchors.fill: parent // 使MouseArea覆盖整个背景矩形
                    onClicked: {
                        // 在这里定义点击图片后触发的事件
                        // 例如，可以打印一条消息到控制台
                        console.log("login clicked!");
                    }
                }
            }

    }
    Rectangle {
        id: sidebar
        width: 60 // 侧边栏宽度
        height: parent.height - topBar.height
        y: topBar.height // 将侧边栏放置在顶部栏下方
        color: "#f0f0f0"

        Column {
            id: sidebarContent
            anchors.fill: parent
            spacing: 10 // 图片和文字之间的间距
            anchors.margins: 10 // 边距

            // 首页图片和文字
            Item {
                width: parent.width
                height: childrenRect.height
                Rectangle {
                        id: imageBackground1
                        color: "#457ec9" // 设置您想要的背景颜色
                        radius: width / 2 // 使矩形变为圆形
                        width: 36
                        height: 36
                Image {
                    id: image1
                    source: "qrc:/icons/home.png" // 替换为您的图片路径
                    width: parent.width - 1 * sidebarContent.anchors.margins
                    fillMode: Image.PreserveAspectFit
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                }

                Text {
                    id: text1
                    text: "首页"
                    color: "#000000" // 默认黑色
                    font.pixelSize: 12
                    anchors.top: imageBackground1.bottom
                    anchors.horizontalCenter: parent.horizontalCenter

                    MouseArea {
                        id: mouseArea1
                        anchors.fill: parent
                        onClicked: {
                            text1.color = "#457ec9"; // 点击后变蓝
                            // 处理首页点击事件
                            console.log("首页 clicked");
                            // 使用单次定时器来恢复颜色
                            timer1.start();
                        }
                    }
                }
            }

            // 传输图片和文字
            Item {
                width: parent.width
                height: childrenRect.height
                Rectangle {
                        id: imageBackground2
                        color: "#457ec9" // 设置您想要的背景颜色
                        radius: width / 2 // 使矩形变为圆形
                        width: 36
                        height: 36
                Image {
                    id: image2
                    source: "qrc:/icons/upload.png" // 替换为您的图片路径
                    width: parent.width - 1 * sidebarContent.anchors.margins
                    fillMode: Image.PreserveAspectFit
                    anchors.horizontalCenter: parent.horizontalCenter
                }
}
                Text {
                    id: text2
                    text: "传输"
                    color: "#000000" // 默认黑色
                    font.pixelSize: 12
                    anchors.top: imageBackground2.bottom
                    anchors.horizontalCenter: parent.horizontalCenter

                    MouseArea {
                        id: mouseArea2
                        anchors.fill: parent
                        onClicked: {
                            text2.color = "#457ec9"; // 点击后变蓝
                            // 处理传输点击事件
                            console.log("传输 clicked");
                            // 使用单次定时器来恢复颜色
                            timer2.start();
                        }
                    }
                }
            }

            // 信息图片和文字
            Item {
                width: parent.width
                height: childrenRect.height
                Rectangle {
                        id: imageBackground3
                        color: "#457ec9" // 设置您想要的背景颜色
                        radius: width / 2 // 使矩形变为圆形
                        width: 36
                        height: 36
                Image {
                    id: image3
                    source: "qrc:/icons/message.png" // 替换为您的图片路径
                    width: parent.width - 1 * sidebarContent.anchors.margins
                    fillMode: Image.PreserveAspectFit
                    anchors.horizontalCenter: parent.horizontalCenter
                }
}
                Text {
                    id: text3
                    text: "信息"
                    color: "#000000" // 默认黑色
                    font.pixelSize: 12
                    anchors.top: imageBackground3.bottom
                    anchors.horizontalCenter: parent.horizontalCenter

                    MouseArea {
                        id: mouseArea3
                        anchors.fill: parent
                        onClicked: {
                            text3.color = "#457ec9"; // 点击后变蓝
                            // 处理信息点击事件
                            console.log("信息 clicked");
                            // 使用单次定时器来恢复颜色
                            timer3.start();
                        }
                    }
                }
            }
        }
        Image {
                    id: bottomImage
                    source: "qrc:/icons/seting.png" // 替换为你的图片路径
                    anchors.bottom: settingsText.top // 图片底部与文字顶部对齐
                    anchors.horizontalCenter: parent.horizontalCenter // 水平居中
                    // 可以根据需要设置图片的宽度和高度
                    width: 30
                    height: 30
                    // 如果需要保持图片比例，可以设置如下
                    // fillMode: Image.PreserveAspectFit
                }

                // 文字“设置”
                Text {
                    id: settingsText
                    text: "设置"
                    font.pixelSize: 14 // 可以根据需要调整字体大小
                    color: "#000000" // 文字颜色
                    anchors.bottom: parent.bottom // 文字底部与侧边栏底部对齐
                    anchors.horizontalCenter: parent.horizontalCenter // 水平居中
                    bottomPadding: 10 // 与侧边栏底部的间距
                    MouseArea {
                        id: mouseArea4
                        anchors.fill: parent
                        onClicked: {
                            settingsText.color = "#457ec9"; // 点击后变蓝
                            // 处理传输点击事件
                            console.log("设置 clicked");
                            // 使用单次定时器来恢复颜色
                            timer4.start();
                        }
                    }
                }
        }
    // 第二个侧边栏
    Rectangle {
        id: sidebar2
        width: 170
        height: parent.height - topBar.height
        x: sidebar.width
        y: topBar.height
        color: "#ffffff" // 第二个侧边栏背景颜色
        border.color: "#cccccc" // 灰色边框分隔线
        border.width: 1
        property int myFilesClickCount: 0 // 计数器，跟踪“我的文件”被点击的次数

        property bool isMyFilesExpanded: false // 状态变量，控制“我的文件”下拉框

        Column {
            spacing: 15 // 文本之间的间距
            anchors.top: parent.top // 顶部对齐
            anchors.horizontalCenter: parent.horizontalCenter // 水平居中
            anchors.margins: 10 // 边缘间距

            Text {
                id: myFilesText
                text: "我的文件"
                font.family: "Arial"
                font.pointSize: 14
                color: "#000000"
                MouseArea {
                               id: myFilesMouseArea
                               anchors.fill: parent
                               onClicked: {
                                   sidebar2.myFilesClickCount++; // 增加点击次数
                                   if (sidebar2.myFilesClickCount === 1) {
                                       myFilesText.color = "#457ec9";
                                       console.log("我的文件被点击"); // 第一次点击时输出
                                       timer7.start();
                                   } else if (sidebar2.myFilesClickCount === 2) {
                                       sidebar2.isMyFilesExpanded = true; // 第二次点击时显示下拉框
                                       myFilesOptions.height = 100; // 根据需要调整高度
                                       myFilesOptions.visible = true;
                                   } else {
                                       sidebar2.isMyFilesExpanded = !sidebar2.isMyFilesExpanded; // 之后点击切换下拉框显示
                                       myFilesOptions.height = sidebar2.isMyFilesExpanded ? 100 : 0;
                                       myFilesOptions.visible = sidebar2.isMyFilesExpanded;
                                   }
                               }
                           }
                       }

            Rectangle {
                id: myFilesOptions
                width: parent.width
                height: 0 // 默认高度为0，点击后展开
                color: "#ffffff"
                visible: false

                Column {
                    spacing: 5
                    anchors.fill: parent
                    Text {
                        text: "文件1"
                        font.family: "Arial"
                        font.pointSize: 12
                        color: "#000000"
                    }
                    Text {
                        text: "文件2"
                        font.family: "Arial"
                        font.pointSize: 12
                        color: "#000000"
                    }
                    // ... 更多文件选项
                    Text {
                        text: "文件3"
                        font.family: "Arial"
                        font.pointSize: 12
                        color: "#000000"
                    }
                }
            }

            Text {
                id: mySharesText
                text: "我的分享"
                font.family: "Arial"
                font.pointSize: 14
                color: "#000000"
                y: myFilesOptions.visible ? myFilesOptions.height : 0
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        mySharesText.color = "#457ec9";
                        // 处理“我的分享”点击事件
                        console.log("我的分享被点击");
                        timer5.start();
                    }
                }
            }

            Text {
                id: recycleBinText
                text: "回收站"
                font.family: "Arial"
                font.pointSize: 14
                color: "#000000"
                y: myFilesOptions.visible ? myFilesOptions.height : 0
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        recycleBinText.color = "#457ec9";
                        // 处理“回收站”点击事件
                        console.log("回收站被点击");
                        timer6.start();
                    }
                }
            }
        }
    }


    // 定时器用于恢复文本颜色
    Timer {
        id: timer1
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: text1.color = "#000000" // 恢复黑色
        repeat: false
    }

    Timer {
        id: timer2
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: text2.color = "#000000" // 恢复黑色
        repeat: false
    }

    Timer {
        id: timer3
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: text3.color = "#000000" // 恢复黑色
        repeat: false
    }
    Timer {
        id: timer4
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: settingsText.color = "#000000" // 恢复黑色
        repeat: false
    }
    Timer {
        id: timer5
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: mySharesText.color = "#000000" // 恢复黑色
        repeat: false
    }
    Timer {
        id: timer6
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: recycleBinText.color = "#000000" // 恢复黑色
        repeat: false
    }
    Timer {
        id: timer7
        interval: 300 // 持续时间，可以根据需要调整
        onTriggered: myFilesText.color = "#000000" // 恢复黑色
        repeat: false
    }
    // 主内容区域
    Rectangle {
        id: contentArea
        x: sidebar.width
        y: topBar.height
        width: parent.width - sidebar.width
        height: parent.height - topBar.height
        color: "transparent"
        // StackView用于页面切换
        StackView {
            id: stackView
            anchors.fill: parent
            initialItem: Rectangle {
                color: "transparent"
            }
        }
    }
}
