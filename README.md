# TopiPal

macOS 原生桌面宠物。当前方向是常驻顶部或用户拖动位置的轻量宠物窗口，不再做通用灵动岛模块。

当前已包含：

- AppKit `NSPanel` 悬浮窗口
- SwiftUI 宠物界面
- 顶部中心锚点展开/收起动画
- 拖动位置保存
- 随机气泡消息
- 点击不同区域触发资源动作
- Spine 3.x / 4.0 资源渲染
- Live2D Cubism Core 资源加载
- 菜单栏显示/隐藏、展开/收起、切换人物
- 资源驱动动作列表的基础扫描
- `.app` 打包脚本和本地签名支持

## 资源支持方向

- 序列帧：按动作文件夹扫描 PNG 帧。
- Spine：读取 `.skel + .atlas + .png`，内置 Spine 3.x 和 4.0 C runtime 桥接。
- Live2D：读取 `model3.json / moc3 / motions / expressions`，通过 Cubism Core 桥接加载模型。

## 构建

```bash
swift build
```

构建产物是 SwiftPM 可执行文件，不会自动出现在 macOS 的“应用程序”目录。

## 运行

```bash
swift run TopiPal
```

应用以 accessory 模式启动，不显示 Dock 图标，通过菜单栏图标管理和退出。

## 打包为 App

生成 `.app` 包：

```bash
Tools/package_app.sh debug
```

输出位置：

```text
.build/TopiPal.app
```

安装到当前用户的“应用程序”目录：

```bash
Tools/package_app.sh debug --install
```

输出位置：

```text
~/Applications/TopiPal.app
```

安装到系统“应用程序”目录：

```bash
Tools/package_app.sh debug --install-system
```

输出位置：

```text
/Applications/TopiPal.app
```

`--install-system` 需要有权限写入 `/Applications`。脚本会优先使用名为 `TopiPal Local Code Signing` 的本地代码签名证书；如果不存在，会退回 ad-hoc 签名。
