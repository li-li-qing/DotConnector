# DotConnector

MFC 键盘连点器，当前整理为单一 Win32 版本，用于继续接入 32 位大漠插件功能。

功能：

- 按 F9 获取鼠标所在窗口/控件句柄。
- 支持动态自动按键列表，可添加/减少和滚动查看，每条单独配置按键与间隔。
- 支持全局随机偏差，例如 1000 毫秒间隔、100 毫秒偏差会在 900~1100 毫秒之间随机触发。
- 支持多条喊话动作，每条可以配置触发键、连续次数、聊天间隔、团队消息、技能前等待、报点后序列和冷却补按。
- 默认包含 F2 拉拽喊话、F3 敌人来了、F4 第三条喊话；新增动作可通过滚动列表查看。
- 报点后序列支持等待毫秒和 `WHEELUP` / `WHEELDOWN`，例如 `F1,50,F3`。
- 喊话冷却期间再次按对应触发键会补按该行动作配置的“冷却补按”技能键；留空则不补按。
- 支持开始和停止。
- “高级功能”首次启用需要输入密码；验证成功后会保存到本机配置，以后不需要重复输入。
- “高级功能”按钮会通过项目内置的 `third_party\dm\dm.dll` 创建大漠对象并调用 `Reg` 注册。
- “高级功能”注册成功后会初始化大漠资源路径：图片放在 `assets\images\`，字库放在 `assets\dicts\`，默认加载 `assets\dicts\default.txt`。
- 高级页提供号角自动触发：配置 Debuff 范围、号角范围、精准度、Debuff 条件和技能键后，会用 `FindPicEx` 检测图片并自动按键。
- 状态提示会同步写入底部调试输出框。

## 开发

- 使用 Visual Studio 2022 打开 `KeyboardClicker.sln`。
- 只保留 `Debug|Win32` 和 `Release|Win32`，不要切 x64。
- 构建后会自动复制 `third_party\dm\dm.dll` 到输出目录。
- 构建后会自动复制 `assets\images\` 和 `assets\dicts\` 到输出目录。
- 号角功能会动态加载 `assets\images\horn*.bmp`，也会加载 `assets\images\horn\` 下的所有 `.bmp` 样本；可以点“号角样本”按钮导入，不需要重新编译。
- Debuff 基础图片使用 `tie.bmp`、`dizziness.bmp`、`bayonet.bmp`、`collapse.bmp`。
- Debuff 的“添加样本”按钮会把更多 bmp 复制到 `assets\images\tie\`、`assets\images\dizziness\`、`assets\images\bayonet\`、`assets\images\collapse\`，并自动加入识别列表。
- Release 输出为 `Win32\Release\DotConnector.exe`。
- 交付朋友使用时，先构建 `Release|Win32`，再运行 `scripts\BuildInstaller.ps1` 生成 `dist\DotConnectorSetup.exe` 安装包。
