import QtQuick 2.0

Rectangle {
    property string title

    anchors{
        verticalCenter: parent.verticalCenter;
        fill: parent.fill
    }

    Text {
        anchors{
            verticalCenter: parent.verticalCenter;
            left: parent.left
            leftMargin: 10
        }
        text: '<b>' + title + qsTr(' are not supported') + '</b>' + qsTr(' server side.') +
              '<br/>' + qsTr('The server is running: ')+ cppRemote.clemVersion() +
              '<br/>' + qsTr('it should at least be v') +
              '<b>' + cppRemote.clementineFilesSupportMinVersion() +'</b>'

        width: parent.width - 10
        wrapMode: Text.WordWrap
    }
}
