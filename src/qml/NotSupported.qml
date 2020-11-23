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
