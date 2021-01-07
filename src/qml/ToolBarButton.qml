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
import QtQuick.Controls 2.14

Item {
    id: button

    property string imagePath;
    property int    barIndex   : 0;
    property bool   selectable : true;

    property color color: "transparent"

    property double opacityInactive: 0.2
    property double opacityHover   : 0.4
    property double opacitySelected: 1

    property int borderWidth : 0
    property int borderRadius: 0


    property string previousState: "Inactive";

    property color badgeColor : "#ec3e3a";  // redish color (exactly the one used in OS X 10.10)



    // Allow the programmer to define the text to display.
    // Note that this control can display both text and numbers.
//    property alias badgeText: badgeLbl.text


    signal selected(int idx);



    //RectangItemle to draw the button
    Rectangle {
        id: rect

        anchors.fill: parent
        radius: borderRadius
        color: button.enabled ? button.color : "grey"

        border.width: borderWidth
        border.color: "black"

        opacity: opacityInactive

        Image {
            id: img
            anchors.fill: parent;
            source: imagePath;
            fillMode: Image.PreserveAspectFit;
        }
    }

    Rectangle {
        id: badge

        visible: false
        smooth: true

        // Create an animation when the opacity changes
        Behavior on opacity {NumberAnimation{}}

        // Setup the anchors so that the badge appears on the bottom right
        // area of its parent
        anchors.right: rect.right
        anchors.top: rect.top

        // This margin allows us to be "a little outside" of the object in which
        // we add the badge
//        anchors.margins: -parent.width / 5 + device.ratio(1)

        color: badgeColor

        // Make the rectangle a circle
        radius: width / 2

        // Setup height of the rectangle (the default is 18 pixels)
        height: 18

        // Make the rectangle and ellipse if the length of the text is bigger than 2 characters
        width: badgeLbl.text.length > 2 ? badgeLbl.paintedWidth + height / 2 : height

        // Create a label that will display the number of connected users.
        Label {
            id: badgeLbl
            color: "#fdfdfdfd"
            font.pixelSize: 9
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter

            // We need to have the same margins as the badge so that the text
            // appears perfectly centered inside its parent.
            anchors.margins: parent.anchors.margins
        }
    }


    //Mouse area to react on click events
    MouseArea {
        hoverEnabled: true
        anchors.fill: button
        onEntered: {
            button.previousState = button.state;
            if (button.state != 'Selected')
                button.state='Hover';

        }

        onExited : { button.state = previousState; }

        onClicked: {
            if (selectable)
            {
                button.previousState = 'Selected';
                button.state         = 'Selected';
            }
            button.selected(button.barIndex);
        }
    }


    function setBadge(msg)
    {
        if (msg !== "" && msg !== "0")
        {
            badge.visible = true;
            badgeLbl.text = msg;

            badge.width = badgeLbl.text.length > 2 ? badgeLbl.paintedWidth + badgeLbl.height / 2 : badgeLbl.height;
        }
        else
            badge.visible = false;

    }

    function clearBadge() {badge.visible = false;}


    //change the color of the button in different button states
    states: [
        State {
            name: "Inactive"
            PropertyChanges {
                target : rect
                opacity: opacityInactive
            }
        },
        State {
            name: "Hover"
            PropertyChanges {
                target : rect
                opacity: opacityHover
            }
        },
        State {
            name: "Selected"
            PropertyChanges {
                target : rect
                opacity: opacitySelected
            }
        }
    ]

    state: previousState;


    //define transmission for the states
    transitions: [
        Transition {
            from: ""; to: "Hover"
            OpacityAnimator { duration: 200 }
        },
        Transition {
            from: "*"; to: "Selected"
            OpacityAnimator { duration: 10 }
        }
    ]
}
