import QtQuick 2.0

Rectangle {
    radius: 10

    anchors{
        verticalCenter: parent.verticalCenter;
        fill: parent.fill
    }

    Text {
        id: gsTitle
        anchors{
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 20
        }
        text: '<h3>' + qsTr('Global Search not implemented') + '</h3'
    }

    Text {
        id: gsText
        anchors{
            top: gsTitle.bottom;
            left: parent.left
            topMargin: 30
            leftMargin: 10
        }
        text: qsTr("As I don't use any Internet Storage, I've not implemented this part...")+
              '<br/><br/>' +
              qsTr("If you'd be interested in it, please open a ticket on ") +
              '<a href="' + cppRemote.projectURL() +'">Clementine Remote GitHub</a> ' +
              '<br/>' + qsTr('or drop me an email at Matthieu.Bruel@gmail.com') +
              '<br/><br/>' +
              qsTr('If you like the app and wish to reward the effort, feel free to donate what you can :)') +
              '<br/><br/>' + qsTr('Here is my btc address: ') + '<b>' + cppRemote.btcAddress() +'</b>'
        width: parent.width - 10
        wrapMode: Text.WordWrap
        onLinkActivated: Qt.openUrlExternally(cppRemote.projectURL())
    }

    property int qrMinHeight : parent.height - gsTitle.height - gsText.height - 60
    Image {
        id: btcImg
        fillMode: Image.PreserveAspectFit;
        anchors {
            left: parent.left
            leftMargin: 20
            top: gsText.bottom
            topMargin: 10
        }
        source: "btc_qr.gif"
        height: qrMinHeight < 150 ? qrMinHeight : 150
    }

    Rectangle {
        height: donateImg.height
        width: donateImg.width
        color: "transparent"
        anchors {
            left: btcImg.right
            verticalCenter: btcImg.verticalCenter
            leftMargin: 20
        }
        Image {
            id: donateImg
            width: 147
            height: 47
            fillMode: Image.PreserveAspectFit;
            anchors.fill: parent
            source: "icons/donate.gif"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: Qt.openUrlExternally(cppRemote.donateURL())
        }
    }
}
