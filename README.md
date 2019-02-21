# dsiflipencode
now without a misleading project name

before the copyright police murder me, this project uses https://github.com/lvandeve/lodepng, https://github.com/jtilly/inih, and parts of https://github.com/FFmpeg/FFmpeg

## usage
you will need ffmpeg and imagemagick


first, dump the frames of the video you're going to convert
```
ffmpeg -i input.mp4 -vf scale=256:192 frame_%d.png
```
ffmpeg doesn't create a frame_0.bmp, so remember to put something there (watermark, copy of first frame, etc)

for most videos (other than something like bad apple), you'll have to use dithering
```
magick mogrify -format png -colors 2 -type bilevel *.png
```
to prepare the audio data, run this command
```
ffmpeg -i input.mp4 -ac 1 -ar 8192 audio.wav
```
of course, it's possible to have a different frame speed and bgm frame speed for better audio, but that's an exercise for the reader

a `thumbnail.bin` can be grabbed from a flipnote by copying 0x600 bytes at 0xA0, but making a 0x600 byte file filled with zeros works too

`config.ini` stores various metadata for the generated flipnote. here's a commentated example
```
author=Khang                      # utf-8 (sorry people who don't speak english) string (11 character limit)
filename=116AE34C2880B000         # the last 2 parts of a flipnote filename, without the _
fsid=5473EB00A0BC70FA             # the flipnote studio id that shows in your flipnote studio settings
partial_filename=BC70FA116AE34C28 # dump 8 bytes at 0x92 from a real flipnote
use_bgm=true                      # i don't think i need to explain this
frame_speed=8                     # see https://github.com/Flipnote-Collective/flipnote-studio-docs/wiki/PPM-format
bgm_frame_speed=8                 # ^
timestamp=1550776227              # current unix timestamp
```
finally, run the executable on the directory where all of these files are stored
```
./dsiflipencode /path/to/files
```
most flipnotes encoded with this program should run fine on a custom flipnote player, such as https://flipnote.rakujira.jp/

getting an encoded flipnote to run on a real dsi/3ds takes a bit more work though

1. it needs to be signed with the correct private key (this program will not sign flipnotes for you, nor does it have the ability to)
2. the flipnote will not load if it is too big (the program tries to point this out, but it might still load anyway)

it's also worth noting that flipnotes that won't load from the sd card might load from flipnote hatena

an example data directory (the entire bad apple music video) is available at https://my.mixtape.moe/jwpkte.7z

**THIS WILL NOT LOAD ON A REAL DSI/3DS, SIGNED OR NOT, DUE TO HOW BIG IT IS**


discord: `Khangaroo#5062`

twitter: `@Khangarood`
