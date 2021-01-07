//========================================================================
//
// Copyright (C) 2020 Matthieu Bruel <Matthieu.Bruel@gmail.com>
// This file is a part of ClementineRemote : https://github.com/mbruel/ClementineRemote
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3..
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>
//
//========================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    radius: 10
    id: gsRect

    property int qrBtcSize: 120

    anchors.fill: parent.fill

    Flickable {
        id: flickable
        clip: true
        width: parent.width
        height: parent.height
        contentHeight: gsText.height + gsTitle.height + qrBtcSize + 60

        ScrollIndicator.vertical: ScrollIndicator {
            parent: gsRect
            anchors.top: flickable.top
            anchors.bottom: flickable.bottom
            anchors.right: parent.right
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
                  qsTr("If you'd be interested in it, please open a request/issue on ") +
                  '<a href="' + cppRemote.projectURL() +'/issues">Clementine Remote GitHub</a> ' +
                  '<br/>' + qsTr('or drop me an email at Matthieu.Bruel@gmail.com') +
                  '<br/><br/>' +
                  qsTr('If you like the app and wish to reward the effort, feel free to donate what you can :)') +
                  '<br/><br/>' + qsTr('Here is my btc address: ') + '<b>' + cppRemote.btcAddress() +'</b>'
            width: parent.width - 10
            wrapMode: Text.WordWrap
            onLinkActivated: Qt.openUrlExternally(cppRemote.projectURL()+'/issues')
        }

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
            width: qrBtcSize
            height: qrBtcSize
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
}
