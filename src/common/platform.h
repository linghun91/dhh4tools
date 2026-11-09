/* Win32 编译设置与系统头文件。所有源文件都经由各自的功能头文件最先包含它，
 * 保证 UNICODE 与目标 Windows 版本处处一致。 */
#ifndef DK4_COMMON_PLATFORM_H
#define DK4_COMMON_PLATFORM_H
#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#include <windows.h>
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif
#include <commctrl.h>
#include <commdlg.h>
#include <strsafe.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif
