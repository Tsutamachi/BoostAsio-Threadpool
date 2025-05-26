import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
// 用于控制界面的显示
Item {
    id:root
    width: parent.width
    height: parent.height
    property alias stackView: _stackView
    // 导航方法
    function lodermainwindows() {
        stackView.push(Qt.resolvedUrl("Main.qml"))
    }
    function loderregister() {
        stackView.push(Qt.resolvedUrl("register.qml"))
    }
    function loderlogin() {
        stackView.push(Qt.resolvedUrl("Firstlogin.qml"))
    }
    function removeregister() {
        stackView.pop()
        // stackView.pop()
    }
    StackView {
        id: _stackView
        anchors.fill: parent
        initialItem: firstlogin

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 200
            }
        }
    }
    Firstlogin{
        id:firstlogin
    }

}
