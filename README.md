# DotConnector

MFC 键盘连点器，当前整理为单一 Win32 版本，用于继续接入 32 位 Fairy 插件功能。

功能：

- 按 F9 获取鼠标所在窗口/控件句柄。
- 首次启动为空白配置，会在 exe 同目录生成 `DotConnector.ini`。
- 支持动态自动按键列表，可添加/减少和滚动查看，每条单独配置按键与间隔。
- 支持全局随机偏差，例如 1000 毫秒间隔、100 毫秒偏差会在 900~1100 毫秒之间随机触发。
- 支持多条喊话动作，每条可以配置触发键、连续次数、聊天间隔、团队消息、技能前等待、报点后序列和冷却补按。
- 报点后序列支持等待毫秒和 `WHEELUP` / `WHEELDOWN`，例如 `F1,50,F3`。
- 喊话冷却期间再次按对应触发键会补按该行动作配置的“冷却补按”技能键；留空则不补按。
- 支持开始和停止。
- “高级功能”首次启用需要输入密码；Fairy 注册成功后会把密码写入本机配置 AdvancedPassword，以后不需要重复输入。
- “高级功能”按钮会通过项目内置的 `third_party\dm\Fairy.dll` 创建 Fairy 对象并调用 `Reg` 注册。
- “高级功能”注册成功后会初始化 Fairy 资源路径：图片放在 `assets\images\`，字库放在 `assets\dicts\`，默认加载 `assets\dicts\default.txt`。
- 高级页提供号角自动触发：配置 Debuff 范围、号角范围、精准度、Debuff 条件和技能键后，会用 `FindPicEx` 检测图片并自动按键。
- “数字药品”页提供数字 OCR 自动用药：配置数字范围、小绿瓶/大红瓶图片范围、各自图片检测间隔、按键和数字阈值后，勾选“检测数字”即可按配置检测；数字 OCR 使用颜色偏色 `acae91-494A62`。
- 状态提示会同步写入底部调试输出框。

## 开发

- 使用 Visual Studio 2022 打开 `KeyboardClicker.sln`。
- 只保留 `Debug|Win32` 和 `Release|Win32`，不要切 x64。
- 构建后会自动复制 `third_party\dm\Fairy.dll` 到输出目录。
- 构建后会自动复制 `assets\images\` 和 `assets\dicts\` 到输出目录。
- 本机设置保存在运行目录的 `DotConnector.ini`，不要把自己的 ini 一起发给别人。
- 号角功能会动态加载 `assets\images\horn*.bmp`，也会加载 `assets\images\horn\` 下的所有 `.bmp` 样本；可以点“号角样本”按钮导入，不需要重新编译。
- Debuff 基础图片使用 `tie.bmp`、`dizziness.bmp`、`bayonet.bmp`、`collapse.bmp`。
- Debuff 的“添加样本”按钮会把更多 bmp 复制到 `assets\images\tie\`、`assets\images\dizziness\`、`assets\images\bayonet\`、`assets\images\collapse\`，并自动加入识别列表。
- 小绿瓶样本按钮会把 BMP 复制到 `assets\images\small_green_potion\`，大红瓶样本按钮会把 BMP 复制到 `assets\images\big_red_potion\`；构建后这些目录会随资源一起复制到输出目录。
- 数字药品页的启用状态、数字范围、药瓶范围、检测间隔、药品按键和数字阈值都会保存到运行目录的 `DotConnector.ini`。
- Release 输出为 `Win32\Release\DotConnector.exe`。
