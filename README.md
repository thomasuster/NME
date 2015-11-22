## Native Media Engine

[![Build Status](https://travis-ci.org/haxenme/nme.png?branch=master)](https://travis-ci.org/haxenme/nme) [![Join the chat at https://gitter.im/haxenme/nme](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/haxenme/nme?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

A proven backend for native iOS, Android, Windows, Mac and Linux

### Installation

1. Install [Haxe](http://www.haxe.org)

1. Install NME
```
haxelib install hxcpp
haxelib install nme
haxelib run nme setup
```

To install a specific version: 
Go to http://nmehost.com/nme
Download a version, for example nme-5.5.11.zip
```haxelib install ~/Downloads/nme-5.5.11.zip```

### Building applications

NME comes with a custom build tool to configure the application, define the assets and manage the platform settings. The application is configured with a _.nmml file instead of a _.hxml_.

Build applications using one of the build targets:
```
cd nme/samples/DisplayingABitmap
nme test neko
nme test cpp
nme test flash
nme test mac
nme test windows
nme test android
nme test webos
nme test ios
````

 > *Note:* `nme` is a shortcut to `haxelib run nme`

### Learning NME

To learn NME by example, check out the [sample projects](https://github.com/haxenme/nme/tree/master/samples). These are also included in the installation. Every sample project contains the _.hx_ Haxe sources and the _.nmml_ config file to build the example.

