<img align="left" width="80" height="80" src="https://raw.githubusercontent.com/mbruel/ClementineRemote/main/src/data/icon.png" alt="ClementineRemote">

# ClementineRemote
Remote for Clementine Music Player developped in C++/QT5/QML to be available on any OS. <br/>
Its main goal is to add the Files View missing in the Android Remote.
<br/><br/>
The modification needed in Clementine can be found on [my fork](https://github.com/mbruel/Clementine). I've asked 4 pull requests that are not yet merged so I've build the release myself for Linux, MacOS and Windows (using the Clementine workflow on my branch)<br/>
**You can download [Clementine v1.4.0rc1-391-g90ec6c58b](https://github.com/mbruel/Clementine/releases/tag/1.4.0rc1-391-g90ec6c58b)**

<img width="250" src="https://raw.githubusercontent.com/mbruel/ClementineRemote/main/pics/ClementineRemote_Playlist.png" alt="ClementineRemote on Debian"/> <img width="250" src="https://raw.githubusercontent.com/mbruel/ClementineRemote/main/pics/ClementineRemote_iOS.png" alt="ClementineRemote on iPhone SE"/> <img width="250" src="https://raw.githubusercontent.com/mbruel/ClementineRemote/main/pics/ClementineRemote_Radio.png" alt="ClementineRemote on iPhone SE"/><br/>
(first snapshot is from a Debian Laptop, second from an iPhone SE first generation (small screen that's why the volume slider is vertical)

### How to build
#### Dependencies:
- build-essential (C++ compiler, libstdc++, make,...)
- qt5-default (Qt5 libraries and headers)
- qt5-qmake (to generate the moc files and create the Makefile)
- protobuf-compiler and libprotobuf-dev

#### Build:
- go to `src/protobuf/` and generate the protobuf C++ file with the command `protoc remotecontrolmessages.proto --cpp_out=.`
- go to `src`
- qmake
- make


### Donations
I'm Freelance nowadays, working on several personal projects, so if you use the app and would like to contribute to the effort, feel free to donate what you can.<br/>
<br/>
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2C236U6JNTUA&item_name=ClementineRemote&currency_code=EUR"><img align="left" src="https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif" alt="ClementineRemote"></a>
 or in Bitcoin at this address: **3BGbnvnnBCCqrGuq1ytRqUMciAyMXjXAv6**
<img align="right" align="bottom" width="120" height="120" src="https://raw.githubusercontent.com/mbruel/ngPost/master/pics/btc_qr.gif" alt="ngPost_QR">
