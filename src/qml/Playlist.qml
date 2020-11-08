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
    id: root
    radius: 10


    property int   buttonSize        : 30

    property int   headerHeight      : 50;
    property int   headerSpacing     : 10;
    property int   headerPadding     : 5;
    property color headerColor       : "white"
    property color headerBorderColor : "black"
    property int   headerBorderWidth : 1

    property int   comboLineHeight   : 40;
    property int   comboIconSize     : 30
    property int   comboLineSpacing  : 5;

    property color colorSongPlaying  : "#ff8c00"
    property color colorSongSelected : "lightblue"


    // private properties
    property bool selectionMode    : false

    property int  playingSongIdx   : -1
    property int  playlistIdx      : 0
    property int  activePlaylistIdx: 0


    Connections {
        target: cppRemote
        function onCurrentSongIdx(idx) { updateCurrentSong(idx); }
        function onUpdatePlaylist(idx) { updateCurrentPlaylist(idx); }
        function onClosedPlaylistsReceived(nbClosedPlaylists){
//            print("nb Closed Playlists: "+nbClosedPlaylists);
            if (nbClosedPlaylists === 0)
                noMorePlaylistDialog.open();
            else
                openPlaylistDialog.open();
        }
    } // Connections cppRemote


    Component.onCompleted : {
        updateCurrentPlaylist(cppRemote.playlistIndex());

//        print("songsView.size: "+songsView.count);
        let songIdx = cppRemote.currentSongIndex();
        updateCurrentSong(songIdx);
        if (songIdx)
            songsView.positionViewAtIndex(songsView.currentIndex, ListView.Center);
    } // Component.onCompleted


    function updateCurrentSong(idx)
    {
//        print("updateCurrentSong: " + idx);
        songsView.currentIndex = idx;
        if (cppRemote.isPlaying())
        {
            playingSongIdx    = songsView.currentIndex;
            activePlaylistIdx = cppRemote.playlistIndex();
            print("activePlaylistIdx: "+activePlaylistIdx);
        }
//        if (idx)
//            songsView.positionViewAtIndex(idx, ListView.Center)
    }

    function updateCurrentPlaylist(idx){
//        print("updateCurrentPlaylist: "+idx);
        playlistIdx = idx;
        if (playlistCombo.currentIndex !== idx)
            playlistCombo.currentIndex = idx;
    }


    Rectangle{
        id     : playlistsRow
        width  : parent.width
        height : headerHeight
        anchors {
            top : parent.top
            left: parent.left
        }
        color       : headerColor
        border.color: headerBorderColor
        border.width: headerBorderWidth

        Row {
            width: parent.width
            anchors {
//                leftMargin: 20;
                verticalCenter: parent.verticalCenter
            }
            spacing    : headerSpacing
            leftPadding: headerPadding
            ComboBox {
                id: playlistCombo
//                model : cppRemote.playlistsList()

                model   : cppRemote.modelOpenedPlaylists()
                delegate: playlistDelegate
                textRole: "name"
            }

            TextField {
                id: searchField
                width: playlistsRow.width - playlistCombo.width - moreOptions.width - 3*parent.spacing
//                Layout.fillWidth: true
                placeholderText: qsTr("search")

                inputMethodHints: Qt.ImhNoPredictiveText;

                onTextChanged: {
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

            ImageButton {
                id: moreOptions
                size  : buttonSize
                source: "icons/moreOptions.png";                
                anchors.verticalCenter: parent.verticalCenter

                onClicked: playlistMenu.open();
            } // moreOptions
        } // Row
    } // playlistsRow


    ListView {
        id: songsView

        anchors{
            top: playlistsRow.bottom
            left: parent.left
        }
        implicitWidth: parent.width
        implicitHeight: parent.height - playlistsRow.height
        clip: true

        model: cppRemote.modelRemoteSongs()
        delegate: songDelegate

        flickableDirection: Flickable.VerticalFlick
        boundsBehavior    : Flickable.DragOverBounds
        boundsMovement    : Flickable.FollowBoundsBehavior
        ScrollBar.vertical: ScrollBar {}

//        onCurrentItemChanged: console.log("songsView current index: "+songsView.currentIndex)
    } // songsView


    ImageButton {
        id:   exitSelectModeButton
        size: buttonSize
        anchors {
            bottom: parent.bottom
            right:  selectAllButton.left
            rightMargin: headerSpacing
        }
        source: "icons/close.png";
        onClicked: {
            cppRemote.selectAllSongsFromProxyModel(false);
            selectionMode = false;
        }
        visible: selectionMode
    }
    ImageButton {
        id:   selectAllButton
        size: buttonSize
        anchors {
            bottom: parent.bottom
            right:  deleteSongsButton.left
            rightMargin: headerSpacing
        }
        source: "icons/select_all.png";
        onClicked: {
            let allSelected = cppRemote.allSongsSelected();
            cppRemote.selectAllSongsFromProxyModel(!allSelected);
            if (allSelected)
                songsView.currentIndex = -1;
        }
        visible: selectionMode
    }
    ImageButton {
        id: deleteSongsButton
        size  : buttonSize
        source: "icons/delete.png";
        anchors{
            bottom: parent.bottom
            right:  parent.right
            rightMargin: headerSpacing
        }
        onClicked: {
            cppRemote.deleteSelectedSongs();
            selectionMode = false;
        }
        visible: selectionMode
    } // deleteSongsButton



    Dialog {
        id: todoDialog

        width: root.width *3/4

        x: (root.width - width) / 2
        y: (root.height - height) / 2

        title: qsTr("TODO")

        Label {
            width: parent.width - 5
            text: qsTr("this feature is not implemented yet...")
            wrapMode: Text.WordWrap
        }
    } // todoDialog

    Dialog {
        id: noMorePlaylistDialog

        width: root.width *3/4

        x: (root.width - width) / 2
        y: (root.height - height) / 2

        title: qsTr("No more Playlists")

        Label {
            width: parent.width - 5
            text: qsTr("All available Playlists are already opened")
            wrapMode: Text.WordWrap
        }
    } // noMorePlaylistDialog


    Dialog {
        id: playlistDestructionConfirmationDialog

        width: root.width *3/4

        x: (root.width - width) / 2
        y: (root.height - height) / 2

        title: qsTr("Delete Playlist")

        standardButtons: Dialog.Yes | Dialog.No
        onAccepted:  cppRemote.closePlaylist(cppRemote.currentPlaylistID());

        Label {
            width: parent.width - 5
            text: qsTr("You are about to remove a playlist which is not part of your favourite playlists: \
the playlist will be deleted (this action cannot be undone).<br/>
Are you sure you want to continue?")
            wrapMode: Text.WordWrap
        }
    } // playlistDestructionConfirmationDialog

    Dialog {
        id: openPlaylistDialog

        property bool createNewPlaylist: false

        width: root.width *3/4

        x: (root.width - width) / 2
        y: (root.height - height) / 2
        parent: Overlay.overlay

        focus: true
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: cppRemote.openPlaylist(closedPlaylistCombo.currentValue);
        onOpened:   closedPlaylistCombo.currentIndex = 0;

        ColumnLayout {
            spacing: 20
            anchors.fill: parent
            Label {
                elide: Label.ElideMiddle
                text: qsTr("Please select a Playlist to open")
                Layout.fillWidth: true
            }
            ComboBox {
                id: closedPlaylistCombo
                model   : cppRemote.modelClosedPlaylists()
                delegate: ItemDelegate{
                    text: name ;
                    width: ListView.view.width;
                    highlighted: ListView.isCurrentItem
                }
                textRole: "name"
                valueRole: "id"
            }
        }
    } // openPlaylistDialog

    Dialog {
        id: renPlaylistDialog

        property bool createNewPlaylist: false

        width: root.width *3/4

        x: (root.width - width) / 2
        y: (root.height - height) / 2
        parent: Overlay.overlay

        focus: true
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            if (newNameField.text.length > 0)
            {
                if (createNewPlaylist)
                    cppRemote.createPlaylist(newNameField.text);
                else
                    cppRemote.renamePlaylist(cppRemote.currentPlaylistID(), newNameField.text);
            }
        }

        ColumnLayout {
            spacing: 20
            anchors.fill: parent
            Label {
                elide: Label.ElideMiddle
                text: qsTr("Please provide a name for the Playlist")
                Layout.fillWidth: true
            }
            TextField {
                id: newNameField
                focus: true
                placeholderText: "playlist name"
                Layout.fillWidth: true
            }
        }
    } // renPlaylistDialog



    // Delegates
    Component {
        id: playlistDelegate

        ItemDelegate {
            width: ListView.view.width
            height: comboLineHeight

            Image {
                id: icon
                x : comboLineSpacing
                width : comboIconSize
                height: comboIconSize
                source: iconSrc

                anchors.verticalCenter: parent.verticalCenter
            }

            Text{
                id: txtName
                text: name
                x: comboLineHeight + 2*comboLineSpacing

                anchors.verticalCenter: icon.verticalCenter

                elide: Text.ElideRight
                width: parent.width - icon.width - 3*comboLineSpacing
            }

            highlighted: ListView.isCurrentItem
            onClicked: {
                playlistCombo.popup.close()
                playlistCombo.currentIndex = index;
                if (index != playlistIdx)
                {
                    print("Playlist clicked: #" + index+ " : " +name )
                    cppRemote.changePlaylist(index)
                }
            }
        }
/*
        Rectangle {
            width: ListView.view.width
            height: comboLineHeight

//            color: ListView.isCurrentItem ? colorSongSelected : "white"

            Image {
                id: icon
                x : combolineSpacing
                width : comboIconSize
                height: comboIconSize
                source: iconSrc

                anchors.verticalCenter: parent.verticalCenter
            }

            Text{
                id: txtName
                text: name
                x: comboLineHeight + 2*combolineSpacing

                anchors.verticalCenter: icon.verticalCenter

                elide: Text.ElideRight
                width: parent.width - icon.width - 3*combolineSpacing
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled : true
                onClicked: {
                    playlistCombo.popup.close()
                    playlistCombo.currentIndex = index;
                    if (index != playlistIdx)
                    {
                        print("Playlist clicked: #" + index+ " : " +name )
                        cppRemote.changePlaylist(index)
                    }
                }

                onEntered: color = "lightgray"
                onExited: color = "white"
            }
        }
*/
    } // Component playlistDelegate

    Component {
        id: songDelegate

        Rectangle {
            id: rootDelegate
            property bool isSelected: ListView.isCurrentItem

            width: ListView.view.width
            height: 50

            color: {
                if (selectionMode){
                    if (selected)
                        return colorSongSelected;
                }
                else {
                    if (activePlaylistIdx === playlistIdx && index === playingSongIdx)
                        return colorSongPlaying
                    else if (ListView.isCurrentItem)
                        return colorSongSelected;
                }
                return "white";
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
                    if (selectionMode)
                        selected = !selected;
                }

                onDoubleClicked: {
                    print("onDoubleClicked #" + index + ": " + title )
                    songsView.currentIndex = index
                    cppRemote.changeToSong(index)
                }

                onPressAndHold: {
                    songsView.currentIndex = index
                    selectionMode = true
                    selected = true;
                    print("onPressAndHold #" + index + ": " + title)
                }
            }
        }
    } // Component songDelegate



    Menu {
        id: playlistMenu
        x: parent.width - width
        transformOrigin: Menu.TopLeft

        Action {
            icon.source: "icons/refresh.png"
            text: qsTr("Refresh Playlist")
            onTriggered: cppRemote.changePlaylist(playlistCombo.currentIndex);
        }
        Action {
            icon.source: "icons/star.png"
            text: qsTr("Save Playlist")
            onTriggered: cppRemote.savePlaylist(cppRemote.currentPlaylistID());
        }
        Action {
            icon.source: "icons/renamePlaylist.png"
            text: qsTr("Rename Playlist")
            onTriggered: {
                renPlaylistDialog.title = qsTr("Rename Playlist ") + playlistCombo.currentText;
                renPlaylistDialog.createNewPlaylist = false;
                renPlaylistDialog.open();
            }
        }
        Action {
            icon.source: "icons/clear.png"
            text: qsTr("Clear Playlist")
            onTriggered: cppRemote.clearPlaylist(cppRemote.currentPlaylistID());
        }
        Action {
            icon.source: "icons/close.png"
            text: qsTr("Close Playlist")
            onTriggered: {
                // Won't allow removing the last playlist
                if (playlistCombo.count <= 1)
                    return;
                if (!cppRemote.isCurrentPlaylistSaved())
                    playlistDestructionConfirmationDialog.open();
                else
                    cppRemote.closePlaylist(cppRemote.currentPlaylistID());
            }
        }
        Action {
            icon.source: "icons/nav_downloads.png"
            text: qsTr("Download Playlist")
            onTriggered: {
                print("Download playlist");
                todoDialog.open()
            }
        }
        MenuSeparator {}
        Action {
            icon.source: "icons/open.png"
            text: qsTr("Open a Playlist")
            onTriggered: cppRemote.getAllPlaylists();
        }
        Action {
            icon.source: "icons/newFile.png"
            text: qsTr("Create new Playlist")
            onTriggered: {
                renPlaylistDialog.title = qsTr("Create New Playlist ");
                renPlaylistDialog.createNewPlaylist = true;
                renPlaylistDialog.open();
            }
        }
    } // playlistMenu
}
