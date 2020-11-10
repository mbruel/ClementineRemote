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

Item {
    id: mainApp

    visible: true
    width  : 400
    height : 600

    property string title: qsTr("no title...")

    property int toolBarIndex       : 0
    property int toolBarHeight      : 50
    property int toolBarSpacing     : 15
    property int toolBarMargin      : 20
    property int toolBarButtonWidth : 50

    property int headerButtonSize   : 30
    property int mainMargin         : 20

    property int downDialogTimeout  : 8000

    property int trackLength: cppRemote.currentTrackLength()

    property color bgGradiantStart: "#8b008b" // darkmagenta (https://doc.qt.io/qt-5/qml-color.html)
    property color bgGradiantStop:  "#ff8c00" // darkorange


    ListModel { // ListModel for ToolBar
        id: toolBarModel

        ListElement { name : "playlist" ; color: "white" }
        ListElement { name : "file"     ; color: "lightgray" }
        ListElement { name : "internet" ; color: "ivory" }
        ListElement { name : "library"  ; color: "lightblue" }
        ListElement { name : "search"   ; color: "lightgreen" }
    }
    ListModel { // ListModel for repeatMenu
        id: repeatModel
        ListElement { text : qsTr("Don't Repeat")    ; icon: "icons/ab_repeat_off.png" }
        ListElement { text : qsTr("Repeat Track")    ; icon: "icons/ab_repeat_track.png" }
        ListElement { text : qsTr("Repeat Album")    ; icon: "icons/ab_repeat_album.png" }
        ListElement { text : qsTr("Repeat Playlist") ; icon: "icons/ab_repeat_playlist.png" }
    }
    ListModel { // ListModel for shuffleMenu
        id: shuffleModel
        ListElement { text : qsTr("Don't Shuffle")                ; icon: "icons/ab_shuffle_off.png" }
        ListElement { text : qsTr("Shuffle All")                  ; icon: "icons/ab_shuffle.png" }
        ListElement { text : qsTr("Shuffle tracks in this Albums"); icon: "icons/ab_shuffle_album.png" }
        ListElement { text : qsTr("Shuffle Albums")               ; icon: "icons/ab_shuffle_albums.png" }
    }


    Connections{
        target: cppRemote

        function onCurrentSongLength(length, pretty_length){
            print("new track duration: "+pretty_length + " ("+length+")");
            trackLength = length;
            trackDuration.text = pretty_length;
        }
        function onCurrentTrackPosition(pos){
            trackPosition.text = cppRemote.prettyLength(pos);
            trackSlider.value  = pos / trackLength;
        }
        function onUpdateVolume(vol){
            volPct.text = cppRemote.volumePct();
            volumeSlider.value = vol / 100;
        }
        function onUpdateEngineState(){ updatePlayerState(); }
        function onUpdateShuffle(mode){ updateShuffle(mode); }
        function onUpdateRepeat(mode) { updateRepeat(mode); }

        function onDownloadComplete(downloadedFiles, totalFiles, errorList){
            downloadsDialog.title = qsTr("Download Complete");
            downloadsDialog.text  = qsTr("%1 / %2 file(s) have been downloaded successfully!").arg(
                        downloadedFiles).arg(totalFiles)

            if (errorList.length > 0)
            {
                downloadsDialog.text += "<br/><br/>" +qsTr("There were %1 error(s):").arg(errorList.length);
                downloadsDialog.text += "<ul>";
                for (const err of errorList)
                    downloadsDialog.text += "<li>" + err + "</li>";
                downloadsDialog.text += "</ul>";
            }
            downloadsDialog.open();
            downloadsTimer.start();

            cppRemote.setIsDownloading(false);
        }
    } // Connections cppRemote

    Component.onCompleted: {
        changeMainMenu(toolBarIndex);
        updatePlayerState();
        updateRepeat(cppRemote.repeatMode());
        updateShuffle(cppRemote.shuffleMode());

        //TODO get the Slider orientation from C++ QSettings
        // have a Setting page where this can be changed ;)
        useVolumeButtonWithVerticalSlider(true);
        useVolumeButtonWithVerticalSlider(false);
    } // Component.onCompleted


    function changeMainMenu(selectedMenu){
        console.log("New idx: "+selectedMenu);

        // make previous ToolBarButton Inactive
        for (var i = 0; i < toolBar.children.length; ++i)
        {
            if (i !== selectedMenu)
                toolBar.children[i].state = "Inactive";
        }

        // is there not supported Menus?
        if ([1, 2].includes(selectedMenu) && !cppRemote.clementineFilesSupport() ){
            mainArea.setSource("NotSupported.qml");
            mainArea.item.title = (selectedMenu === 1 ? qsTr("Files") : qsTr("Radios"));
            return;
        }

        if (selectedMenu === 0)
            mainArea.sourceComponent = playlistPage;
        else if (selectedMenu === 1)
        {
            cppRemote.getServerFiles(cppRemote.remoteFilesPath_QML());
            mainArea.sourceComponent = filesPage;
        }
        else if (selectedMenu === 2)
        {
            cppRemote.getServerFiles(cppRemote.remoteFilesPath_QML());
            mainArea.sourceComponent = radiosPage;
        }
        else
        {
            mainArea.setSource("SimpleMainArea.qml");
            mainArea.item.lbl.text = toolBarModel.get(selectedMenu).name;//menuList[selectedMenu];
            mainArea.item.color    = toolBarModel.get(selectedMenu).color;
            mainArea.opacity = 0.8;
        }
    } // function changeMainMenu

    function updatePlayerState(){
        var enableStop = true;
        if (cppRemote.isPlaying())
            playSong.source =  "icons/media-playback-pause.png";
        else if (cppRemote.isPaused())
            playSong.source =  "icons/media-playback-start.png";
        else{
            playSong.source =  "icons/media-playback-start.png";
            enableStop = false;
        }
        stopSong.enabled = enableStop
        stopSong.opacity = enableStop ? 1 : 0.5
    } // function updatePlayerState

    function useVolumeButtonWithVerticalSlider(useVolButton) {
        if (useVolButton)
        {
            // make volButton visible and invert its position with volPct
            volButton.visible       = true;
            volButton.anchors.right = playerBar.right;
            volPct.anchors.right    = volButton.left;

            // rotate volumeSlider and set it above volButton
            volumeSlider.anchors.right = undefined;
            volumeSlider.anchors.left  = volButton.left;
            rotationSliderVol.angle    = -90;
            volumeSlider.width         = 200;
            volumeSlider.visible       = false;
        }
        else
        {
            volButton.visible       = false;
            volPct.anchors.right    = playerBar.right;
            volButton.anchors.right = volPct.left;

            volumeSlider.anchors.left  = undefined;
            volumeSlider.anchors.right = volPct.left;
            rotationSliderVol.angle    = 0;
            volumeSlider.width         = Qt.binding(function() { return volumeSlider.resizableWidth; })
            volumeSlider.visible       = true;
        }
    } // function useVolumeButtonWithVerticalSlider

    function updateShuffle(mode) {
        switch(mode)
        {
        case 0:
            shuffleButton.source = "icons/ab_shuffle_off.png";
            break;
        case 1:
            shuffleButton.source = "icons/ab_shuffle.png";
            break;
        case 2:
            shuffleButton.source = "icons/ab_shuffle_album.png";
            break;
        case 3:
            shuffleButton.source = "icons/ab_shuffle_albums.png";
            break;
        default:
            shuffleButton.source = "icons/ab_shuffle_off.png";
            break;
        }
    } // function updateShuffle

    function updateRepeat(mode){
        switch(mode)
        {
        case 0:
            repeatButton.source = "icons/ab_repeat_off.png";
            break;
        case 1:
            repeatButton.source = "icons/ab_repeat_track.png";
            break;
        case 2:
            repeatButton.source = "icons/ab_repeat_album.png";
            break;
        case 3:
            repeatButton.source = "icons/ab_repeat_playlist.png";
            break;
        default:
            repeatButton.source = "icons/ab_repeat_off.png";
            break;
        }
    } // function updateRepeat

    function downloadPossible() {
        if (cppRemote.downloadsAllowed())
        {
            if (cppRemote.isDownloading())
            {
                downloadsDialog.title = qsTr("Already Downloading...");
                downloadsDialog.text  = qsTr("There is already a Download in process.") +
                        "<br/>" + qsTr("Either cancel it or wait its end.");
                downloadsDialog.open();
                downloadsTimer.start();
                return false;
            }
            let downloadPath = cppRemote.downloadPath();
            if (downloadPath === "")
            {
                let error = cppRemote.setDownloadFolder();
                if (error === "")
                    downloadPath = cppRemote.downloadPath()
                else
                {
                    downloadsDialog.title = qsTr("Download Error");
                    downloadsDialog.text  = error;
                    downloadsDialog.open();
                    downloadsTimer.start();
                    return false;
                }
            }
            print("downloadPath: "+downloadPath);
            return true;
        }
        else
        {
            downloadsDialog.title = qsTr("Downloads forbidden...");
            downloadsDialog.text  = qsTr("Downloads are not allowed on Clementine...<br/><br/>\
You can change that in:<br/>Tools -> Preferences -> Network Remote");
            downloadsDialog.open();
            downloadsTimer.start();
            return false;
        }
    } // downloadPossible

    function downloadCurrentSong() {
        if (downloadPossible())
        {
            cppRemote.setIsDownloading(true);
            cppRemote.downloadCurrentSong();
        }
    } // downloadCurrentSong


    ////////////////////////////////////////
    //            QML Items               //
    ////////////////////////////////////////


    Rectangle {
        id: background
        anchors {
            fill: parent;
            margins: 0;
        }
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: bgGradiantStart }
            GradientStop { position: 1.0; color: bgGradiantStop }
        }
        opacity: 0.8
    } // background


    ToolBarButton {
        id: nav_menu
        anchors {
            top       : parent.top
            left      : parent.left
            topMargin : toolBarMargin
            leftMargin: toolBarMargin
        }
        width : toolBarButtonWidth
        height: toolBar.height

        imagePath : "icons/nav_menu.png"
        onSelected:  drawer.open();
    } // nav_menu

    Row {
        id: toolBar
        anchors {
            top       : parent.top
            left      : nav_menu.right
            topMargin : toolBarMargin;
            leftMargin: 2*mainMargin
//            leftMargin: 2*toolBarMargin + (parent.width - width - 3*toolBarMargin - nav_menu.width)/2
//            horizontalCenter: parent.horizontalCenter
        }
        height : toolBarHeight
        spacing: toolBarSpacing

        Repeater {
            model: toolBarModel; //menuList
            ToolBarButton {
                id: button
                width : toolBarButtonWidth
                height: toolBar.height

                barIndex   : model.index
                imagePath  : "icons/" + model.name + ".png"

                state      : barIndex === mainApp.toolBarIndex ?  "Selected" : "Inactive"
                onSelected : changeMainMenu(barIndex)
            } // ToolBarButton
        } // Repeater
    } // toolBar



    Loader {
        id: mainArea
        anchors {
            top      : toolBar.bottom
            left     : parent.left
            topMargin: toolBarMargin;
        }
        width : parent.width
        height: parent.height - toolBarHeight - 3*toolBarMargin - playerBar.height
    } // mainArea


    Rectangle {
        id: playerBar
        height: 2*toolBarMargin + headerButtonSize +trackSlider.height
        width: parent.width - 2*toolBarMargin
        anchors {
            bottom     : parent.bottom
            left       : parent.left
            leftMargin : toolBarMargin
            rightMargin: toolBarMargin
        }
//        border { width: 2 ; color: "black" }
        color: "transparent"

        Row {
            id: trackRow
            anchors{
                top  : parent.top
                right: parent.right
            }
            width: parent.width

            ImageButton {
                id: downSong
                size     : headerButtonSize
                source   : "icons/nav_downloads.png";
                onClicked: downloadCurrentSong()
            } // downSong

            Slider {
                id: trackSlider
                value: 1/3

//                anchors{
//                    top: parent.top
//                    topMargin: toolBarMargin
//                    horizontalCenter: parent.horizontalCenter
//                }

                width: parent.width - 3*headerButtonSize

                background: Rectangle {
                    x: trackSlider.leftPadding
                    y: trackSlider.topPadding + trackSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: trackSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#bdbebf"

                    Rectangle {
                        width: trackSlider.visualPosition * parent.width
                        height: parent.height
                        color: "blue";//"#21be2b"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: trackSlider.leftPadding + trackSlider.visualPosition * (trackSlider.availableWidth - width)
                    y: trackSlider.topPadding + trackSlider.availableHeight / 2 - height / 2
                    implicitWidth: 22
                    implicitHeight: 22
                    radius: 13
                    color: trackSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                    border.color: "#bdbebf"
                }

                onPressedChanged: {
                    if (!pressed)
                    {
                        print("onPressedChanged trackPosition: "+Math.round(value*trackLength))
                        cppRemote.setTrackPostion(Math.round(value*trackLength))
                    }
                }
//                onMoved:{
////                    print("new trackPosition: "+value*trackLength)
//                }
            } // trackSlider

            ImageButton {
                id: repeatButton
                size     : headerButtonSize
                source   : "icons/ab_repeat_off.png";
                onClicked: repeatMenu.open()
            } // repeatButton

            ImageButton {
                id: shuffleButton
                size     : headerButtonSize
                source   : "icons/ab_shuffle_off.png";
                onClicked: shuffleMenu.open();

            }
        } // trackRow


        Text{
            id: trackPosition
            anchors{
                top: trackRow.top
                left: trackRow.left
                leftMargin: headerButtonSize + toolBarMargin
                topMargin: -5
            }
            text: ""
        } // trackPosition

        Text{
            id: trackDuration
            anchors{
                top: trackRow.top
                right: trackRow.right
                rightMargin: 2*headerButtonSize + toolBarMargin
                topMargin: -5
            }
            text: cppRemote.currentTrackDuration()
        } // trackDuration



        // Second line
        Row {
            id: commandsRow
            anchors{
                left        : parent.left
                bottom      : parent.bottom
                bottomMargin: toolBarMargin
            }
            spacing: 10

            ImageButton {
                id: previousSong
                size     : headerButtonSize
                source   : "icons/media-skip-backward.png";
                onClicked: cppRemote.previousSong();
            } // previousSong
            ImageButton {
                id: playSong
                size     : headerButtonSize
                source   : "icons/media-playback-start.png";
                onClicked: {
                    cppRemote.playpause();
                    updatePlayerState();
                }
            } // playSong
            ImageButton {
                id: stopSong
                size     : headerButtonSize
                source   : "icons/media-playback-stop.png";
                onClicked: {
                     cppRemote.stop();
                     updatePlayerState();
                }
            } // stopSong
            ImageButton {
                id: nextSong
                size     : headerButtonSize
                source   : "icons/media-skip-forward.png";
                onClicked: cppRemote.nextSong();
            }// nextSong
        } // commandsRow

        Slider {
            id: volumeSlider
            value: cppRemote.volume()/100

            property int resizableWidth : parent.width - volPct.width - (headerButtonSize + commandsRow.spacing + toolBarMargin) * 4
            width: resizableWidth

            anchors{
                right       : volPct.left
                bottom      : parent.bottom
                bottomMargin: toolBarMargin
                rightMargin : toolBarMargin
            }
            background: Rectangle {
                x: volumeSlider.leftPadding
                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                implicitWidth : 200
                implicitHeight: 4
                width : volumeSlider.availableWidth
                height: implicitHeight
                radius: 2
                color : "#bdbebf"

                Rectangle {
                    width : volumeSlider.visualPosition * parent.width
                    height: parent.height
                    color : "#21be2b"
                    radius: 2
                }
            }
            handle: Rectangle {
                x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                implicitWidth : 26
                implicitHeight: 26
                color: "transparent"
                Image{
                    source  : "icons/sound.png"
                    fillMode: Image.PreserveAspectFit;
                    width   : parent.width
                    height  : parent.height
                }
            }
            onPressedChanged: {
                if (!pressed)
                {
                    cppRemote.setCurrentVolume(Math.round(value*100));
                    volPct.text = cppRemote.volumePct();
                }
            }            
            transform: [
                Rotation {
                    id: rotationSliderVol
                    origin { x: 0; y: 0; z: 0}
                    angle: 0
                }
            ]
        } // volumeSlider


        ImageButton {
            id: volButton

            anchors{
                right       : volPct.left
                bottom      : parent.bottom
                bottomMargin: toolBarMargin
            }
            visible: false

            size     : headerButtonSize
            source   : "icons/sound.png";
            onClicked: volumeSlider.visible = !volumeSlider.visible;
        } // volButton

        Text{
            id: volPct
            anchors{
                right         : parent.right
                rightMargin   : toolBarMargin
                verticalCenter: volumeSlider.verticalCenter
            }
            text: cppRemote.volumePct();
        } // volPct
    } // playerBar



    ////////////////////////////////////////
    //      Components and Popups         //
    ////////////////////////////////////////


    Component {
        id: playlistPage

        Playlist {
            anchors.fill  : parent
        }
    } // playlistPage

    Component {
        id: filesPage

        ServerFiles {
            anchors.fill  : parent
            headerButtonSize: headerButtonSize
        }
    } // filesPage

    Component {
        id: radiosPage

        SavedRadios {
            anchors.fill  : parent
        }
    } // radiosPage

    Dialog {
        id: todoDialog

        width: mainApp.width *4/5
        x: (mainApp.width - width) / 2
        y: (mainApp.height - height) / 2

        title: "TODO"

        Label {
            width: parent.width - 5
            wrapMode: Text.WordWrap
            text: qsTr("this feature is not implemented yet...")
        }
    } // todoDialog

    Dialog {
        id: downloadsDialog
        width: mainApp.width *4/5
        property alias text: downloadsDialogLbl.text


        x: (mainApp.width - width) / 2
        y: (mainApp.height - height) / 2

        title: qsTr("Downloads forbidden...")

        Label {
            id: downloadsDialogLbl
            width: downloadsDialog.width - 5
            wrapMode: Text.WordWrap
            text: "to be set"
        }

        Timer {
            id: downloadsTimer
            interval: downDialogTimeout;
            running: false;
            repeat: false
            onTriggered: downloadsDialog.close()
        }
    } // todoDialog

    Menu {
        id: repeatMenu
        x: repeatButton.x - width + mainMargin
        y: mainApp.height - height
        transformOrigin: Menu.TopRight

        Repeater {
            model: repeatModel;
            MenuItem {
                text       : model.text
                icon.source: model.icon;
                onTriggered: {
                    cppRemote.repeat(model.index);
                    updateRepeat(model.index);
                }
            }
        } // Repeater
    } // repeatMenu

    Menu {
        id: shuffleMenu
        x: shuffleButton.x - width + mainMargin
        y: mainApp.height - height
        transformOrigin: Menu.TopRight

        Repeater {
            model: shuffleModel;
            MenuItem {
                text       : model.text
                icon.source: model.icon;
                onTriggered: {
                    cppRemote.repeat(model.index);
                    updateRepeat(model.index);
                }
            }
        } // Repeater
    } // shuffleMenu

    Drawer {
        id: drawer
        width : parent.width * 2/3
        height: parent.height

//        height: drawerContent.height //parent.height / 2
        // Make drawer comes from the left
        edge: Qt.LeftEdge
//        opacity: 0.8

        Column {
            anchors.fill: parent
            spacing: toolBarMargin

            Rectangle {
                id: drawerHeader
                anchors.horizontalCenter: parent.horizontalCenter
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: bgGradiantStart }
                    GradientStop { position: 1.0; color: bgGradiantStop }
                }
                width: drawer.width;// - 20;
                height: 100

                Image {
                    id: drawerLogo
                    width   : 50
                    height  : 50
                    source  : "icon_large.png";
                    fillMode: Image.PreserveAspectFit;
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                        leftMargin: toolBarSpacing
                    }
                } // drawerLogo
                Text {
                    id: drawerHeaderTxt
                    anchors {
                        left: drawerLogo.right
                        verticalCenter: parent.verticalCenter
                        leftMargin: toolBarSpacing
                    }
                    width: parent.width - drawerLogo.width - 2*toolBarSpacing
                    font.pointSize: 18;
                    wrapMode: Text.WordWrap

                    text    : qsTr("Clementine on ") + cppRemote.hostname()
                } // drawerHeaderTxt
            }// drawerHeader

            ItemDelegate {
                id: settings
                width: parent.width
                text : qsTr("Settings")
                icon.source: "icons/nav_settings.png"
                onClicked: {
                    console.log("onClicked " + settings.text);
                    drawer.close();
                    todoDialog.open();
                }
//                highlighted: true;
            } // settings
            ItemDelegate {
                id: downloads
                width: parent.width
                text : qsTr("Downloads")
                icon.source: "icons/nav_downloads.png"
                onClicked: {
                    console.log("onClicked " + downloads.text);
                    drawer.close();
                    todoDialog.open();
                }
            } // downloads
            ItemDelegate {
                id: donate
                width: parent.width
                text : qsTr("Donate")
                icon.source: "icons/nav_donate.png"
                onClicked: {
                    console.log("onClicked " + donate.text)
                    drawer.close();
                    todoDialog.open();
                }
            } // donate
            ItemDelegate {
                id: about
                width: parent.width
                text : qsTr("About")
                icon.source: "icon.png"
                onClicked: {
                    console.log("onClicked " + about.text)
                    drawer.close();
                    todoDialog.open();
                }
            } // about
            ItemDelegate {
                id: logout
                width: parent.width
                text : qsTr("Disconnect")
                icon.source: "icons/nav_quit.png"
                onClicked: {
                    console.log("onClicked " + logout.text)
                    drawer.close();
                    drawer.disconnect = true;
                }
            } // logout
        }
        // to disconnect we need to wait that the Drawer is completely closed
        // otherwise the Overlay would stay
        property bool disconnect: false
        onClosed: if (disconnect) cppRemote.disconnectFromServer();

//        Overlay.modal: Rectangle {
//            color: "red"
//        }
    } // drawer
}
