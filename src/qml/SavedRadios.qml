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
import QtQuick.Controls 2.15

import RadioStream 1.0

Rectangle {
    id: radioStream
    radius: 10

    property int   lineSpacing    : 10
    property int   lineHeigth     : 45
    property color colorSelected  : "lightblue"


    ListView {
        id: filesView
        focus: true

        model: RadioStreamModel {
            remote: cppRemote
        }
        delegate: radioStreamDelegate

        anchors{
            top: parent.top
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height

        clip: true
    }

    Component {
        id: radioStreamDelegate

        Rectangle {
            id: radioStreamDelegateRect
//            property bool isSelected: ListView.isCurrentItem

            width: ListView.view.width
            height: lineHeigth

            color: ListView.isCurrentItem ? colorSelected : "white"

            Image {
                id: icon
                x: lineSpacing
                width: lineHeigth
                height: lineHeigth - 4
                source: logoUrl
            }

            Text{
                id: txtRadio
                text: name
                x: lineHeigth + 2*lineSpacing

                anchors.verticalCenter: icon.verticalCenter

                elide: Text.ElideRight
                width: parent.width - icon.width - 3*lineSpacing
            }

            MouseArea {
                anchors.fill: parent
                onClicked: filesView.currentIndex = index;

                onDoubleClicked: {
                    filesView.currentIndex = index;
                    cppRemote.addRadioToPlaylist(index);
                }
            }
        }
    }
}
