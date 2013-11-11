# SLIDER

**Slider** - *PDF presentation tool*

Author: Jesse McClure, Copyright 2012, 2013
License: GPLv3

## Synopsis

`slider [<options>] <presentation.pdf> [<notes.pdf>]`

## Description

Coming soon ...

## Options

<dl>
<dt>-c</dt>
	<dd>Continous loop mode.  When the presentation finishes, it will
	loop back to slide number one.</dd>
<dt>-aN</dt>
	<dd>Autoplay mode.  Slides will advance automatically at intervals of
	<code>N</code> seconds.  If N is not specified, a default of 30 is
	used.</dd>
<dt>-sN</dt>
	<dd>Scales the primary view in presenter mode to N times larger than
	secondary views (default=1.8).  This is ignored if you specify any
	geometry strings.  Good values for default geometries range from
	1.75-3 for presentations with separate notes, or 0.5-3 for
	presentations alone.</dd>
<dt>-pN</dt>
	<dd>Prerenders N slides (default=1).  Rendering proceeds faster if
	the presentation is "prerendered", but then you have to wait for it
	to complete before interacting with slider.  For very large
	presentations, it may be useful to prerender many/all slides.  If N
	anything other than a positive integer, all slides are
	prerendered.</dd>
<dt>-mVID1 | -mVID1,VID2</dt>
	<dd>Overrides default monitor detection and shows the presentation on
	output VID1.  The first form disables the presenter mode notes.  The
	second form sends the presentation to VID1 and the notes to
	VID2.</dd>
<dt>-g</dt>
	<dd>Geometry strings for view panes in presenter mode.  Set the
	width, height, and location of up to 4 panels with a notes.pdf or 3
	panels without.  Panels display the following items on the local
	screen:
	
<table style="margin: auto;">
<tr><th>PANEL #</th><th>DISPLAY w/Notes</th><th>DISPLAY w/o</th></tr>
<tr><td>1</td><td>Notes slide</td><td>Current slide</td>
<tr><td>2</td><td>Current slide</td><td>Next slide</td></tr>
<tr><td>3</td><td>Next slide</td><td>Previous slide</td></tr>
<tr><td>4</td><td>Previous slide</td><td>N/A</td></tr>
</table>

	In the absense of geometry strings, what should be a useful default
	is selected based on whether a notes.pdf was provided.  Defaults can
	be adjusted with the -sN scaling factor.</dd>
<dt>presentation.pdf</dt>
	<dd>Required pdf to display</dd>
<dt>notes.pdf</dt>
	<dd>Optional pdf for presenter-mode notes.</dd>


## Controls

This section may not yet be up to date (J. McClure - 11 Nov 2013)

<dt>Ctrl-Q</dt>
<dd>Quit.</dd>
<dt>Tab</dt>
<dd>Overview mode.</dd>
<dt>Return</dt>
<dd>Refresh/Redraw.  Removes any pen/rectangle drawing, and "unzooms".</dd>
<dt>Space</dt>
<dd>Next slide.</dd>
<dt>Right</dt>
<dd>Next slide.</dd>
<dt>Left</dt>
<dd>Previous slide.</dd>
<dt>Up</dt>
<dd>Previous slide in normal mode, or up in overview mode.</dd>
<dt>Down</dt>
<dd>Next slide in normal mode, or down in overview mode.</dd>
<dt>b</dt>
<dd>Mute (black).</dd>
<dt>w</dt>
<dd>Mute (white).</dd>
<dt>z</dt>
<dd>Zoom in.</dd>
<dt>Shift-z</dt>
<dd>Zoom in with a fixed aspect ratio.</dd>
<dt>r</dt>
<dd>Draw a blue/cyan rectangle/box.  Colors, opacity, and line width can be customized in config.h.</dd>
<dt>Shift-r</dt>
<dd>Draw a green rectangle/box.</dd>
<dt>Ctrl-r</dt>
<dd>Draw a red rectanlge/box.</dd>
<dt>1-6</dt>
<dd>Polka-dot cursors of various sizes and colors.  Colors, opacity, and size can be customized in config.h.</dd>
<dt>F1-F6</dt>
<dd>Drawing pens of various sizes and colors.  Colors, opacity, and size can be customized in config.h.</dd>

## Configuration

Slider will look in the following location for a configuration file:

`$XDG_CONFIG_HOME/slider/config`,
`$HOME/.config/slider/config`,
`$HOME/.sliderrc`,
`/usr/share/slider/config`.

Only the first file found will be used.  `/usr/share/slider/config` is
installed with defaults by the Makefile install directive.

## Author

Copyright &copy;2013 Jesse McClure

License GPLv3: GNU GPL version 3: http://gnu.org/licenses/gpl.html

This is free software: you are free to change and redistribute it.

There is NO WARRANTY, to the extent permitted by law. 

Submit bug reports via github: http://github/com/TrilbyWhite/Slider.git

I would like your feedback. If you enjoy *Slider*
see the bottom of the site below for detauls on submitting comments: http://mccluresk9.com/software.html

