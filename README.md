# Hard Reference Finder

An open-source editor plugin for Unreal Engine 5 that identifies hard references in a blueprint graph.  

The plugin allows you to summon a window which links to the various function calls, variables, graph pins, etc that are causing hard package references to other assets. Results are grouped by package and sorted by size, from largest to smallest.

Built on UE 5.1 and backwards compatible with UE 4.27.

![Image showing plugin usage in an example blueprint](Documentation/main-image.png)


# Installation

 - Download the zip and unpack it (or clone this repository) to your projects *Plugins* folder.
 - Build the game with the plugin.
 - If necessary, enable the plugin from the plugins windows.

# Usage

Open any blueprint with a graph or function view, then select *Window -> Hard References* from the toolbar.

![Image showing how to summon the hard references viewport](Documentation/usage-guide.png)


# Known Issues
 - After modifying a blueprint, you have to compile/save it before 'Refresh' will display the updated list of references.
 - Doesn't identify sources in actor components 
 - Doesn't identify sources from local variables in function graphs
 - Needs more testing! This is an initial version, bug reports/pull requests appreciated.
