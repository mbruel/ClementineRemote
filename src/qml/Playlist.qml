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
import QtQuick.Layouts 1.3

import PlayList 1.0

Rectangle {
    //    property alias lbl: lbl
    //    property int iconSize: 30

    id: root
    radius: 10

    property bool  editMode: false

    property int playlistIdx: 0
    property int activePlaylistIdx: 0

    property int headerButtonSize   : 30

    property int playingSongIdx: -1

    property color colorSongPlaying:  "#ff8c00"
    property color colorSongSelected: "lightblue"
    property color colorSongEdited:   "lightgray"


    Component {
        id: songDelegate

        Rectangle {
            id: rootDelegate
            property bool isSelected: ListView.isCurrentItem

            width: ListView.view.width
            height: 50

            color: {
                if (/*searchField.text === ""
                        &&*/ activePlaylistIdx === playlistIdx
                        && index === playingSongIdx)
                    return colorSongPlaying;
                else
                {
                    if (ListView.isCurrentItem)
                        return colorSongSelected;
                    else
                        return editMode ? colorSongEdited : "white";
                }
            }

            onIsSelectedChanged: {
                if (isSelected)
                {
                    if (txtTitle.truncated)
                    {
                        txtTitle.elide = Text.ElideNone;
                        titleAnimation.start();
                    }
                    if (txtArtistAlbum.truncated)
                    {
                        txtArtistAlbum.elide = Text.ElideNone;
                        artistAnimation.start();
                    }
                }
                else
                {
                    if (titleAnimation.running)
                    {
                        titleAnimation.stop();
                        txtTitle.elide = Text.ElideRight;
                        txtTitle.x = 0;
                    }

                    if (artistAnimation.running)
                    {
                        artistAnimation.stop();
                        txtArtistAlbum.elide = Text.ElideRight;
                        txtArtistAlbum.x = 0;
                    }
                }
            }

            Text{
                id: txtTitle
                text: (track !== -1 ? String(track).padStart(2, '0')+" - " : "") + title
//                anchors.left: parent.left
                elide: Text.ElideRight
                width: parent.width - txtLength.width

                NumberAnimation {
                    id: titleAnimation
                    target: txtTitle
                    property: "x"
                    from: 20
                    to: -rootDelegate.width
                    duration: 3000
                    loops: Animation.Infinite
                }

                onTruncatedChanged: {
                    print("onTruncatedChanged for: "+txtTitle.text)
                    if (truncated && rootDelegate.isSelected)
                    {
                        txtTitle.elide = Text.ElideNone;
                        titleAnimation.start();
                    }
                }
            }
            Text{
                id: txtLength
                text: pretty_length
                horizontalAlignment: Text.AlignRight
                anchors.right: parent.right
            }
            Text{
                id: txtArtistAlbum
                width: parent.width
                elide: Text.ElideRight
                text: {
                    if (artist !== "" || album !== "")
                        return artist + " / " + album;
                    else
                        return "";
                }
                color: "blue"
                anchors{
                    top: txtTitle.bottom
                    //                    topMargin: 2
//                    left: txtTitle.left
                }
                x: 0

                NumberAnimation {
                    id: artistAnimation
                    target: txtArtistAlbum
                    property: "x"
                    from: 20
                    to: -rootDelegate.width
                    duration: 3000
                    loops: Animation.Infinite
                }

                onTruncatedChanged: {
                    print("onTruncatedChanged for: "+txtArtistAlbum.text)
                    if (truncated && rootDelegate.isSelected)
                    {
                        txtArtistAlbum.elide = Text.ElideNone;
                        artistAnimation.start();
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    print("Clicked #" + index + ": " + title)
                    songsView.currentIndex = index
                }

                onDoubleClicked: {
                    print("onDoubleClicked #" + index + ": " + title )
                    songsView.currentIndex = index
                    cppRemote.changeToSong(index)
                }

                onPressAndHold: {
                    songsView.currentIndex = index
                    editMode = true
                    print("onPressAndHold #" + index + ": " + title)
                }
            }
        }
    }


    Dialog {
        id: todoDialog

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        title: "TODO"

        Label {
            text: "this feature is not implemented yet..."
        }
    }

    Rectangle{
        id: playlistsRow
        width: parent.width
        height:50
        anchors {
            top: parent.top
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
            spacing: 10
            leftPadding: 5
            ComboBox {
                id: playlistCombo
                model : cppRemote.playlistsList()

                onActivated:{
                    if (index != playlistIdx)
                    {
                        print("Playlist clicked: #" + index+ " : " +currentText )
                        cppRemote.changePlaylist(index)
                    }
                }
            }

            TextField {
                id: searchField
                width: playlistsRow.width - playlistCombo.width - moreOptions.width - 3*parent.spacing
//                Layout.fillWidth: true
                placeholderText: qsTr("search")


                inputMethodHints: Qt.ImhNoPredictiveText;


                onTextChanged: {
                    print("searchField text changed: " + text);
                    cppRemote.setSongsFilter(text);
                    playingSongIdx = cppRemote.activeSongIndex();
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

//                    text: "x"
//                    verticalAlignment: Text.AlignVCenter

                    width: 30
                    height: parent.height - 4
                    anchors {
                        right: parent.right
                        rightMargin: 2
                        verticalCenter: parent.verticalCenter
                    }

                    onClicked: searchField.text = ""
                }
            }

            ImageButton {
                id: moreOptions
                size: headerButtonSize
                source: "icons/moreOptions.png";
                onClicked: {
                    print("playlist moreOptions clicked!")
                     playlistMenu.open();
                }
                anchors.verticalCenter: parent.verticalCenter

                Menu {
                    id: playlistMenu
                    x: parent.width - width
                    transformOrigin: Menu.TopLeft

                    Action {
                        text: qsTr("Refresh Playlist")
                        onTriggered: {
                            print("Refresh playlist");
                            todoDialog.open()
                        }
                    }
                    Action {
                        text: qsTr("Clear Playlist")
                        onTriggered: {
                            print("Clear playlist");
                            todoDialog.open()
                        }
                    }
                    Action {
                        text: qsTr("Close Playlist")
                        onTriggered: {
                            print("Close playlist");
                            todoDialog.open()
                        }
                    }
                    Action {
                        text: qsTr("Download Playlist")
                        onTriggered: {
                            print("Download playlist");
                            todoDialog.open()
                        }
                    }
                    Action {
                        text: qsTr("Create new Playlist")
                        onTriggered: {
                            print("Create playlist");
                            todoDialog.open()
                        }
                    }
                }

            }
        }

    }

    ListView {
        id: songsView

        anchors{
            top: playlistsRow.bottom
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height - playlistsRow.height

        //        implicitWidth: 250
        //        implicitHeight: 250
        //                    anchors.fill: parent

        clip: true

        model: cppRemote.playListModel()
//        model: PlayListModel {
//            remote: cppRemote
//        }


        delegate: songDelegate

        //        onCurrentItemChanged: console.log(model.get(songsView.currentIndex).name + ' selected')
        onCurrentItemChanged: console.log("songsView current index: "+songsView.currentIndex)


    }

    Component.onCompleted : {
        updateCurrentPlaylist(cppRemote.playlistIndex());

        print("songsView.size: "+songsView.count);
        let songIdx = cppRemote.currentSongIndex();
        updateCurrentSong(songIdx);
        if (songIdx)
            songsView.positionViewAtIndex(songsView.currentIndex, ListView.Center);

    }

    function updateCurrentSong(idx)
    {
        print("updateCurrentSong: " + idx);
        songsView.currentIndex = idx;
        if (cppRemote.isPlaying())
        {
            playingSongIdx    = songsView.currentIndex;
            activePlaylistIdx = cppRemote.playlistIndex();
            print("activePlaylistIdx: "+activePlaylistIdx)
        }

//        if (idx)
//            songsView.positionViewAtIndex(idx, ListView.Center)

    }

    function updateCurrentPlaylist(idx){
        print("updateCurrentPlaylist: "+idx);
        playlistIdx = idx;
        if (playlistCombo.currentIndex !== idx)
            playlistCombo.currentIndex = idx;

    }

    Connections{
        target: cppRemote
        function onCurrentSongIdx(idx){
            updateCurrentSong(idx);
        }

        function onUpdatePlaylist(idx)
        {
            updateCurrentPlaylist(idx);
        }
    }

    Button {
        text: qsTr("Edit Mode")
        onClicked: {
            print("change delegate")
            editMode = !editMode;
        }

        Layout.fillWidth: true

        anchors{
            bottom: parent.bottom
            right: parent.right
        }
    }
}



