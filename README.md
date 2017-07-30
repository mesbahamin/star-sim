![Gif of lazily wandering stars](https://cloud.githubusercontent.com/assets/5744114/24231758/af94bcbe-0f43-11e7-83ef-9e937b498c78.gif)

This is a simulation of gravitational attraction between lots of 'stars'.

The [Barnes-Hut Algorithm](http://arborjs.org/docs/barnes-hut) is used to
efficiently calculate the forces acting on each star.

![Gif of recursive partition grid](https://cloud.githubusercontent.com/assets/5744114/25370870/6c51d2e8-2942-11e7-8a0b-929a185b768c.gif)


### Keyboard Shortcuts

- `b`: Toggle brute-force mode. Stars turn blue when this mode is active, and
  gravitational attraction is calculated for every single pair of stars. The
  Barnes-Hut grid is not updated.
- `g`: Toggle Barnes-Hut grid rendering.
- `p`: Pause/unpause simulation.
- `t`: Toggle star trail rendering.


### Dependencies

- [SDL 2.0.5](https://www.libsdl.org/download-2.0.php)
