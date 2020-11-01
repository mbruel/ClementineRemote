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

import RemoteFile 1.0

Rectangle {
    id: root
    radius: 10

    property bool  editMode: false
    property int headerButtonSize   : 30
    property int headerSpacing      : 5

    property int lineHeigth: 25

    property color colorSongSelected: "lightblue"
    property color colorSongEdited:   "lightgray"


    Rectangle{
        id: pathRect
        width: parent.width
        height: headerButtonSize
        anchors {
            top: parent.top
            left: parent.left
        }
        color: "white"
        border.color: "black"

        Row{
            width: parent.width
            spacing: headerSpacing
            anchors {
                left: parent.left;
                leftMargin: headerSpacing;
                verticalCenter: parent.verticalCenter
            }

            ImageButton {
                id:   previousPathButton
                size: headerButtonSize
                source: "icons/go-previous.png";
                onClicked: {
                    print("[ServerFiles] previousPathButton! TODO...")
                }
            }
            ImageButton {
                id:   nextPathButton
                size: headerButtonSize
                source: "icons/go-next.png";
                onClicked: {
                    print("[ServerFiles] nextPathButton! TODO...")
                }
            }
            ImageButton {
                id:   parentPathButton
                size: headerButtonSize
                source: "icons/go-up.png";
                onClicked: {
                    print("[ServerFiles] parentPathButton!")
                    cppRemote.getServerFiles(relativePath.text, "..");
                }
            }
            ImageButton {
                id:   homePathButton
                size: headerButtonSize
                source: "icons/go-home.png";
                onClicked: {
                    print("[ServerFiles] homePathButton!")
                    cppRemote.getServerFiles("./", "");
                }
            }

            Text {
                id: relativePath
                width: parent.width - 4*(headerButtonSize + headerSpacing) - 2*headerSpacing
                text: cppRemote.remoteFilesPath_QML()
                anchors.verticalCenter: parent.verticalCenter
//                font.pointSize: 20;
                elide: Text.ElideLeft
            }

        }
    }


    ListView {
        id: filesView

        model: RemoteFileModel {
            remote: cppRemote
        }
        delegate: remoteFileDelegate

        anchors{
            top: pathRect.bottom
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height - pathRect.height - actionRect.height

        clip: true
    }


    Rectangle{
        id: actionRect
        width: parent.width
        height:50
        anchors {
            bottom: parent.bottom
            left: parent.left
        }
        color: "white"
        border.color: "black"

        Row{
            width: parent.width
            anchors {
//                leftMargin: 20;
                verticalCenter: parent.verticalCenter
            }

        }
    }

    Connections{
        target: cppRemote
        function onUpdateRemoteFilesPath(newRemotePath){
            relativePath.text = newRemotePath;
        }
    }

    Component {
        id: remoteFileDelegate

        Rectangle {
            id: rootDelegate
            property bool isSelected: ListView.isCurrentItem

            width: ListView.view.width
            height: lineHeigth

            color: {
                if (ListView.isCurrentItem)
                    return colorSongSelected;
                else
                    return editMode ? colorSongEdited : "white";
            }

            Image {
                id: icon
                x: headerSpacing
                width: lineHeigth
                height: lineHeigth
                source: isDir ? "icons/folder.png" : "icons/music.png"
            }

            Text{
                id: txtFilename
                text: filename
                x: icon.width + 2*headerSpacing

                elide: Text.ElideRight
                width: parent.width - icon.width - 3*headerSpacing
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    print("Clicked #" + index + ": " + filename)
                    filesView.currentIndex = index
                }

                onDoubleClicked: {
                    print("onDoubleClicked #" + index + ": " + filename )
                    filesView.currentIndex = index
                    if (isDir)
                        cppRemote.getServerFiles(relativePath.text, filename);
                    else
                        print("TODO: double click on Remote File!");
                }

                onPressAndHold: {
                    filesView.currentIndex = index
                    editMode = true
                    print("onPressAndHold #" + index + ": " + filename)
                }
            }
        }
    }


}
