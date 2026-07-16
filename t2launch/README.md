# t2launch - _Tribes 2_ launcher

A long time ago [...], in April 2001, [Loki Entertainment](https://en.wikipedia.org/wiki/Loki_Entertainment)
released a native Linux version of _[Tribes 2](https://en.wikipedia.org/wiki/Tribes_2)_.

Surprisingly, it is still possible to enjoy this game (among others)
on modern Linux based systems, thanks to efforts of some
[people](https://github.com/twolife/tribes2-linux-sdl).

This Linux version came with a small launcher that allowed the player to choose
between "Online Play" and "Local/Lan" mode.
That launcher was provided as a statically linked binary, that didn't age well
and is virtually unusable today; due to changes in how alpha-transparency
is handled between X protocol / server versions.

This is a rewrite of that `t2launch` launcher, in python3. 

![Obligatory screenshot](https://raw.githubusercontent.com/twolife/tribes2-linux-sdl/main/t2launch/screenshot.png)
