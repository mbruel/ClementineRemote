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
import QtQuick.Controls 2.15 as QQC
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls.Styles 1.4 as QQCS1
import Library 1.0
import QtQml.Models 2.15 // needed on iOS for TreeView... (otherwise for SelectionModel)

Rectangle {
    id: library
    radius: 10

    property int   lineSpacing    : 10
    property int   lineHeigth     : 45
    property color colorSelected  : "lightblue"

    property int   headerButtonSize    : 30

    property int   headerHeight      : 50;
    property int   headerSpacing     : 10;
    property int   headerPadding     : 5;
    property color headerColor       : "white"
    property color headerBorderColor : "black"
    property int   headerBorderWidth : 1

    property int rowHeight: 30
    property int indent: 20

    function loadLibrary() {
        if (cppRemote.isLibraryLoaded()) {
            libView.visible = true;
            loadingIndicator.visible = false;
        }
        else
        {
            libView.visible = false;
            loadingIndicator.visible = true;
            if (cppRemote.delayLibraryLoading())
                requestLibraryTimer.start(); // delay the loading to make sure the view is visible
        }
    }

    Component.onCompleted: loadLibrary();

    Connections{
        target: cppRemote
        function onLibraryLoaded(){
            loadLibrary();
        }
    }

    Timer {
        id: requestLibraryTimer
        interval: 100
        running: false
        repeat: false
        onTriggered: cppRemote.requestLibrary();
    }


    Rectangle{
        id     : headerRow
        width  : parent.width
        height : headerHeight
        anchors {
            top : parent.top
            left: parent.left
        }
        color       : headerColor
        border.color: headerBorderColor
        border.width: headerBorderWidth

        SearchField {
            id: searchField
            width: parent.width - headerButtonSize - 2*headerSpacing
            anchors {
                left: parent.left
                leftMargin: 5
                verticalCenter: parent.verticalCenter
            }
            onTextChanged: cppRemote.setLibraryFilter(text);
        }// searchField

        ImageButton {
            id:   refreshLibButton
            size: headerButtonSize
            anchors {
                right: parent.right
                rightMargin: headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/refresh.png";
            onClicked: {
                libView.visible = false;
                loadingIndicator.visible = true;
                cppRemote.getLibrary();
            }
        } // refreshLibButton
    } // headerRow


    QQC1.TreeView {
        id: libView
        focus: true
        clip: true

        anchors{
            top: headerRow.bottom
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height - headerRow.height - actionRect.height

        headerVisible: false;


//        selectionMode: QQC1.SelectionMode.MultiSelection
//        selection: ItemSelectionModel {
//            id: ism
//            model: cppRemote.libraryModel()
//        }

        model: cppRemote.libraryModel();

        onDoubleClicked: {
            let expandableIndexes = cppRemote.getExpandableIndexes(index);
            let currentExpanded = isExpanded(index);
            for(let i = 0; i < expandableIndexes.length ; ++i)
                 currentExpanded ? collapse(expandableIndexes[i]) : expand(expandableIndexes[i]);

// TODO: shall we add a Setting to send tracks to playlist on double click?
//            cppRemote.appendLibraryItem(index);
        } // onDoubleClicked

        style: QQCS1.TreeViewStyle {
            rowDelegate: Rectangle {
                height: rowHeight
                color:  styleData.selected ? colorSelected : "white"
            }
            branchDelegate: Image{
                width:  indent
                height: indent
                source: styleData.isExpanded ? "icons/collapse.png" : "icons/expand.png"
            }
//            branchDelegate: Rectangle {
//                width:  library.indent
//                height: library.indent
//                color: styleData.isExpanded ? "green" : "red"
//            }
            indentation: indent
        } // TreeViewStyle

        QQC1.TableViewColumn {
            id: column
//            title: qsTr("Artists")
            role: "name"
            delegate: Rectangle {
                height: rowHeight
//                implicitHeight: height
//                implicitWidth: width
                color: styleData.selected ? colorSelected : "white"
                Image {
                    id: itemIcon
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    height: 20
                    width: 20
                    source: cppRemote.libraryItemIcon(styleData.index);
//                    visible: cppRemote.isLibraryItemTrack(styleData.index)
                } // itemIcon
                Text{
//                    width: parent.width - (itemIcon.visible ? itemIcon.width : 0)
//                    anchors.left: itemIcon.visible ? itemIcon.right : parent.left
//                    anchors.leftMargin: itemIcon.visible ? 5 : 2
                    width: parent.width - itemIcon.width
                    anchors.left: itemIcon.right
                    anchors.leftMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    text: styleData.value
                    elide: Text.ElideRight
                }
            }
        } // column (TableViewColumn)
    } // libView


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
            id:   downButton
            size: headerButtonSize
            anchors {
                right: appendButton.left
                rightMargin: 3*headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/nav_downloads.png";
            onClicked: cppRemote.downloadLibraryItem(libView.currentIndex);
        } //  downButton
        ImageButton {
            id:   appendButton
            size: headerButtonSize
            anchors {
                right: newPlaylistNameField.left
                rightMargin: headerSpacing
                verticalCenter: parent.verticalCenter
            }
            source: "icons/addToPlayList.png";
            onClicked: cppRemote.appendLibraryItem(libView.currentIndex, "");
        } // appendButton
        QQC.TextField {
            id: newPlaylistNameField
            placeholderText: qsTr("new playlist")
            horizontalAlignment: TextInput.AlignHCenter
            height: parent.height - 4
//            width: newPlaylistNameWidth
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
                rightMargin: 5
                verticalCenter: parent.verticalCenter
            }
            source: "icons/newDoc.png";
            onClicked: {
                if (!mainApp.checkClementineVersion())
                    return
                if (newPlaylistNameField.text === "")
                    mainApp.error(qsTr("No playlist name"),
                                 qsTr("Please enter a name for the new playlist if you wish to create one..."));
                else {
                    cppRemote.appendLibraryItem(libView.currentIndex, newPlaylistNameField.text);
                    mainApp.info(qsTr("New Playlist created"),
                                 qsTr("The new playlist '%1' has been created<br/>It is now the current playlist.").arg(
                                     newPlaylistNameField.text));
                    newPlaylistNameField.text = "";
                }
            } // onClicked
        } // newPlaylistButton
    } // actionRect

    QQC.BusyIndicator {
        id: loadingIndicator
        running: true
        anchors.centerIn: libView
    }
}
