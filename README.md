# sust - sustained habit tracker

> Don't be sus, do your tasks.

A super simple habit tracker written in C.
Designed to be much more minimal than the already minimal `habitctl` and `harsh`.

Mostly an exercise in file processing.

## Features

 - suckless-style config.h (hackable!)
 - configurable column size (for small terminals!)
 - month-based overview log (keep in time with the seasons!)
 - compatible-ish with habitctl files
   (i.e. tsv but ignore empty lines and lines that start with "#")

## Non-Features

 - Unicode awareness. ASCII or UTF-8 is assumed.
 - support for anything that isn't a UNIX flavour
