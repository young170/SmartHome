#ifndef _APP_VERSION_H_
#define _APP_VERSION_H_

/*  values come from cmake/version.cmake
 * BUILD_VERSION related  values will be 'git describe',
 * alternatively user defined BUILD_VERSION.
 */

/* #undef ZEPHYR_VERSION_CODE */
/* #undef ZEPHYR_VERSION */

#define APPVERSION          0x2050000
#define APP_VERSION_NUMBER  0x20500
#define APP_VERSION_MAJOR   2
#define APP_VERSION_MINOR   5
#define APP_PATCHLEVEL      0
#define APP_VERSION_STRING  "2.5.0"

#define APP_BUILD_VERSION 8594851ab5d0


#endif /* _APP_VERSION_H_ */
