# TopiPal

macOS 原生桌面宠物。当前方向是常驻顶部或用户拖动位置的轻量宠物窗口，不再做通用灵动岛模块。

当前已包含：

- AppKit `NSPanel` 悬浮窗口
- SwiftUI 宠物界面
- 顶部中心锚点展开/收起动画
- 拖动位置保存
- 随机气泡消息
- 点击不同区域触发资源动作
- 4 套 Seele Spine 资源打包
- 菜单栏显示/隐藏、展开/收起、切换人物
- 资源驱动动作列表的基础扫描

## 资源支持方向

- 序列帧：按动作文件夹扫描 PNG 帧。
- Spine：读取 `.skel + .atlas + .png`，当前已打包资源并能扫描候选动作；真实骨骼渲染需要接 Spine 4.1 runtime。
- Live2D：计划读取 `model3.json / moc3 / motions / expressions`；真实渲染需要接 Cubism Native SDK。

## 构建

```bash
swift build
```

## 运行

```bash
swift run TopiPal
```

应用以 accessory 模式启动，不显示 Dock 图标，通过菜单栏图标管理和退出。
