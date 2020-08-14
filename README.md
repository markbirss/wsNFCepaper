Waveshare NFC E-ink upload tool
===============================

Write image to [Waveshare NFC-Powered e-Paper](https://www.waveshare.com/4.2inch-nfc-powered-e-paper.htm).
You will need [LibNFC](https://github.com/nfc-tools/libnfc) and [MagickWand](https://imagemagick.org/script/magick-wand.php) to build/run.

Build :

```
$ mkdir build
$ cd build
$ cmake ../
$ make
```

You can also build/install a Debian package :

```
$ make package
$ sudo dpkg -i epnfcup-0.0.1-Linux.deb
```

Equivalent tool by Eric Betts, written in JS available here : [https://gitlab.com/bettse/wne_writer](https://gitlab.com/bettse/wne_writer).
