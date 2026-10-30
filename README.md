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

## 地方舰队亏损修正

地方舰队和下属 NPC 舰队自动买货时，成本按 `货价 × (1000 − 船长交易能力 + 随机 0~99) ÷ 1000` 计算。原版能力最高 500，所以最多打五折。属性结算上限放开后能力会超过 1000，括号里变成负数，游戏再按无符号数相除，一次买货就记几百万支出，月报显示亏损上亿。

“写入主程序”每次都会顺带修正：这一处的能力先按 500 封顶再算，等于原版满能力。别处的能力值不受影响。四个语言版都用字节模式定位，补丁代码放在该函数结尾后的空位里。

已经亏出的欠账还在存档里：付不起的亏损会转到下个月支出，修正只能阻止新的亏损。最好读修正前、出问题之前的存档；或者解散该舰队，代价是身上的钱可能被扣光。
