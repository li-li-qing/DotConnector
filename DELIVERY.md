# DotConnector 交付说明

## 构建

1. 使用 Visual Studio 2022 打开 `KeyboardClicker.sln`。
2. 选择 `Release|Win32`。
3. 生成解决方案。

## 交付文件

构建成功后，把下面内容放在同一个文件夹里交给朋友：

- `Win32\Release\DotConnector.exe`
- `Win32\Release\dm.dll`
- `Win32\Release\assets\`

## 使用提示

- 第一次点击“高级功能”时需要输入密码。
- 密码验证成功后会保存到当前 Windows 用户配置，之后再次打开不需要重复输入。
- 如果要重新触发密码验证，可以删除该 Windows 用户下 DotConnector 的应用配置。
