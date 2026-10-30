# dhh4tools

《大航海时代4 HD》存档修改器源码。窗口程序，读写 `Documents\KoeiTecmo\DK4HD\Savedata` 里的 `savedat00.dk4` 到 `savedat10.dk4`，也可改四个语言版主程序里的等级、属性、技能和港口商业/军事上限。

## 仓库里有什么

| 文件 | 作用 |
| --- | --- |
| `dk4save.c` | 全部源码（单文件，Win32 API） |
| `build.bat` | 构建脚本，优先 MinGW gcc，其次 MSVC |
| `dk4save.rc` | 资源脚本，把清单嵌进 exe |
| `dk4save.manifest` | Common Controls 6、DPI、长路径 |
| `mingw-endfile.specs` | gcc 链接时用的 specs，避免 WinLibs 默认 endfile 冲突 |
| `README.md` | 本说明 |

没有第三方库。链接的是系统自带的 `comctl32`、`comdlg32`、`gdi32`、`user32`。

不进仓库的：`dk4save.exe`（构建产物）、`dk4save.ini`（运行时记下的游戏目录）、`.o` / `.obj` / `.res`（中间文件）。

## 构建

Windows。任选一套编译器：

1. [WinLibs MinGW-w64](https://winlibs.com/)（UCRT），或
   `winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT`
2. Visual Studio，带 MSVC 和 Windows SDK

在本目录运行：

```bat
build.bat
```

成功后会在本目录生成 `dk4save.exe`。改主程序前请完全退出游戏。

## 备份与还原

- 第一次保存存档、第一次写入主程序时，原文件存为 `文件名.bak`，以后不再覆盖。
- 之后每次写入前的文件另存为 `文件名.日期-时间-毫秒.bak`（例如 `DK4HD_sc.exe.20261030-153012-123.bak`），每个文件只留最近 10 份。
- 备份失败时不写入。
- “游戏上限”窗口的“还原原版主程序”用 `DK4HD_*.exe.bak` 把四个主程序还原成第一次写入前的样子；还原前的主程序同样另存一份。存档不受影响。
