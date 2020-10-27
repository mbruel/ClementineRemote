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
import QtQuick.Controls 2.14

Rectangle {
    property alias lbl: lbl
    property int iconSize: 30

    radius: 10
    opacity: 0.8

    signal goBack

    Text  {
        id: lbl
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter;
        }
        text: "not set...";
    }

    Item {
        id: backButton
        visible: false

        anchors {
            bottom: parent.bottom
            left: parent.left
            margins:20
        }

        width: back.width
        height: back.height

        Image {
            id: back
            source: "icons/back.png";
            fillMode: Image.PreserveAspectFit;
            width: iconSize
            height: iconSize
            opacity: 0.7
        }

        MouseArea{
            hoverEnabled: true
            anchors.fill: back
            onEntered: back.opacity = 1;
            onExited : back.opacity = 0.7;
            onClicked: goBack();
        }
    }

    function enableBackButton(enable)
    {
        backButton.visible = true;
    }
}
