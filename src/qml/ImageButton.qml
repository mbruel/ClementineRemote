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
    property alias source: img.source

    property int size: 24
    property color colorHovered: 'blue'
    property color colorDefault: 'transparent'

    signal clicked;
    height: size
    width:  size
    color: colorDefault

    Image {
        id: img
        fillMode: Image.PreserveAspectFit;
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: parent.color = colorHovered
        onExited:  parent.color = colorDefault
        onClicked: parent.clicked();
    }
}
