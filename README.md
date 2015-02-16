# SLIDER

**Slider** - *PDF presentation tool*

Author: Jesse McClure, Copyright 2012-2015
License: GPLv3

v4 branch: version 4 is 'pre-alpha' and for previewing purposes only.  This is a
complete rewrite from scratch with different design and implementation.

## Major changes from v3

- Sorter has been replaced with "coverflow" style previews
- All rendering is done on demand;
	- pro: no delay during startup
	- pro: no need to load all of the pdf into memory
	- con: very large/elaborate pages can take a moment to render on older hardware
- All new approach to user configuration

## Todo

- Configuration (in progress)
- Uncommon links types
- Cursors (other than current circle/dot, low priority)
- History (low priority)
- Track down memory leaks (in cairo?)

