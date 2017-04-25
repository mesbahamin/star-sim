![Gif of lazily wandering stars](https://cloud.githubusercontent.com/assets/5744114/24231758/af94bcbe-0f43-11e7-83ef-9e937b498c78.gif)

### Barnes Hut Simulation

I'm currently working on implementing the [Barnes-Hut Algorithm](https://en.wikipedia.org/wiki/Barnes%E2%80%93Hut_simulation).

I've made a quadtree that I use to recursively partition the simulation space.

![Gif of recursive partition grid](https://cloud.githubusercontent.com/assets/5744114/25370870/6c51d2e8-2942-11e7-8a0b-929a185b768c.gif)

Now I just have to use that tree to simplify the gravitational attraction
calculations between distant groups of stars.

Dependencies
------------
- [SDL 2.0.5](https://www.libsdl.org/download-2.0.php)
