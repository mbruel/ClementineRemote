//========================================================================
//
// Copyright (C) 2020 Matthieu Bruel <Matthieu.Bruel@gmail.com>
//
// This file is a part of ClementineRemote : https://github.com/mbruel/ClementineRemote
//
// ClementineRemote is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 3.0 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
//========================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: aboutDialog
    property color bgGradiantStart: "#8b008b" // darkmagenta (https://doc.qt.io/qt-5/qml-color.html)
    property color bgGradiantStop:  "#ff8c00" // darkorange
    property int   mainSpacing     : 15

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: bgGradiantStart }
            GradientStop { position: 1.0; color: bgGradiantStop }
        }
        opacity: 1
    } // background

    Image {
        id: aboutLogo
        width   : 50
        height  : 50
        source  : "icon_large.png";
        fillMode: Image.PreserveAspectFit;
        anchors {
            left: parent.left
            top: parent.top
            margins: mainSpacing
        }
    } // aboutLogo
    Text {
        id: aboutTitle
        anchors {
            left: aboutLogo.right
            verticalCenter: aboutLogo.verticalCenter
            leftMargin: mainSpacing
        }
        font.pointSize: 18;
        text    : cppRemote.appName() + ' v' + cppRemote.appVersion()
    } // aboutTitle

    Flickable {
        id: flickable
        clip: true
        width: parent.width
        height: parent.height - aboutLogo.height - donateImg.height - mainSpacing*3
        contentHeight: aboutLbl.height

        anchors {
            left: parent.left
            top: aboutTitle.bottom
            topMargin: mainMargin
            leftMargin: 5
        }

        Text {
            id: aboutLbl
            linkColor: "white"
            width: parent.width
            text: qsTr('Remote for Clementine Music Player developped in C++/QT5/QML to be available on any OS.')
                  + '<br/>'+ qsTr('Its main goal is to add the Files View missing in the Android Remote.')
                  + '<br/>'+ qsTr('Extra features have been added to <b>Clementine</b>, ')
                  + qsTr('in order to use them, you need at least v') + cppRemote.clementineFilesSupportMinVersion()
                  + '<br/><br/>'+ qsTr('for more information, ')
                  + '<a href="' + cppRemote.projectURL() +'">'+qsTr('visit the github project')+'</a>'
                  +'<br/><br/>'
                  + qsTr('If you like the app and wish to reward the effort, feel free to donate what you can :)')
                  + '<br/>' + qsTr('Here is my btc address: ') + '<b>' + cppRemote.btcAddress() +'</b>'
                  wrapMode: Text.Wrap
            onLinkActivated: Qt.openUrlExternally(cppRemote.projectURL())
        }

        ScrollIndicator.vertical: ScrollIndicator {
            parent: aboutDialog.contentItem
            anchors.top: flickable.top
            anchors.bottom: flickable.bottom
            anchors.right: parent.right
            anchors.rightMargin: -aboutDialog.rightPadding + 1
        }
    }

    Rectangle {
        height: donateImg.height
        width: donateImg.width
        color: "transparent"
        anchors {
            left: parent.left
            bottom: parent.bottom
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

    Text {
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        color : "lightgray"
        text: '2020 © Matthieu Bruel'
        font {
            bold: true
            italic: true
        }
    }
}
