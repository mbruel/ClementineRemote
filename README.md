<img align="left" width="80" height="80" src="https://raw.githubusercontent.com/mbruel/ClementineRemote/main/src/data/icon.png" alt="ClementineRemote">

# ClementineRemote
Remote for Clementine Music Player developped in C++/QT5/QML to be available on any OS. 
Its main goal is to add the Files View missing in the Android Remote.

![](https://raw.githubusercontent.com/mbruel/ClementineRemote/main/pics/ClementineRemote_v0.1.png)


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
