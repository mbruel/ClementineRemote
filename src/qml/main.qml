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

import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    id: root

    visible: true
    width: 414
    height: 736
    title: cppRemote.appTitle()

    property string urlAuth    : "https://mbruel.fr/cgi-bin/appAuth.pl"
    property string postUser   : "login"
    property string postPass   : "pass"
    property int httpTimeoutMS : 4000


    property color bgGradiantStart: "#8b008b" // darkmagenta (https://doc.qt.io/qt-5/qml-color.html)
    property color bgGradiantStop:  "#ff8c00" // darkorange

    property int toolBarIndex       : 0
    property int toolBarHeight      : 50;
    property int toolBarSpacing     : 15;
    property int toolBarMargin      : 10;
    property int toolBarButtonWidth : width/10;

    property int headerButtonSize   : 42;
    property int mainMargin         : 20;


//    Keys.onReleased: {
//        console.log("Key pressed: "+event.key);
//        if (event.key === Qt.Key_Back) {
//            console.log("BACK");
//            onDisconnected();
//        }
//    }

    onClosing: {
        console.log("Close event");
//        close.accepted = false
//        mainArea.sourceComponent = loginPage;
    }

    Loader {
        id: mainArea
        anchors.fill: parent
    }

//    Component.onCompleted:  mainArea.sourceComponent = mainApp;
    Component.onCompleted:  mainArea.sourceComponent = loginPage;


    Connections {
        target: cppRemote

        function onConnected() {
            print("connected");
            mainArea.sourceComponent = mainApp;
        }

        function onDisconnected(reason)
        {
            print("disconnected");
            mainArea.sourceComponent = loginPage;
            mainArea.item.errMsg.text = reason;
        }

//        onError: error(txt);
    }

    Component {
        id: loginPage

        LoginPage {
            anchors.fill  : parent
        }
    }

    Component {
        id: mainApp

        MainApp {
            title              : root.title
            toolBarIndex       : root.toolBarIndex
            toolBarHeight      : root.toolBarHeight
            toolBarSpacing     : root.toolBarSpacing
            toolBarMargin      : root.toolBarMargin
            toolBarButtonWidth : root.toolBarButtonWidth

            headerButtonSize   : root.headerButtonSize
            mainMargin         : root.mainMargin
        }
    }
}
