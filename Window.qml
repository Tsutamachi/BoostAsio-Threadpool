import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: windows
    width: 1100
    height: 700
    visible: true
    Content {
        id: content
        anchors.fill: parent // 改为填充整个窗口
        // 通过StackView的currentItem获取当前页面引用
        property var currentPage: content.stackView.currentItem
        Connections {
           target: content.currentPage
           ignoreUnknownSignals: true
           onLoginButClicked:content.lodermainwindows()
        }
        Connections {
        }
    }

}
