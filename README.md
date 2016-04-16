# About
Qt5 OpenHome Control Point.

# Building

## Linux

1. mkdir build
2. cd build
3. cmake .. -DCMAKE_BUILD_TYPE=Release
4. make
5. sudo make install


## Mac

Icon created under OSX using the createicon.sh in the mac folder.

The following steps are used to compile, and create the OS X application
bundle.

These steps assume the following structure:

    src/
    build/
    install/

1. Install Qt5.3.1 (or later) from https://qt-project.org/downloads

2. Install HomeBrew

3. brew install cmake

4. Load the top-level CMakeLists.txt in QtCreator, and pass the following to cmake:
     ../src -DCMAKE_PREFIX_PATH=/Users/$USER/Qt/5.3/clang_64/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=`pwd`/../install

5. Build via QtCreator

6. Create an 'install' project in QtCreator
  - Projects
  - Add/Clone Selected
  - Expand Build Steps, and select install

7. Build 'install' project via QtCreator


### Create Installer

1. Go into the 'mac' folder within the build folder

2. Run ./create-dmg.sh

