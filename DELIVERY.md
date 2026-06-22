# DotConnector 交付说明

## 给朋友的安装包

1. 使用 Visual Studio 2022 打开 `KeyboardClicker.sln`。
2. 选择 `Release|Win32`。
3. 生成解决方案。
4. 在项目根目录运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\BuildInstaller.ps1
```

生成完成后，把下面这个文件发给朋友：

- `dist\DotConnectorSetup.exe`

朋友只需要双击安装包。安装后会自动创建桌面快捷方式和开始菜单快捷方式。

## 安装内容

安装包会自动安装这些运行文件，朋友不需要手动处理：

- `DotConnector.exe`
- `dm.dll`
- `assets\`

## 使用提示

- 第一次点击“高级功能”时需要输入密码。
- 密码验证成功后会保存到当前 Windows 用户配置，之后再次打开不需要重复输入。
- 开始菜单里会有 `Uninstall DotConnector`，可用于卸载安装文件和快捷方式。
