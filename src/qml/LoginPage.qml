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


Page {
    property alias errMsg: errMsg;
    title: cppRemote.appTitle()

    property string logoImage: "icon_large.png"
    property int    bottomMargin: 20

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

                color: "black"
                background: Rectangle { radius: 8 ; border.width: 0 }
            }

        }

        Button {
            id: connectButton
            text: '<font color="white">'+qsTr("Connect")+'</font>'

            font.family: "Arial"
            font.pointSize: 22

            anchors {
                horizontalCenter: parent.horizontalCenter
            }
//            palette: {
//                button: "darkorange"
//                buttonText: "white"
//            }
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

        Text {
            id: errMsg
            anchors.horizontalCenter: parent.horizontalCenter
            text: " " // to have the empty line
        }


    }

    function connect() {
        var ip = ipField.text;
        var port = parseInt(portField.text);
        var pass = parseInt(passField.text);
        cppRemote.connectToServer(ip, port, pass);
    }

    Connections{
        target: cppRemote
        function onConnectionError(err){
            errMsg.text = err;
        }
    }

    Component.onCompleted: {
        var ip = cppRemote.settingHost();
        var port = cppRemote.settingPort();
        var pass = cppRemote.settingPass()
        if (ip !== "")
            ipField.text = ip;
        if (port !== "")
            portField.text = port;
        if (pass !== "")
            passField.text = pass;
    }
}
