synthesizer
===========

As result of playing around with sound-synthesis, I created this simple synthesizer.

One of the requirements I set to myself, was to avoid any allocation of dynamic memory,
but building a tree-structure of instrument-definitions at compile-time.
This, combined with the demand to keep it modular and extensible, lead to a scary
amount of macros -- but, well, it works, and code using it is quite readable.
While debugging it is a pain, nonetheless.
