import QtQuick 2.15
import QtQuick.Controls 2.15
// cf https://doc.qt.io/qt-5/qtquickcontrols2-customize.html#customizing-switch
Switch {
    id: control
    property int sliderSize: 26
    indicator: Rectangle {
        implicitWidth: parent.width
        implicitHeight: sliderSize
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 13
        color: control.checked ? "#17a81a" : "#ffffff"
        border.color: control.checked ? "#17a81a" : "#cccccc"

        Rectangle {
            x: control.checked ? parent.width - width : 0
            width: sliderSize
            height: sliderSize
            radius: 13
            color: control.down ? "#cccccc" : "#ffffff"
            border.color: control.checked ? (control.down ? "#17a81a" : "#21be2b") : "#999999"
        }
    }
}
