This Repository holds various custom plugins for the Software [Nuke](https://www.foundry.com/products/nuke-family/nuke) from the Foundry.


## Plugins

- Circle create a circle based on a user defined center point. Color and falloff can be adjusted.
- 
- [CircularRamp](https://vimeo.com/640930193) creates a radial falloff on a user defined center. The Falloff itself can be rotated and will always be full frame. It is intended rather as a technical help than directly used as an image.

- CircularRays create rays from a user defined center. The rays can be rotated per needs and the amount can be defned as well. Full umbers get entire rays, while fractions will create, like intented a=one partly cretaed ray.

- CircularRings creates rings from a user defined center. Color and size can be adjusted. 

- [Vibrant](https://vimeo.com/637353122) is a masked saturation operation, which adds colorfullness to less saturated parts of the image while affecting already saturated parts less.


## Pre-compiled binaries
- In the [release](https://github.com/falkhofmann/nuke_plugins/releases) scetion are pre-compiled files for Linux from Nuke 11.3 > 13.1.


## Build yourself
- The batchInstall.sh file should help to build these yourself for Linux or Windows.
- I have set up the building on CentOS 7 with devtoolset-7 and cmake 3.16.x

## Credits 
- The adapted cmake setup and batch files were setup by [Jonas Sorgenfrei](https://github.com/jonassorgenfrei) originally on the [DeepC](https://github.com/charlesangus/DeepC) repository.
