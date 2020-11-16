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
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls.Styles 1.4 as QQCS1
import Library 1.0
//import QtQml.Models 2.15 // for SelectionModel

Rectangle {
    id: library
    radius: 10

    property int   lineSpacing    : 10
    property int   lineHeigth     : 45
    property color colorSelected  : "lightblue"

    property int   headerHeight      : 50;
    property int   headerSpacing     : 10;
    property int   headerPadding     : 5;
    property color headerColor       : "white"
    property color headerBorderColor : "black"
    property int   headerBorderWidth : 1


    property int rowHeight: 30
    property int indent: 20

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

        TextField {
            id: searchField
            width: parent.width
//                Layout.fillWidth: true
            placeholderText: qsTr("search")

            inputMethodHints: Qt.ImhNoPredictiveText;

            onTextChanged: {
                cppRemote.setLibraryFilter(text);
//                playingSongIdx = cppRemote.activeSongIndex();
            }

            Button {
                id: clearSearch
                Text {
                    text: "x"
                    anchors{
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                }
                width: 30
                height: parent.height - 4
                anchors {
                    right: parent.right
                    rightMargin: 2
                    verticalCenter: parent.verticalCenter
                }
                onClicked: searchField.text = ""
            } //clearSearch
        } // searchField
    }

    QQC1.TreeView {
        id: filesView
        focus: true
        clip: true

        anchors{
            top: headerRow.bottom
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height - headerRow.height

        headerVisible: false;


//        selectionMode: QQC1.SelectionMode.MultiSelection
//        selection: ItemSelectionModel {
//            id: ism
//            model: cppRemote.libraryModel()
//        }

        model: cppRemote.libraryModel();

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
        }

        QQC1.TableViewColumn {
            id: column
//            title: qsTr("Artists")
            role: "name"
            delegate: Rectangle {
                height: library.rowHeight
//                implicitHeight: height
//                implicitWidth: width
                color: styleData.selected ? colorSelected : "white"
                Image {
                    id: disc
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    height: 20
                    width: 20
                    source: "icons/music.png"
                    visible: cppRemote.isLibraryItemTrack(styleData.index)
                }
                Text{
                    width: parent.width - (disc.visible ? disc.width : 0)
                    anchors.left: disc.visible ? disc.right : parent.left
                    anchors.leftMargin: disc.visible ? 5 : 2
                    anchors.verticalCenter: parent.verticalCenter
                    text: styleData.value
                    elide: Text.ElideRight
                }
            }
        }
    }
}
