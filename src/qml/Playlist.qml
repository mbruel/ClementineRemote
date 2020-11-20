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
    id: playlist
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

    property int  activeSongIdx   : -1
    property int  playlistIdx      : 0
    property int  activePlaylistIdx: 0


    Connections {
        target: cppRemote
        function onActiveSongIdx(idx) { updateActiveSong(idx); }
        function onUpdatePlaylist(idx) { updateCurrentPlaylist(idx); }
        function onClosedPlaylistsReceived(nbClosedPlaylists){
//            print("nb Closed Playlists: "+nbClosedPlaylists);
            if (nbClosedPlaylists === 0)
                noMorePlaylistDialog.open();
            else {
                playlistChoiceDialog.text = qsTr("Please select a Playlist to open");
                playlistChoiceDialog.combo.model  = cppRemote.modelClosedPlaylists();
                playlistChoiceDialog.openPlaylist = true;
                playlistChoiceDialog.open();
            }
        } // onClosedPlaylistsReceived

        function onAskPlaylistDestID() {
            playlistChoiceDialog.text = qsTr("Please select a destination Playlist");
            playlistChoiceDialog.combo.model  = cppRemote.modelOpenedPlaylists();
            playlistChoiceDialog.openPlaylist = false;
            playlistChoiceDialog.open();
        } // onAskPlaylistDestID
    } // Connections cppRemote


    Component.onCompleted : goToCurrentPlaylistAndTrack();


    function goToCurrentPlaylistAndTrack() {
        if (playlistIdx !== cppRemote.getAtivePlaylistIndex())
            cppRemote.changePlaylist(cppRemote.getAtivePlaylistIndex());

        updateActiveSong(cppRemote.getActiveSongIndex());
        songsView.positionViewAtIndex(activeSongIdx, ListView.Center);
    } // goToCurrentPlaylistAndTrack

    function updateActiveSong(idx) {
        activeSongIdx = idx;
        activePlaylistIdx = cppRemote.getAtivePlaylistIndex();
        if (playlistIdx === activePlaylistIdx)
            songsView.currentIndex = idx;
    } // updateActiveSong

    function updateCurrentPlaylist(idx){
//        print("updateCurrentPlaylist: "+idx);
        songsView.currentIndex = -1; // don't select any song
        playlistIdx = idx;
        if (playlistCombo.currentIndex !== idx)
            playlistCombo.currentIndex = idx;
    } // updateCurrentPlaylist

    function downloadPlaylist() {
        if (mainApp.downloadPossible()) {
            cppRemote.setIsDownloading(true);
            cppRemote.downloadPlaylist(cppRemote.playlistID(), cppRemote.playlistName());
        }
    } // downloadCurrentSong



    ////////////////////////////////////////
    //            QML Items               //
    ////////////////////////////////////////

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
            } // playlistCombo
            TextField {
                id: searchField
                width: playlistsRow.width - playlistCombo.width - moreOptions.width - 3*parent.spacing
//                Layout.fillWidth: true
                placeholderText: qsTr("search")
                inputMethodHints: Qt.ImhNoPredictiveText;

                onTextChanged: {
                    cppRemote.setSongsFilter(text);                    
                    activeSongIdx = cppRemote.getActiveSongIndex();
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
            rightMargin: headerSpacing/2
        }
        source: 'icons/' + cppRemote.iconClick();
        onClicked: {
            cppRemote.selectAllSongsFromProxyModel(false);
            selectionMode = false;
        }
        visible: selectionMode
    } // exitSelectModeButton
    ImageButton {
        id:   selectAllButton
        size: buttonSize
        anchors {
            bottom: parent.bottom
            right:  addToOtherPlaylistButton.left
            rightMargin: 2*headerSpacing
        }
        source: "icons/select_all.png";
        onClicked: {
            let allSelected = cppRemote.allSongsSelected();
            cppRemote.selectAllSongsFromProxyModel(!allSelected);
            if (allSelected)
                songsView.currentIndex = -1;
        }
        visible: selectionMode
    } // selectAllButton
    ImageButton {
        id:   addToOtherPlaylistButton
        size: buttonSize
        anchors {
            bottom: parent.bottom
            right:  downSelectedSongsButton.left
            rightMargin: headerSpacing
        }
        source: "icons/addToPlayList.png";
        onClicked: {
            if (!mainApp.checkClementineVersion())
                return
            if (!cppRemote.appendSongsToOtherPlaylist())
                mainApp.error(qsTr("No track selected"), qsTr("Please select at least one Track"));
        }
        visible: selectionMode
    } // addToOtherPlaylistButton
    ImageButton {
        id:   downSelectedSongsButton
        size: buttonSize
        anchors {
            bottom: parent.bottom
            right:  deleteSongsButton.left
            rightMargin: 2*headerSpacing
        }
        source: "icons/nav_downloads.png";
        onClicked: {
            if (!mainApp.checkClementineVersion())
                return
            if (mainApp.downloadPossible())
                cppRemote.downloadSelectedSongs();
            cppRemote.selectAllSongsFromProxyModel(false);
            selectionMode = false;
        }
        visible: selectionMode
    } // downSelectedSongsButton
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
        id: noMorePlaylistDialog

        width: playlist.width *3/4

        x: (playlist.width - width) / 2
        y: (playlist.height - height) / 2

        title: qsTr("No more Playlists")

        Label {
            width: parent.width - 5
            text: qsTr("All available Playlists are already opened")
            wrapMode: Text.WordWrap
        }
    } // noMorePlaylistDialog


    ////////////////////////////////////////
    //      Components and Popups         //
    ////////////////////////////////////////

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
            id: songDelegateRect
            property bool isSelected: ListView.isCurrentItem

            width: ListView.view.width
            height: 50

            color: {
                if (selectionMode){
                    if (selected)
                        return colorSongSelected;
                } else {
                    if (activePlaylistIdx === playlistIdx && index === activeSongIdx)
                        return colorSongPlaying
                    else if (ListView.isCurrentItem)
                        return colorSongSelected;
                }
                return "white";
            } // color

            onIsSelectedChanged: {
                if (isSelected) {
                    if (txtTitle.truncated) {
                        txtTitle.elide = Text.ElideNone;
                        titleAnimation.start();
                    }
                    if (txtArtistAlbum.truncated) {
                        txtArtistAlbum.elide = Text.ElideNone;
                        artistAnimation.start();
                    }
                } else {
                    if (titleAnimation.running) {
                        titleAnimation.stop();
                        txtTitle.elide = Text.ElideRight;
                        txtTitle.x = 0;
                    }
                    if (artistAnimation.running) {
                        artistAnimation.stop();
                        txtArtistAlbum.elide = Text.ElideRight;
                        txtArtistAlbum.x = 0;
                    }
                }
            } // onIsSelectedChanged

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
                    to: -songDelegateRect.width
                    duration: 3000
                    loops: Animation.Infinite
                } // titleAnimation
                onTruncatedChanged: {
//                    print("onTruncatedChanged for: "+txtTitle.text)
                    if (truncated && songDelegateRect.isSelected) {
                        txtTitle.elide = Text.ElideNone;
                        titleAnimation.start();
                    }
                }
            } // txtTitle
            Text{
                id: txtLength
                text: pretty_length
                horizontalAlignment: Text.AlignRight
                anchors.right: parent.right
            } // txtLength
            Text{
                id: txtArtistAlbum
                x: 0
                width: parent.width
                elide: Text.ElideRight
                text: (artist !== "" || album !== "") ? artist + " / " + album : ""
                color: "blue"
                anchors.top: txtTitle.bottom

                NumberAnimation {
                    id: artistAnimation
                    target: txtArtistAlbum
                    property: "x"
                    from: 20
                    to: -songDelegateRect.width
                    duration: 3000
                    loops: Animation.Infinite
                } // artistAnimation
                onTruncatedChanged: {
//                    print("onTruncatedChanged for: "+txtArtistAlbum.text)
                    if (truncated && songDelegateRect.isSelected) {
                        txtArtistAlbum.elide = Text.ElideNone;
                        artistAnimation.start();
                    }
                }
            } // txtArtistAlbum

            MouseArea {
                anchors.fill: parent
                onClicked: {
//                    print("Clicked #" + index + ": " + title)
                    songsView.currentIndex = index
                    if (selectionMode)
                        selected = !selected;
                }
                onDoubleClicked: {
//                    print("onDoubleClicked #" + index + ": " + title )
                    songsView.currentIndex = index
                    cppRemote.changeToSong(index)
                }
                onPressAndHold: {
                    songsView.currentIndex = index
                    selectionMode = true
                    selected = true;
//                    print("onPressAndHold #" + index + ": " + title)
                }
            } // MouseArea
        } // songDelegateRect
    } // Component songDelegate

    Dialog {
        id: playlistDestructionConfirmationDialog

        width: playlist.width *3/4

        x: (playlist.width - width) / 2
        y: (playlist.height - height) / 2

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
        id: playlistChoiceDialog
        property alias text: openPlaylistLbl.text
        property alias combo: closedPlaylistCombo

        property bool  openPlaylist: true

        width: playlist.width *3/4

        x: (playlist.width - width) / 2
        y: (playlist.height - height) / 2
        parent: Overlay.overlay

        focus: true
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened:   closedPlaylistCombo.currentIndex = 0;
        onAccepted: {
            if (openPlaylist)
                cppRemote.openPlaylist(closedPlaylistCombo.currentValue);
            else
            {
                cppRemote.insertUrls(closedPlaylistCombo.currentValue, "");
                cppRemote.selectAllSongsFromProxyModel(false);
                selectionMode = false;
            }
        }
        onRejected: if (!openPlaylist) cppRemote.releaseUserMutex();

        ColumnLayout {
            spacing: 20
            anchors.fill: parent
            Label {
                id: openPlaylistLbl
                elide: Label.ElideMiddle
//                text: qsTr("Please select a Playlist to open")
                Layout.fillWidth: true
            }
            ComboBox {
                id: closedPlaylistCombo
//                model   : cppRemote.modelClosedPlaylists()
                delegate: ItemDelegate{
                    text: name ;
                    width: ListView.view.width;
                    highlighted: ListView.isCurrentItem
                }
                textRole: "name"
                valueRole: "id"
            }
        }
    } // playlistChoiceDialog

    Dialog {
        id: renPlaylistDialog

        property bool createNewPlaylist: false

        width: playlist.width *3/4

        x: (playlist.width - width) / 2
        y: (playlist.height - height) / 2
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
            onTriggered: {
                if (!mainApp.checkClementineVersion())
                    return
                cppRemote.savePlaylist(cppRemote.currentPlaylistID());
            }
        }
        Action {
            icon.source: "icons/renamePlaylist.png"
            text: qsTr("Rename Playlist")
            onTriggered: {
                if (!mainApp.checkClementineVersion())
                    return
                renPlaylistDialog.title = qsTr("Rename Playlist ") + playlistCombo.currentText;
                renPlaylistDialog.createNewPlaylist = false;
                renPlaylistDialog.open();
            }
        }
        Action {
            icon.source: "icons/clear.png"
            text: qsTr("Clear Playlist")
            onTriggered: {
                if (!mainApp.checkClementineVersion())
                    return
                cppRemote.clearPlaylist(cppRemote.currentPlaylistID());
            }
        }
        Action {
            icon.source: "icons/close.png"
            text: qsTr("Close Playlist")
            onTriggered: {
                // Won't allow removing the last playlist
                if (playlistCombo.count <= 1)
                    return;
                if (cppRemote.clementineFilesSupport()) {
                    if (!cppRemote.isCurrentPlaylistSaved())
                        playlistDestructionConfirmationDialog.open();
                    else
                        cppRemote.closePlaylist(cppRemote.currentPlaylistID());
                }
                else
                    cppRemote.closePlaylist(cppRemote.currentPlaylistID());
            }
        }
        Action {
            icon.source: "icons/nav_downloads.png"
            text: qsTr("Download Playlist")
            onTriggered: downloadPlaylist();
        }
        MenuSeparator {}
        Action {
            icon.source: "icons/open.png"
            text: qsTr("Open a Playlist")
            onTriggered: {
                if (!mainApp.checkClementineVersion())
                    return
                cppRemote.getAllPlaylists();
            }
        }
        Action {
            icon.source: "icons/newFile.png"
            text: qsTr("Create new Playlist")
            onTriggered: {
                if (!mainApp.checkClementineVersion())
                    return
                renPlaylistDialog.title = qsTr("Create New Playlist ");
                renPlaylistDialog.createNewPlaylist = true;
                renPlaylistDialog.open();
            }
        }
    } // playlistMenu
}
