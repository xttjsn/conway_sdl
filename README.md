# Conway's Game of Life

Implemented with SDL and QuadTree, tested on OSX.


## Usage

```
$ make
$ echo """
(0, 0)
(1, 0)
(0, 1)
(1, 1)
(10, 0)
(10, 1)
(10, 2)
(11, -1)
(12, -2)
(13, -2)
(11, 3)
(12, 4)
(13, 4)
(14, 1)
(15, -1)
(16, 0)
(16, 1)
(16, 2)
(15, 3)
(17, 1)
(20, 0)
(21, 0)
(20, -1)
(21, -1)
(20, -2)
(21, -2)
(22, -3)
(22, 1)
(24, 1)
(24, 2)
(24, -3)
(24, -4)
(34, -1)
(34, -2)
(35, -1)
(35, -2)
""" | ./game > output.txt

```

After started GUI, press SPACE to start.

You can use the arrow key (Up/Down/Left/Right) to change observation window position.

![screenshot](./screenshot.png)
