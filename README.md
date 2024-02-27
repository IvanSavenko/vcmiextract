## vcmiextract
Extractor of Heroes 3 data files based on VCMI source code

Primary made for my own use, so nothing fancy

## Supported formats
Archives - all files will be extracted as it, with exception of images which will be converted to png
- .lod
- .snd
- .vid

Animations - will be extracted as set of png files and additional text file that contains order of images in .def file
- .def
- .d32: used by HotA

Images - will be converted to .png format
- .pcx: custom format of Heroes 3, not related to well-known pcx format
- .p32: used by HotA

## Usage - Windows

Drag-and-drop file(s) that you want to extract on executable. Extracted files will be placed in a directory with same name as input file

## Usage - Command line

```
./vcmiextract [archive.lod]...
./vcmiextract [animation.def]...
```