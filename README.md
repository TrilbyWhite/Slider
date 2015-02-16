# SLIDER

**Slider** - *PDF presentation tool*

Author: Jesse McClure, Copyright 2012-2015
License: GPLv3

Version 4 is in active development and is a complete rewrite from scratch with
different design and implementation.  Many features are currently incomplete,
some are not implemented at all just yet.  Please consider 4.0 for testing
purposes only.  The 3.0 code has been moved to it's own branch and can continue
to be used for daily use.  As of 16 Feb 2015, the master branch points to 4.0.

## Major changes from v3

- Sorter has been replaced with "coverflow" style previews
- All rendering is done on demand;
	- pro: no delay during startup
	- pro: no need to load all of the pdf into memory
	- con: very large/elaborate pages can take a moment to render on older hardware
- All new approach to user configuration

## Todo

- Multimonitor testing
- Notes windows (currently not implemented at all)
- Configuration (in progress)
- Uncommon links types
- Cursors (other than current circle/dot, low priority)
- History (low priority)
- Track down memory leaks (in cairo?)

