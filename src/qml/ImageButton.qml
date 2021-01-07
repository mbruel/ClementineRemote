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

import QtQuick 2.0

Rectangle {
    property alias source: img.source

    property int size: 24
    property color colorHovered: 'blue'
    property color colorDefault: 'transparent'
    property bool hovered: false // in case we want to use tooltips on hovered

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
        onEntered: {hovered = true;  parent.color = colorHovered; }
        onExited:  {hovered = false; parent.color = colorDefault; }
        onClicked: parent.clicked();
    }
}
