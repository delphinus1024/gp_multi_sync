# gp_multi_sync

Control multiple DSLR cameras to do synchronized interval capture via USB.

## Description

This is a program that controls multiple DSLR cameras to do synchronized interval capture via USB, will be useful to time lapse in panorama or stereogram format, so on.
The interval time can be specified by command option.
For ease of post processing, list of image file names of all cameras are written to cvs log file, each row of cvs represents captured images at same time slice. 
Since the filename of the cvs log file is automatically assigned according to current date&time, so that the cvs log file won't overwritten even if you launch this program multiple times.

NOTE that this program may need modification depend on your camera, lens, external storage and version of libgphoto2.

This program uses libgphoto2 to interface to DSLR.
For more details of gphoto2 and libgphoto2, refer to.

[http://gphoto.sourceforge.net/](http://gphoto.sourceforge.net/)

## Requirement

- Linux PC or Linux board. (I checked with Beagle Bone Green + pre installed Debian)
- libgphoto2 (I checked with version 2.5.7. Newer version than this, somehow, did't work correctly under my environment.)
- gcc (I checked with (Debian 4.6.3-14) 4.6.3)
- DSLR supported by libgphoto2. (I checked with EOS 5D2 & 5D3, maximum two cameras.)

## Usage

- Connect between Linux and your camera(s) with USB cable.
- Power on your camera(s).
- lsusb to see your camera(s) is surely connected to the computer.
- Open command prompt.
- $./gp_multi_sync [interval (sec)]
- Then capture sequence will start.
- Hit ESC to terminate. (it'll take a while to exit when capture is going on.)
- cvs log file is written to the specified directory as follows. Each row shows image files of each cameras at the same time slice.

/store_00010001/DCIM/100EOS5D,7S4A1467.CR2,/store_00010001/DCIM/100EOS5D,IMG_3825.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1468.CR2,/store_00010001/DCIM/100EOS5D,IMG_3826.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1469.CR2,/store_00010001/DCIM/100EOS5D,IMG_3827.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1470.CR2,/store_00010001/DCIM/100EOS5D,IMG_3828.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1471.CR2,/store_00010001/DCIM/100EOS5D,IMG_3829.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1472.CR2,/store_00010001/DCIM/100EOS5D,IMG_3830.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1473.CR2,/store_00010001/DCIM/100EOS5D,IMG_3831.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1474.CR2,/store_00010001/DCIM/100EOS5D,IMG_3832.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1475.CR2,/store_00010001/DCIM/100EOS5D,IMG_3833.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1476.CR2,/store_00010001/DCIM/100EOS5D,IMG_3834.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1477.CR2,/store_00010001/DCIM/100EOS5D,IMG_3835.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1478.CR2,/store_00010001/DCIM/100EOS5D,IMG_3836.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1479.CR2,/store_00010001/DCIM/100EOS5D,IMG_3837.CR2,
/store_00010001/DCIM/100EOS5D,7S4A1480.CR2,/store_00010001/DCIM/100EOS5D,IMG_3838.CR2,
.
.
.

- NOTE: The interval time must be longer than the shutter speed.
- NOTE: Be careful the program fails if your camera(s) are under the condition of auto power off.

## Build

- Modify Makefile according to your libgphoto2 inludes and libs environment.
- Modify "sdroot" variable in gp_multi_sync.cpp so that you can change path of cvs log file. (Usually, assigning this path to external storage will be convenient.)
- If you don't need cvs logging, undefine DO_LOG in gp_multi_sync.cpp.
- If you control more than four cameras, change "MAX_CAMERAS" definition in gp_multi_sync.cpp.
- make

## Author

delphinus1024

## License

[MIT](https://raw.githubusercontent.com/delphinus1024/opencv30hdr/master/LICENSE.txt)
