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

import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Session 1.0


Page {
    id: loginPage
    title: cppRemote.appTitle()

    property alias errMsg: errMsg;

    property string logoImage: "icon_large.png"
    property int    bottomMargin: 20
    property bool   initialized : false


    function connect() {
        let host = ipField.text;
        let port = parseInt(portField.text);
        let pass = parseInt(passField.text);
        cppRemote.tryConnectToServer(sessions.currentIndex, host, port, pass);
    }

    Timer {
        id: clearErrorTimer
        interval: 3000
        running: false
        repeat: false
        onTriggered: errMsg.text = " "
    }

    function displayError(err) {
        errMsg.text = err;
        clearErrorTimer.running = true;
    }

    function displayMessage(msg) {
        errMsg.text = "<font color='darkblue'><b><i>%1</i><b></font>".arg(msg);
        clearErrorTimer.running = true;
    }

    function displaySession() {
//        print("Session index: " + sessions.currentIndex);
        let session = cppRemote.getSession(sessions.currentIndex);
        ipField.text   = session.host;
        portField.text = session.port;
        if (session.pass !== -1)
            passField.text = session.pass;
        else
            passField.text = "";

        delSessionButton.visible = sessions.currentIndex != 0;
        connectButton.forceActiveFocus();
    }

    function deleteCurrentSession() {
        cppRemote.deleteCurrentSession();
        sessions.model = cppRemote.sessionNames();
        sessions.currentIndex = 0;
        displayMessage(qsTr("Session deleted"));
    }

    function saveSession(sessionName) {
        if (sessionName.length === 0) {
            displayError(qsTr("You should enter a name..."));
            return;
        }
        if (sessions.currentText === sessionName){
            if (sessions.currentIndex === 0) {
                displayError(qsTr("You can't name your session '%1'").arg(sessionName));
                return;
            }
            let session = cppRemote.getSession(sessions.currentIndex);
            session.host = ipField.text;
            session.port = parseInt(portField.text);
            session.pass = passField.text.length === 0 ? -1 : parseInt(passField.text);
            displayMessage(qsTr("Session saved"));
        }
        else {
            let lastIdx = cppRemote.createNewSession(sessionName,
                                       ipField.text,
                                       parseInt(portField.text),
                                       passField.text.length === 0 ? -1 : parseInt(passField.text));
            sessions.model = cppRemote.sessionNames();
            sessions.currentIndex = lastIdx;
            displayMessage(qsTr("Session created"));
        }
    }

    Connections{
        target: cppRemote
        function onConnectionError(err){
            errMsg.text = err;
        }
    }

    Component.onCompleted: {
        sessions.currentIndex = cppRemote.lastSessionIndex();
        initialized = true;
        displaySession();
    }

    anchors {
        fill: parent.fil;
        margins: 0;
    }

    focus: true
    Keys.onReturnPressed: connect()
    Keys.onEnterPressed:  connect()


    background: Rectangle{
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: root.bgGradiantStart }
            GradientStop { position: 1.0; color: root.bgGradiantStop }
        }
    }

    Image {
        id: logo
        source: logoImage;
        fillMode: Image.PreserveAspectFit;
        height: (parent.height - bottomRect.height- bottomMargin) / 2

        anchors.horizontalCenter: parent.horizontalCenter
        y: (parent.height - bottomRect.height- bottomMargin - height)/2

        opacity: 1
    }


    Column {
        id : bottomRect
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter

            bottomMargin: bottomMargin
        }
        width : parent.width /2
        spacing: 10

        Row {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter

            TextField {
                id: ipField
                placeholderText: qsTr("Enter the IP address")
                horizontalAlignment: TextInput.AlignHCenter

                color: "black"
                background: Rectangle { radius: 8 ; border.width: 0 }
            }
            TextField {
                id: portField
                validator: IntValidator {bottom: 1024; top: 65535;}
                placeholderText: qsTr("port")
                horizontalAlignment: TextInput.AlignHCenter

                color: "black"
                background: Rectangle { radius: 8 ; border.width: 0 }
            }
            TextField {
                id: passField
                validator: IntValidator {bottom: 0}
                placeholderText: qsTr("pass")
                horizontalAlignment: TextInput.AlignHCenter
                echoMode: TextInput.PasswordEchoOnEdit

                color: "black"
                background: Rectangle { radius: 8 ; border.width: 0 }
            }

            ImageButton {
                id:   saveSessionButton
                size: parent.height
                anchors.verticalCenter: parent.verticalCenter;
                source: "icons/save.png";
                onClicked: saveSessionDialog.open();
            }
        }

        Row {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter

            ComboBox {
                id: sessions
                model: cppRemote.sessionNames();
                anchors.verticalCenter: parent.verticalCenter;

                property int maxWidth: loginPage.width - delSessionButton.width - connectButton.width - parent.spacing*2 - leftPadding - 1.5*rightPadding
                property int modelWidth

                width: (modelWidth < maxWidth) ? modelWidth + leftPadding + 1.5*rightPadding : maxWidth
                height: ipField.height

                onCurrentIndexChanged: if (initialized) displaySession();

                // https://doc.qt.io/qt-5/qtquickcontrols2-customize.html#customizing-combobox
                background: Rectangle {
//                    border.color: sessions.pressed ? "#17a81a" : "#21be2b"
//                    border.width: sessions.visualFocus ? 2 : 1
                    radius: 8
                }

                delegate: ItemDelegate {
                    width: sessions.width
                    text: sessions.textRole ? (Array.isArray(sessions.model) ? modelData[sessions.textRole] : model[sessions.textRole]) : modelData
                    font.weight: sessions.currentIndex === index ? Font.DemiBold : Font.Normal
                    font.family: sessions.font.family
                    font.pointSize: sessions.font.pointSize
                    highlighted: sessions.highlightedIndex === index
                    hoverEnabled: sessions.hoverEnabled
                }

                TextMetrics {
                    id: textMetrics
                }

                // https://stackoverflow.com/questions/45029968/how-do-i-set-the-combobox-width-to-fit-the-largest-item
                onModelChanged: {
                    textMetrics.font = sessions.font
                    for(var i = 0; i < model.length; i++){
                        textMetrics.text = model[i]
                        modelWidth = Math.max(textMetrics.width, modelWidth)
                    }
                }
            }

            ImageButton {
                id:   delSessionButton
                size: saveSessionButton.height
                anchors.verticalCenter: parent.verticalCenter;
                source: "icons/delete.png";
                onClicked: delSessionDialog.open();
            }

            Button {
                id: connectButton
                text: '<font color="white">'+qsTr("Connect")+'</font>'

                font.family: "Arial"
                font.pointSize: 22

//                anchors {
//                    horizontalCenter: parent.horizontalCenter
//                }
//                palette: {
//                    button: "darkorange"
//                    buttonText: "white"
//                }
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 40
                    color: connectButton.down ? "darkmagenta" : "darkorange"
                    border.color: "#26282a"
                    border.width: 1
                    radius: 4
                }

                onClicked: connect()
            }
        }

        Text {
            id: errMsg
            anchors.horizontalCenter: parent.horizontalCenter
            text: " " // to have the empty line
        }
    }


    Dialog {
        id: saveSessionDialog

        width: parent.width*3/4

        x: (parent.width - width) / 2
        y: bottomRect.y - height
        parent: Overlay.overlay

        focus: true
        modal: true
        title: qsTr("Save Session")
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: saveSession(newSessionName.text);
        Keys.onReturnPressed: accept()
        Keys.onEnterPressed:  accept()

        ColumnLayout {
            spacing: 20
            anchors.fill: parent
            Label {
                elide: Label.ElideRight
                text: qsTr("Session name:")
                Layout.fillWidth: true
            }
            TextField {
                id: newSessionName
                focus: true
                placeholderText: "new session"
                Layout.fillWidth: true
                text: sessions.currentIndex == 0 ? "" : sessions.currentText
            }
        }
    }


    Dialog {
        id: delSessionDialog

        width: parent.width*3/4

        x: (parent.width - width) / 2
        y: bottomRect.y - height
        parent: Overlay.overlay

        modal: true
        title: qsTr("Delete Confirmation")
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: deleteCurrentSession()
        Keys.onReturnPressed: accept()
        Keys.onEnterPressed:  accept()

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: qsTr("Are you sure you want to delete the session <b>%1</b>?").arg(sessions.currentText);
        }
    }
}
