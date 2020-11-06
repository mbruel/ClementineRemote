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

import RemoteFile 1.0

Rectangle {
    id: root
    radius: 10

    property int   newPlaylistNameWidth: 100
    property int   headerButtonSize    : 30
    property int   headerSpacing       : 5
    property int   lineHeigth          : 25
    property color colorSelected       : "lightblue"

    // private properties
    property bool  selectionMode       : false


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
                    resetSelectionMode();
                    print("[ServerFiles] parentPathButton!")
                    cppRemote.getServerFiles(relativePath.text, "..");
                }
            }
            ImageButton {
                id:   homePathButton
                size: headerButtonSize
                source: "icons/go-home.png";
                onClicked: {
                    resetSelectionMode();
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
        focus: true

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

        flickableDirection: Flickable.VerticalFlick
        boundsBehavior    : Flickable.DragOverBounds
        boundsMovement    : Flickable.FollowBoundsBehavior
        ScrollBar.vertical: ScrollBar {}

        function selectAllFiles(selectAll)
        {
            model.selectAllFiles(selectAll);
            if (!selectAll)
                filesView.currentIndex = -1;
        }

        function selectCurrentFile(select){
            model.select(filesView.currentIndex, select);
        }
    }




    Rectangle{
        id: actionRect
        width: parent.width
        height:headerButtonSize
        anchors {
            bottom: parent.bottom
            left: parent.left
        }
        color: "white"
        border.color: "black"



        ImageButton {
            id:   selectModeButton
            size: headerButtonSize
            anchors {
                left: parent.left
                leftMargin: headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/" + (selectionMode ? "selection.png" : "click.png");
            onClicked: {
                selectionMode = !selectionMode
                if (selectionMode)
                    filesView.selectCurrentFile(true);
                else
                    filesView.selectAllFiles(false);
//                print("[ServerFiles] change selection mode: " + selectionMode)
            }
        }

        ImageButton {
            id:   selectAllButton
            size: headerButtonSize
            anchors {
                left: selectModeButton.right
                leftMargin: headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/select_all.png";
            onClicked: {
                selectionMode = !cppRemote.allFilesSelected();
                filesView.selectAllFiles(selectionMode);
//                print("[ServerFiles] selectAll: " + selectionMode);
            }
        }


        ImageButton {
            id:   appendButton
            size: headerButtonSize
            anchors {
                right: newPlaylistNameField.left
                rightMargin: 2*headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/addToPlayList.png";
            onClicked: sendSelectedFiles("");
        }


        TextField {
            id: newPlaylistNameField
            placeholderText: qsTr("new playlist")
            horizontalAlignment: TextInput.AlignHCenter
            height: parent.height - 4
            width: newPlaylistNameWidth

            anchors {
                right: newPlaylistButton.left
                rightMargin: headerSpacing / 2
                verticalCenter: parent.verticalCenter
            }

            color: "black"
            background: Rectangle { radius: 8 ; border.width: 1; border.color: colorSelected }
        }


        ImageButton {
            id:   newPlaylistButton
            size: headerButtonSize
            anchors {
                right: parent.right
                rightMargin: headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/newDoc.png";
            onClicked: {
                if (newPlaylistNameField.text === "")
                {
                    infoDialog.title = "No playlist name";
                    infoDialog.text  = "Please enter a name for the new playlist\
 if you wish to create one...";
                    infoDialog.open();
                }
                else
                    sendSelectedFiles(newPlaylistNameField.text);
            }
        }

    }

    Component.onCompleted: {
        if (cppRemote.hideServerFilesPreviousNextNavButtons())
        {
            previousPathButton.visible = false;
            nextPathButton.visible = false;
            relativePath.width = root.width - 2*(headerButtonSize + headerSpacing + headerSpacing)
        }
        filesView.currentIndex = -1; // no default selection
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
                if (selectionMode){
                    if (selected)
                        return colorSelected;
                }
                else {
                    if (ListView.isCurrentItem)
                        return colorSelected;
                }
                return "white";
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
                    filesView.currentIndex = index
                    if (selectionMode)
                        selected = !selected;
                }

                onDoubleClicked: {
//                    print("onDoubleClicked #" + index + ": " + filename )
                    filesView.currentIndex = index
                    if (isDir)
                    {
                        resetSelectionMode();
                        cppRemote.getServerFiles(relativePath.text, filename);
                    }
                    else
                    {
                        selected = true;
                        sendSelectedFiles("");
                        selected = false;
                    }
                }

                onPressAndHold: {
                    filesView.currentIndex = index;
                    selectionMode = true;
                    selected = true;
//                    print("onPressAndHold #" + index + ": " + filename)
                }
            }
        }
    }

    Dialog {
        id: infoDialog
        property alias text: infoLbl.text

        width: root.width * 2/3
        x: (root.width - width) / 2
        y: (root.height - height) / 2

        title: "TODO"

        Label {
            id: infoLbl
            width: parent.width
            text: "to be set..."
            wrapMode: Text.WordWrap
        }
    }

    function resetSelectionMode(){
        selectionMode = false;
        filesView.selectAllFiles(false);
    }

    function sendSelectedFiles(newPlaylist){
        let nbSelectedFiles = cppRemote.sendSelectedFiles(newPlaylist);
        if (nbSelectedFiles === 0)
        {
//            print("no selected files to send...");
            infoDialog.title = "No selected file";
            infoDialog.text  = "Please select at least one file...";
            infoDialog.open();
        }
//        else
//            print("sendSelectedFiles " + nbSelectedFiles);
        resetSelectionMode();
    }
}
