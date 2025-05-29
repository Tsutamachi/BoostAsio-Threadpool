import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.startserver 1.0
import com.cfileloderstart 1.0
Page {
    id: window
    width: 1100
    height: 700
    visible: true
    title: "Baidu Netdisk Clone"
    background: Rectangle { color: "#ffffff" }
    CServerStart{
        id:server;
    }
    CFileloderStart{
        id:serverfile
    }
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
}
