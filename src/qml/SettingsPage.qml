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
//import Qt.labs.platform 1.1
//import QtQuick.Dialogs 1.3

Page {
    title: qsTr('Settings')

    property int headerHeight: 50
    property int headerBottomMargin: 20
    property int sectionMargin: 10

    property int switchWidth: 51
    property int switchHeight: 26

    signal exitSettings

    background: Rectangle{color: "lightgray"}

    Rectangle {
        id: settingsHeader
        anchors {top: parent.top; left: parent.left}
        width: parent.width
        height: headerHeight
        color: "transparent"

        Text {
            text: title
            font.pointSize: 18;
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
        }

        ImageButton {
            id: backButton
            anchors{
                left: parent.left
                verticalCenter: parent.verticalCenter;
                leftMargin: 15
            }
            size     : parent.height - 10
            source   : "icons/backPage.png";
            onClicked: exitSettings();
        } // backButton
    }

    Flickable {
        id: flickable
        anchors {
            top: settingsHeader.bottom;
            left: parent.left
            topMargin: headerBottomMargin
        }
        height: parent.height - settingsHeader.height - anchors.topMargin
        width: parent.width
        contentHeight: column.height

        clip: true

        flickableDirection: Flickable.VerticalFlick
        boundsBehavior    : Flickable.FollowBoundsBehavior
        boundsMovement    : Flickable.FollowBoundsBehavior
        ScrollBar.vertical: ScrollBar {}

        Column {
            id: column
            spacing: 10
            width: parent.width

            Rectangle {
                id: playerSettings
                width: parent.width
                height: playerTitle.height + switchHeight + 4*sectionMargin
                color: "transparent"

                radius: 10
                border.width: 1

                Text {
                    id: playerTitle
                    anchors {
                        left: parent.left
                        top: parent.top
                        topMargin: sectionMargin
                        leftMargin: sectionMargin
                    }
                    text: qsTr("Player")
                    font {
                        pointSize: 15;
                        underline: true
                    }
                } // playerTitle

                Text {
                    id: vSliderLbl
                    anchors {
                        left: parent.left
                        verticalCenter: vSliderSwitch.verticalCenter
                        leftMargin: sectionMargin
                    }
                    text: qsTr("Vertical volume slider")
                } // vSliderLbl
                SettingSwitch{
                    id: vSliderSwitch
                    anchors {
                        right: parent.right
                        top: playerTitle.bottom
                        topMargin: sectionMargin
                        rightMargin: sectionMargin
                    }
                    width: switchWidth
                    sliderSize: switchHeight

                    checked: cppRemote.verticalVolumeSlider()
                    onToggled: cppRemote.setVerticalVolumeSlider(vSliderSwitch.checked);
                } // vSliderSwitch

            } // playerSettings

//            MenuSeparator{ width: parent.width;}

            Rectangle {
                id: downSettings
                width: parent.width
                height: downTitle.height + 2*switchHeight + 5*sectionMargin
                color: "transparent"

                radius: 10
                border.width: 1

                Text {
                    id: downTitle
                    anchors {
                        left: parent.left
                        top: parent.top
                        topMargin: sectionMargin
                        leftMargin: sectionMargin
                    }
                    text: qsTr("Download")
                    font {
                        pointSize: 15;
                        underline: true
                    }
                } // playerTitle


                Text {
                    id: downPathLbl
                    anchors {
                        left: parent.left
                        verticalCenter: downPathEdit.verticalCenter
                        leftMargin: sectionMargin
                    }
                    text: qsTr("Download directory:")
                } // downPathLbl
                TextField {
                    id: downPathEdit
//                    readOnly: true
                    anchors {
                        left: downPathLbl.right
                        top: downTitle.bottom
                        topMargin: sectionMargin
                        leftMargin: sectionMargin
                    }
                    width: parent.width - downPathLbl.width - 4*sectionMargin // - downPathButton.width
                    horizontalAlignment: TextInput.AlignHCenter
                    color: "black"
                    background: Rectangle { radius: 8 ; border.width: 0 }

                    text: cppRemote.downloadPath();
                } // downPathEdit
//                Button{
//                    id: downPathButton
//                    width: 30
//                    height: switchHeight
//                    anchors {
//                        left: downPathEdit.right
//                        verticalCenter: downPathEdit.verticalCenter
//                        leftMargin: sectionMargin
//                    }
//                    text: "..."
//                    onClicked: {
//                        folderDialog.folder = downPathEdit.text
//                        folderDialog.open();
//                    }
//                }


                Text {
                    id: overwriteFilesLbl
                    anchors {
                        left: parent.left
                        verticalCenter: overwriteFilesSwitch.verticalCenter
                        leftMargin: sectionMargin
                    }
                    text: qsTr("Overwrite existing Files (TODO)")
                } // overwriteFilesLbl
                SettingSwitch{
                    id: overwriteFilesSwitch
                    anchors {
                        right: parent.right
                        top: downPathLbl.bottom
                        topMargin: sectionMargin
                        rightMargin: sectionMargin
                    }
                    width: switchWidth
                    sliderSize: switchHeight

                    checked: false
                    onToggled: print("overwriteFilesSwitch: " + checked);
                } // overwriteFilesSwitch

            } // downSettings

//            Label {
//                width: parent.width
//                wrapMode: Label.Wrap
//                horizontalAlignment: Qt.AlignHCenter
//                text: "ScrollBar is an interactive bar that can be used to scroll to a specific position. "
//                      + "A scroll bar can be either vertical or horizontal, and can be attached to any Flickable, "
//                      + "such as ListView and GridView."
//            }
        }
    }

/*
    FileDialog {
        id: folderDialog
//        flags: FolderDialog.ShowDirsOnly
//        folder: 'file://'+downPathEdit.text

        selectFolder : true
        selectMultiple: false
        nameFilters: [ "Select a Folder (*)" ]

        onAccepted: {
//            print("Accepted folder: " + folderDialog.folder)
            print("nb selected: "+folderDialog.fileUrls.length)
            print("file selected: "+folderDialog.fileUrl)
            if (folderDialog.fileUrls.length !== 0)
            {
                let path = folderDialog.fileUrls[folderDialog.fileUrls.length-1].toString();//.replace(/^(file:\/{2})/,"");
                downPathEdit.text = decodeURIComponent(path);
                print("Selected url: "+folderDialog.fileUrls[0]);
            }
        }
    }
*/
}
