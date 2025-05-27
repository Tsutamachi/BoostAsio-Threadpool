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
    function loderregister(registerType) {
        // 直接推入 Page 组件（无需创建组件实例，因为 register.qml 是 Page 类型）
        stackView.push(Qt.resolvedUrl("Register.qml"), {
            registerType: registerType  // 传递参数（需 Page 支持属性赋值）
        });
    }
    function lodercode() {
        stackView.push(Qt.resolvedUrl("code.qml"))
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
