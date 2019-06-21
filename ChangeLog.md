# ChangeLog

## [2.0.9](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.9) (06/21/2019)
- Made the IOCTL queue parallel, allowing more terminal program support.
- Partially implemented GET_WAIT_MASK, SET_WAIT_MASK, and WAIT_ON_MASK.

## [2.0.8](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.8) (05/31/2019)
- Fixed a bug that caused reading and writing to be slower than intended.

## [2.0.7](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.7) (04/02/2019)
- Updated to the MIT license.

## [2.0.6](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.6) (02/25/2019)
- Fixed the 'reprogram firmware' IOCTL.
- Updated the copyright years.

## [2.0.5](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.5) (04/27/2018)
- Removed references to nine bit mode. it is not currently supported. References in the driver are retained for backwards compatibility purposes only.
- Changed IntervalTimeout to immediately occur, increasing compatibility with terminal programs.
- Change /inc to /lib/raw to be more in line with other Commtech repositories.

## [2.0.4](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.4) (09/01/2017)
- Finished implementing isochronous.
- Modified the naming to hopefully resolve some character size issues.

## [2.0.3](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.3) (07/03/2017)
- Modified the drivers to utilize 10.0 Driver Framework for Windows 10, and utilize non-executable memory.
- Some documentation tweaks.
- Fixed the build configurations.
- Small changes to the INF.

## [2.0.2](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.2) (04/19/2017)
- A special release for our new Vendor ID. All releases going forward use this new ID, 0x2EB0.

## [2.0.1](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.1) (02/08/2017)
- Fixed supporting multiple cards.

## [2.0.0](https://github.com/commtech/asynccom-windows/releases/tag/v2.0.0) (01/23/2017)
- Added COM compatibility to the drivers. Not backwards compatible with previous software.

## [1.0.1](https://github.com/commtech/asynccom-windows/releases/tag/v1.0.1) (01/17/2016)
- Fixed some errors in the .INX for Windows 10.

## [1.0.0](https://github.com/commtech/asynccom-windows/releases/tag/v1.0.0) (09/26/2016)
This is the initial release of the 1.0 driver series.