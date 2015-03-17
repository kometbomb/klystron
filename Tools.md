# Tools #

Since klystron uses own file formats and data structures, custom tools are provided in the _tools/_ directory.

## klystrack ##

_[klystrack](http://code.google.com/p/klystrack/)_ is the music and sound effects editor.

## editor ##

_[editor](editor.md)_ is a tile-based level editor that can export packed level files to be used by klystron. The levels contain one or more layers of tile data and any number of _events_ that are interpreted by the level loader. The engine itself doesn't care about the events and only passes them to the client.

The idea behind generic events is that the editor is not tied to only one game. Each event has a number of parameters that in part can contain enumerations of enemy types et cetera. The events also have the property of width and height so they can be used to define areas in the level to e.g. trigger events in the game (and that's why the objects are called "events").

## makebundle ##

_makebundle_ is used to [bundle multiple files](CreatingBundles.md) inside one klystron data container (called a [bundle](Bundle.md)).