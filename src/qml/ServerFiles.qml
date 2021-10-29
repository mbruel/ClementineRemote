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

import RemoteFile 1.0

Rectangle {
    id: serverFiles
    radius: 10

    property int   newPlaylistNameWidth: 100
    property int   headerButtonSize    : 30
    property int   headerSpacing       : 5
    property int   lineHeigth          : 25
    property color colorSelected       : 'lightblue'
    property color colorSelectMode     : 'cornflowerblue'

    // private properties
    property bool  selectionMode       : false

    Connections{
        target: cppRemote
        function onUpdateRemoteFilesPath(newRemotePath){
            relativePath.text = newRemotePath;
        }
    } // Connections cppRemote

    Component.onCompleted: {
        if (cppRemote.hideServerFilesPreviousNextNavButtons())
        {
            previousPathButton.visible = false;
            nextPathButton.visible = false;
            relativePath.width = serverFiles.width - 2*(headerButtonSize + headerSpacing + headerSpacing)
        }
        filesView.currentIndex = -1; // no default selection
    } // Component.onCompleted


    function resetSelectionMode(){
        selectionMode = false;
        filesView.selectAllFiles(false);
    } // resetSelectionMode

    function sendSelectedFiles(newPlaylist){
        cppRemote.sendSelectedFiles(newPlaylist);
        resetSelectionMode();
    } // sendSelectedFiles

    function downloadSelectedFiles(){
        if (!mainApp.downloadPossible())
            return;
        cppRemote.downloadSelectedFiles();
        resetSelectionMode();
    } // downloadSelectedFiles


    ////////////////////////////////////////
    //            QML Items               //
    ////////////////////////////////////////

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

        Row {
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
                onClicked: mainApp.todo();
            } // previousPathButton
            ImageButton {
                id:   nextPathButton
                size: headerButtonSize
                source: "icons/go-next.png";
                onClicked: mainApp.todo();
            } // nextPathButton
            ImageButton {
                id:   parentPathButton
                size: headerButtonSize
                source: "icons/go-up.png";
                onClicked: {
                    resetSelectionMode();
                    if (cppRemote.debugBuild())
                        print("[ServerFiles] parent folder clicked")
                    cppRemote.getServerFiles(relativePath.text, "..");
                }
            } // parentPathButton
            ImageButton {
                id:   homePathButton
                size: headerButtonSize
                source: "icons/go-home.png";
                onClicked: {
                    resetSelectionMode();
                    if (cppRemote.debugBuild())
                        print("[ServerFiles] home clicked!")
                    cppRemote.getServerFiles("./", "");
                }
            } // homePathButton
            ImageButton {
                id:   searchButton
                size: headerButtonSize
                source: "icons/search.png";
                onClicked: searchField.visible = !searchField.visible
            } // searchButton
            Text {
                id: relativePath
                width: parent.width - 3*(headerButtonSize + 2*headerSpacing) - 2*headerSpacing
                text: cppRemote.remoteFilesPath_QML()
                anchors.verticalCenter: parent.verticalCenter
//                font.pointSize: 20;
                elide: Text.ElideLeft
            } // relativePath
        } // Row
    } // pathRect

    SearchField {
        id: searchField
        visible: false
        width: parent.width - 2*headerSpacing
        height: headerButtonSize
        anchors {
            left: parent.left
            leftMargin: headerSpacing
            rightMargin: headerSpacing
            top: pathRect.bottom
        }
        background: Rectangle { radius: 8 ; border.width: 1; border.color: colorSelected }
        clearButton.background: Rectangle { radius: 8 ; color: "lightgrey" }

        onTextChanged: {
            filesView.model.setFilter(text);
            if (text === "")
                searchButton.colorDefault = 'transparent';
            else
                searchButton.colorDefault = "darkred";
            searchButton.color = searchButton.colorDefault;
        }
    } // searchField

    ListView {
        id: filesView
        focus: true

        anchors{
            top: searchField.visible ? searchField.bottom : pathRect.bottom
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height - (searchField.visible ? 2 : 1)*pathRect.height - actionRect.height

        clip: true

//        model: RemoteFileModel { remote: cppRemote }
        model: RemoteFileProxyModel { remote: cppRemote }
        delegate: remoteFileDelegate

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
    } // filesView

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
            source: 'icons/' + (selectionMode ? 'selection.png' : cppRemote.iconClick());
            onClicked: {
                selectionMode = !selectionMode
                if (selectionMode)
                    filesView.selectCurrentFile(true);
                else
                    filesView.selectAllFiles(false);
//                print("[ServerFiles] change selection mode: " + selectionMode)
            }
        } // selectModeButton
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
        } // selectAllButton

        ImageButton {
            id:   downSelectedFilesButton
            size: headerButtonSize
            anchors {
                right: appendButton.left
                rightMargin: 2*headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/nav_downloads.png";
            onClicked: downloadSelectedFiles();
        } //  downSelectedFilesButton

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
        } // appendButton
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
        } // newPlaylistNameField
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
                if (!mainApp.checkClementineVersion())
                    return
                if (newPlaylistNameField.text === "")
                    mainApp.error(qsTr("No playlist name"),
                                  qsTr("Please enter a name for the new playlist if you wish to create one..."));
                else
                {
                    sendSelectedFiles(newPlaylistNameField.text);
                    mainApp.info(qsTr("New Playlist created"),
                                 qsTr("The new playlist '%1' has been created<br/>It is now the current playlist.").arg(
                                     newPlaylistNameField.text));
                    newPlaylistNameField.text = "";
                }
            }
        } // newPlaylistButton
    } // actionRect


    ////////////////////////////////////////
    //      Components and Popups         //
    ////////////////////////////////////////

    Component {
        id: remoteFileDelegate

        Rectangle {
            id: remoteFileDelegateRect
            property bool isSelected: ListView.isCurrentItem

            width: ListView.view.width
            height: lineHeigth

            color: {
                if (selectionMode){
                    if (selected)
                        return colorSelectMode;
                }
                else {
                    if (ListView.isCurrentItem)
                        return colorSelected;
                }
                return 'transparent';
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
                    if (isDir)
                    {
                        if (selectionMode)
                            mainApp.error(qsTr("Selection Error"),qsTr("You can only select files..."))
                        else {
                            resetSelectionMode();
                            cppRemote.getServerFiles(relativePath.text, filename);
                            searchField.clear();
                        }
                    }
                    else if (selectionMode)
                        selected = !selected;
                }

                onDoubleClicked: {
//                    print("onDoubleClicked #" + index + ": " + filename )
                    filesView.currentIndex = index
                    if (!isDir)
                    {
                        selected = true;
                        sendSelectedFiles("");
                        selected = false;
                    }
                }

                onPressAndHold: {
                    if (isDir)
                        mainApp.error(qsTr("Selection Error"),qsTr("You can only select files..."))
                    else {
                        filesView.currentIndex = index;
                        selectionMode = true;
                        selected = true;
//                        print("onPressAndHold #" + index + ": " + filename)
                    }
                }
            }
        }
    }

}
