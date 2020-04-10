# TV-Series-Processing-Tool

*TV Series Processing Tool* is a small software I wrote in C++/Qt during my PhD to annotate, visualize and process data related to three TV shows: *Breaking Bad*, *Game of Thrones* and *House of Cards*.

As shown on the folllowing screenshot, the software includes a basic video playing interface, along with a set of tools to visualize and automatically extract information from the video stream: shot boundaries, speaker clustering (disabled in this public version), interacting speakers... *TV Series Processing Tool* also provides the users with basic annotation capabilities: speaker turns, interlocutors, shot and scene boundaries, recurring shots.
<br />
<br />
<p align="center">
<img src="./pictures/screenshot.png" data-canonical-src="TV Series Processing Tool">
</p>
<br />
<br />

# Installation

## Dependencies

The script **scripts/install_dependencies.sh** will install all the dependencies of *TV Series Processing Tool* (tested under Ubuntu 18.04), except [*IBM/Cplex*](https://www.ibm.com/analytics/cplex-optimizer), a proprietary solver you need to install manually. Free versions are available for academics and students.

```
bash scripts/install_dependencies.sh
```

## Compilation

```
qmake
make
```

# Project files

The subdirectory *projects* contains 3 project files, one for each TV show. For obvious copyright reasons, the textual content of the speech turns is encrypted in these files.

In order to use the files you first need to generate the video files (.avi or .mp4) with the expected dimensions, and to edit accordingly the 'fName' key in the .json project files:

1. *Breaking Bad*: 480x260 (62 episodes).
2. *Game of Thrones*: 480x270 (73 episodes).
3. *House of Cards*: 480x240 (first 26 episodes).

# Use

```
./bin/"TV Series Processing Tool"
```

Once you have launched the software, you need to open one of the three project files located in the subdirectory *projects* (**Project>Open**).
