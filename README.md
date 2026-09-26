<div align="center">

<img src="Resources/Icon128.png" width="140" alt="StarRain"/>

# StarRain · 星雨

**让物体自然坠落、碰撞、堆叠 —— Unreal Engine 5 的物理散布与场景布局工具**

[![UE](https://img.shields.io/badge/Unreal%20Engine-5.4%20%7C%205.5%20%7C%205.6%20%7C%205.7%20%7C%205.8-0E1128?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/License-MIT-3DA639)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Win64-0078D4?logo=windows&logoColor=white)](../../releases/latest)
[![Language](https://img.shields.io/badge/%E7%95%8C%E9%9D%A2-%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87-red)](../../releases/latest)
[![Release](https://img.shields.io/github/v/release/Ker0el/StarRain-UE-Plugin?color=blue&label=%E4%B8%8B%E8%BD%BD)](../../releases/latest)

[下载](#安装) · [快速上手](#快速上手) · [快捷键](#快捷键) · [常见问题](#常见问题)

</div>

---

## 这是什么

**StarRain（星雨）** 是一个 UE5 编辑器模式插件，用**真实的物理模拟**来摆放场景物件。

你在场景里"撒"物体，它们会受重力自然坠落、碰撞、堆叠 —— 就像真的把一堆石头倒在地上，而不是一个个手动调整位置和角度。

适合做 **落叶散布、碎石堆、砖块码放、草丛铺地、桌面杂物** 这类"看起来乱、其实有物理逻辑"的场景。

> 和 Foliage（植被工具）的区别：Foliage 是**贴地刷**，物体永远平躺在地表；
> StarRain 是**物理落体**，物体会掉下来、互相碰撞、按真实堆叠角度停住。

---

## 功能

### 三种模式

编辑器模式下拉里选 **StarRain**，工具条上有三个模式：

| 模式 | 做什么 |
|:---|:---|
| **选择** | 单击选中物体，拖拽框选一片，`Ctrl` + 单击取消选择 |
| **调整** | 选中后拖红绿蓝箭头移动 —— 物体**受物理影响**，拖动时会被别的物体挡住、碰到会掉下去 |
| **摆放** | 在场景里喷撒物体，落地后自然堆叠。`Ctrl` + 拖 = 擦除 |

### 面板

左侧面板分成五个区块，从上到下就是你的操作顺序：

| 区块 | 内容 |
|:---|:---|
| **要放置的物体** | 要撒的网格体列表，每种带缩略图 + 权重滑块（按比例随机混着撒）+「只用列表里选中的那一种」 |
| **基础设置** | 物体间距 · 随机程度（一个滑块搞定）· 让物体受重力下落 |
| **一键预设** | 四个常用风格，点一下直接出效果 |
| **高级设置** | 需要精调时才展开的 14 项参数 |
| **对已放置的物体** | 全选 / 设为静态 / 开关重力 / 合并成实例化网格 / 放回原位 |

### 一键预设

| 预设 | 效果 |
|:---|:---|
| **自然散落** | 像落叶：朝向随机，物体之间留点间隔 |
| **整齐堆叠** | 像码砖：完全不随机，只靠重力自然堆 |
| **密集铺满** | 草丛 / 碎石：挨得很近，随机大，适合铺地面 |
| **稀疏点缀** | 偶尔放几个，给场景加少量细节 |

### 高级参数

手动改任意一项会覆盖「随机程度」滑块的设定。

- **离地高度** —— 从命中点沿表面法线再抬高多少，调大可以让物体从高处落下，堆得更自然
- **位置随机 / 旋转随机 / 缩放随机** —— 各有 最小 / 最大 两端，缩放支持 XYZ 等比锁定
- **贴地朝向补偿** —— 按住 `Shift` 对齐表面时额外偏转的角度，比如让草歪一点
- **拖拽时锁住速度** —— 物体不会在拖动过程中乱滚，松手才恢复物理
- **权重按比例联动** —— 调某种物体的权重，其他种自动反向补偿，总和始终 100%
- **把选中的场景物体也纳入物理** —— 让本来不参与物理的场景物件也能被落下的物体撞动

### 定稿（性能）

物理模拟很吃帧数，摆完之后要"定稿"：

1. **合并成实例化网格** —— 把撒出来的物体合并成一个整体，帧数明显变好
2. **设为静态** —— 彻底固定住，不再参与物理运算

---

## 快捷键

| 按键 | 作用 |
|:---|:---|
| `Tab` | 循环切换三个模式 |
| `Shift` + `Tab` | 反向循环 |
| `Q`（按住） | 临时切到「选择」模式，松开自动回到原来模式 |
| `Ctrl` + 拖拽 | 摆放模式下 = 擦除；选择模式下 = 取消选择 |
| `Shift`（对齐时） | 让物体对齐表面，再叠加「贴地朝向补偿」的角度 |

---

## 安装

### 方式一：下载预编译包（推荐，不用装 Visual Studio）

到 [**Releases**](../../releases/latest) 页下载对应你引擎版本的 zip：

| 引擎版本 | 安装包 |
|:---|:---|
| UE 5.4 | `StarRain_UE5.4.zip` |
| UE 5.5 | `StarRain_UE5.5.zip` |
| UE 5.6 | `StarRain_UE5.6.zip` |
| UE 5.7 | `StarRain_UE5.7.zip` |
| UE 5.8 | `StarRain_UE5.8.zip` |

> 其他渠道（如 B 站动态）分发的同名包叫 `StarRain_星雨插件_UE5.x.zip`，内容完全一样。
> GitHub 会自动把 Release 附件名里的非 ASCII 字符替换成 `.`，所以这里用的是纯英文名。

解压后放进你项目的 `Plugins` 目录，最终结构应该是：

```
你的项目/
├── Content/
├── Plugins/
│   └── StarRain/            ← 解压出来就是这个文件夹
│       ├── StarRain.uplugin
│       ├── Binaries/Win64/UnrealEditor-StarRain.dll
│       ├── Resources/
│       └── Source/
└── 你的项目.uproject
```

> 没有 `Plugins` 文件夹就自己建一个。**纯蓝图项目也能用** —— 预编译的二进制不需要你编译任何 C++。

启动编辑器，如果弹出"是否重新编译"的提示，点**否**（包里的二进制就是给这个版本编译的）。

### 方式二：从源码编译

```bash
# 装到你的项目里
cd 你的项目/Plugins
git clone https://github.com/Ker0el/StarRain-UE-Plugin.git StarRain
```

然后**右键 `.uproject` → Generate Visual Studio project files**，用 VS 打开编译 `Development Editor` 配置。

或者用命令行直接编译：

```bat
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" 你的项目Editor Win64 Development -Project="你的项目.uproject" -WaitMutex
```

---

## 快速上手

1. 打开你的项目，把要撒的网格体拖进面板的**「要放置的物体」**列表（可以拖多个，按权重混着撒）
2. 编辑器模式下拉选 **StarRain**，工具条上切到 **「摆放」**
3. 在场景里**按住左键拖动**，物体就撒下去了 —— 它们会自然坠落、碰撞、堆成一堆
4. 不满意？`Q` 切到选择模式框选，拖动调整；或 `Ctrl` + 拖擦除
5. 效果对了就**定稿**：「对已放置的物体」→ **合并成实例化网格**（或「设为静态」）拉帧数

---

## 兼容性

| 引擎版本 | 状态 | 二进制兼容 |
|:---|:---:|:---|
| UE 5.4 | ✅ | 5.4.x 全系列 |
| UE 5.5 | ✅ | 5.5.x 全系列 |
| UE 5.6 | ✅ | 5.6.x 全系列 |
| UE 5.7 | ✅ | 5.7.x 全系列 |
| UE 5.8 | ✅ | 5.8.x 全系列 |

- **平台**：Win64（Windows 10 / 11）
- **模块类型**：Editor 模块（仅编辑器内使用，不会打进打包后的游戏）
- 每个包都按该引擎的 `CompatibleChangelist` 编译，所以**同一大版本内的小版本升级不用重新编译**

---

## 相对原版的改动

本插件基于 [Saeid Gholizade](http://saeidgholizade.ir) 的 **Physical Layout Tool** 修改（原作品仅支持 UE 4.26 – 5.4）。改动如下：

**移植**

- 移植到 UE 5.5 – 5.8
- 补充缺失的头文件包含（`MeshMerge/MeshInstancingSettings.h`）

**修复缺陷**（原版存在的问题）

- 修复撤销后点击「全选已放置」必崩的**空指针**
- 修复切换编辑器模式必崩的**悬空指针**（状态表改用 `TWeakObjectPtr`，由 GC 维护，对象销毁自动置空）
- **移除**进入模式时把全关卡 Actor 强制设为 **Static** 的破坏性操作 —— 原行为会导致编辑器崩溃后整个关卡的 Actor 永久无法拖动
- 修复预设资产的空指针解引用
- 修正旋转随机的**分量顺序错误**

**界面重构**

- 模式从 4 个简化为 3 个（「笔刷选择」并入「选择」）—— 单击选择和拖拽刷选本来就是同一个动作的两种手势，没必要先选模式
- 面板重新组织为 5 个区块，**「要放置的物体」提到最上面**（这是你第一件要干的事）
- 新增**「随机程度」统一滑块**，替代原来要分别调的 6 个 min/max 随机参数
- 新增**四个一键预设**，新手点一下就能出效果
- 原面板其余 14 个参数全部收进**「高级设置」折叠区**，功能一个没删
- 全部界面文案**简体中文本地化**（74 条）

---

## 常见问题

<details>
<summary><b>启动时提示「模块缺失或不兼容，需要重新编译」</b></summary>

zip 里的 DLL 是按**特定引擎版本**编译的，装错版本就会这样。

确认 `Plugins/StarRain/Binaries/Win64/UnrealEditor.modules` 里的 `BuildId` 和你引擎的 `CompatibleChangelist` 对得上：

```json
{
    "BuildId": "43139311",
    "Modules": { "StarRain": "UnrealEditor-StarRain.dll" }
}
```

引擎自己的版本号在 `<引擎目录>/Engine/Build/Build.version` 里。

</details>

<details>
<summary><b>纯蓝图项目能用吗？</b></summary>

能。预编译包就是给纯蓝图项目准备的 —— 不需要 Visual Studio，不需要编译 C++。

</details>

<details>
<summary><b>打包游戏时报错 / 插件被带进包里了？</b></summary>

本插件是 **Editor 模块**，只在编辑器里工作，不应该打进最终游戏包。如果打包报错，在项目设置的 **Packaging → Additional Asset Directories to Cook** 之外，检查 `.uplugin` 是否被手动改过。

</details>

<details>
<summary><b>摆了几百个物体后编辑器很卡</b></summary>

物理模拟的开销。摆完记得定稿：

1. 「对已放置的物体」→ **合并成实例化网格**（「合并选中的」或「合并全部」）
2. 或者 → **设为静态**

定稿后就是普通的 Instanced Static Mesh，性能和你手动摆的没区别。

</details>

<details>
<summary><b>拖动物体时它一直乱滚 / 掉下去</b></summary>

打开高级设置里的 **「拖拽时锁住速度」**。勾上后物体不会在拖动过程中乱滚，松手才恢复物理。

</details>

<details>
<summary><b>我想让物体悬停不掉下来</b></summary>

取消勾选基础设置里的 **「让物体受重力下落」**。物体就悬在你点的地方，不会掉下来堆起来。

</details>

<details>
<summary><b>物体从太高的地方掉下来，堆得不自然</b></summary>

调高级设置里的 **「离地高度」** —— 控制从命中点沿表面法线再抬高多少。

</details>

---

## 署名与许可

本插件是**基于他人作品的修改版本**，遵守原作品的署名要求：

| | |
|:---|:---|
| **原作品** | Physical Layout Tool |
| **原作者** | Saeid Gholizade（[主页](http://saeidgholizade.ir)） |
| **原作品地址** | [Fab 商店](https://www.fab.com/listings/a7fb6fcf-596f-48e9-83cc-f584aea316b1) |
| **原作品许可** | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| **本修改版** | 修改者 **星空**（[B 站](https://space.bilibili.com/177308205)），采用 [MIT](LICENSE) 许可 |

CC BY 4.0 允许复制、分发、修改、演绎，包括商业用途，唯一强制条件是**署名**。

完整的修改说明与原作品署名见 **[NOTICE.txt](NOTICE.txt)**（随插件和每个分发包一起分发）。

> 本修改版按「原样」提供，不附带任何明示或暗示的担保。原作者不对本修改版负责。

---

<div align="center">

**如果这个插件帮到了你，给个 ⭐ 吧**

作者：[星空](https://space.bilibili.com/177308205) · 基于 [Saeid Gholizade](http://saeidgholizade.ir) 的 Physical Layout Tool 修改

</div>
