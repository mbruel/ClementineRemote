import QtQuick 2.0

Rectangle {
    property string title
    radius: 10

    color: "lightgray"

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
              '<b>' + cppRemote.clementineFilesSupportMinVersion() +'</b>'+
              '<br/><br/>' +
              qsTr("As Clementine team wasn't so communicative on the date for the next release, ") +
              qsTr("I've built one myself on the fork I was working on.")+ '<br/>'+
              qsTr("Feel free to download and try it, it's available for most OS :)") + '<br/>' +
              "It's here: " +
              '<a href="' + cppRemote.clementineReleaseURL() + "'>Clementine 1.4 rc1ClemRemote</a>";

        width: parent.width - 10
        wrapMode: Text.WordWrap
        onLinkActivated: Qt.openUrlExternally(cppRemote.clementineReleaseURL())
    }
}
