

# Project files #

_editor_ needs basic project information that is described in .CFG files. The main file structure is of [libconfig](http://www.hyperrealm.com/libconfig/) format.

Here's a project definition for an imaginary game:

```
tileset = "/path/to/tiles.bmp";
screen : { width = 640; height = 480; scale = 2; };
playfield : { width = 240; height = 320; };
bg_color = 0x0;
events = 
(
	{ 
		name = "Path";
		color = 0xff00ff; 		     
		params = ( { name = "Curve"; enum = [ "Linear", "Hermite" ]; }, { name = "Speed"; range = [ 0, 255 ]; default = 128; }  );
	},
	{
		name = "Enemy";
		color = 0xff0000;
		params = ( { name = "Type"; enum = [ "Bug", "Lazers" ]; } );
	}
	
);
```

**tileset** is a path to an image which will be split into 8x8 tiles.

**screen** tells the main window size. If **scale** is 2, the actual window will be twice that size (2x2 pixels).

The **playfield** setting defines an area that is drawn over the main map view that can be used to simulate the actual resolution of the game. It is useful so that you can see what would be outside the visible screen.

**bg\_color** is a hexadecimal RGB value, e.g. 0xFF0000 is red.

## Events ##

**events** describes the different entities that can be placed on the map. Each klystron event has 16 parameters, the level editor uses three of them for its own uses (param 0 becomes the event type and the last two parameters are used to link events together). The rest of the events can be defined for each project. In the above example, two different event types are declared (in the editor, the parameter 0 will select the event type). Each event type can have its own set of parameters, each of which can be either an integer value or an enumerated value.

**name** is the name of the event type and **color** is an RGB value that specifies the color events of this type are drawn on the map.

**params** is a _list_ of _groups_, each group having a **name** setting and either an **enum** or a **range** setting. **default** is the default value for the parameter (for enumerations, **default** is the index of the value).

# Using _editor_ #

Left click selects/draws tiles, right drag scrolls the map.

|**Cursor keys**|Scroll screen|
|:--------------|:------------|
|**Ctrl+O**|Open a map file|
|**Ctrl+S**|Save map file|
|**Ctrl+F9**|Export the map and events to a packed klystron level file|
|**E**|Switch between event/map editing|

## Tile editing ##

|**Alt**|Hold down to display the tile palette|
|:------|:------------------------------------|
|**Shift+Left Click**|Drag to select an area and use that as a brush|
|**A**|Hold down to display all layers simultaneously|
|**B**|Edit brush|
|**P**|Toggle parallax for current layer|
|**N/M**|Adjust X/Y parallax scale|
|**X/Y**|Toggle X/Y repeat for current layer|
|**1-8**|Select layer|
|**PgUp/PgDown**|Move current layer up or down|
|**Ctrl+Cursor keys**|Resize current layer|
|**Ctrl+F10**|Clear current layer|

## Event editing ##

Use mouse to select and move events.

|**Insert**|Add new event. If an event is selected, the new event will be a clone of the selected event.|
|:---------|:-------------------------------------------------------------------------------------------|
|**Delete**|Delete selected event|
|**PgUp/PgDown**|Select event parameter|
|**Comma/Period**|Adjust selected parameter|
|**Ctrl+Cursor keys**|Resize selected event|
|**Ctrl+Left Click**|Link the currently selected event to another event (EV\_NEXT link)|
|**Ctrl+Shift+Left Click**|Link the currently selected event to another event (EV\_TRGPARENT link)|

# About events #

The "events" (or, entities, objects, whatever) in klystron are very relaxed in that they can represent anything. They simply have a position, size and a few parameters. It is the job of the level loader to interpret these events into game specific objects. This is so that there is no limit in what the events can be used.

However, inside _editor_ some event parameters are used for linking events together as this is something most games will need. For example, events can be used to define paths other game objects will follow. The first parameter tells editor the basic type of the event and also enables differently named paremeters for different event types.

The last two parameters called EV\_NEXT and EV\_TRGPARENT are used to link to the next event in the series (think a path with a next waypoint) and also to set the "trigger parent" which could be used to spawn enemies when the player crosses the parent "trigger" type event.

_editor_ will draw an arrow from an event to the event pointed by EV\_NEXT and EV\_TRGPARENT.

These are merely suggestions in that your game can do whatever it likes with the values.

# Parallax #

Parallax is the phenomenon of faraway things moving less than nearfield things when only the point of view changes, i.e. you move and the objects stay still. This creates an illusion of depth.

Parallax scrolling exploits this illusion by scrolling two or more layers at different speeds, lower layers should scroll slower when compared to layers on top. The bottom layer can even stay completely motionless. In klystron, the main factor that tells how "far" a background layer is, is its size.

When parallax is enabled, all parallax layers are considered to be of the same size but at different distances. That is, two layers of size 10x10 and 20x20 will scroll proportionally the same amount. When the larger layer "loops", the smaller will loop at the same moment as well. Using the parallax multipliers will simply multiply the X and Y speed of the scrolling so that can be used to fine tune the scrolling speed.