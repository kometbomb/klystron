# Creating bundles #

_makebundle_ is used to package multiple files in one data container (called a [bundle](Bundle.md)). E.g. a font is a bundle of an image file and a text file with the character map. The bundles can be accessed in random fashion. The bundle can contain more bundles.

For example, a klystron font is a bundle of the following directory structure:

```
fontdata/charmap.txt
fontdata/font.bmp
```

The actual bundle is created with makebundle as follows:

```
makebundle font.fnt fontdata
```

This bundles all files in the directory _fontdata_ as _font.fnt_. The font could then, for example, be bundled inside a larger bundle containing all game data.