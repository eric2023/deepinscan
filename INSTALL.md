# DeepinScan 安装指南

本文档提供 DeepinScan 的快速安装指南。

## 🚀 快速安装

### 方式一：使用 deb 包（推荐）

```bash
# 1. 下载预编译包（如果可用）
wget https://github.com/eric2023/deepinscan/releases/download/v1.0.0/deepinscan_1.0.0-1_amd64.deb
wget https://github.com/eric2023/deepinscan/releases/download/v1.0.0/libdeepinscan1_1.0.0-1_amd64.deb

# 2. 安装包
sudo dpkg -i libdeepinscan1_1.0.0-1_amd64.deb deepinscan_1.0.0-1_amd64.deb

# 3. 解决依赖（如果需要）
sudo apt-get install -f
```

### 方式二：从源码构建

```bash
# 1. 安装构建依赖
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config qtbase5-dev \
                     libdtkcore-dev libdtkwidget-dev libusb-1.0-0-dev

# 2. 克隆和构建
git clone https://github.com/eric2023/deepinscan.git
cd deepinscan
./build.sh build

# 3. 安装
sudo make -C build install
```

## 📦 包说明

### 运行时包
- **deepinscan**: 主应用程序
- **libdeepinscan1**: 核心库

### 开发包（可选）
- **libdeepinscan-dev**: 开发头文件和库

## ✅ 验证安装

```bash
# 检查程序是否安装
which deepinscan

# 运行测试
deepinscan_core_test

# 检查库版本
pkg-config --modversion deepinscan
```

## 🗑️ 卸载

```bash
# 卸载 deb 包
sudo dpkg -r deepinscan libdeepinscan-dev libdeepinscan1

# 或从源码构建的安装
sudo make -C build uninstall  # 如果支持
```

## 🔧 故障排除

### 常见问题

**问题**: 找不到 DTK 库
```bash
# 解决方案：安装 DTK 开发包
sudo apt-get install libdtkcore-dev libdtkwidget-dev
```

**问题**: 权限不足
```bash
# 解决方案：添加用户到 scanner 组
sudo usermod -a -G scanner $USER
# 注销并重新登录
```

**问题**: 找不到扫描仪
```bash
# 检查 SANE 配置
sudo sane-find-scanner
scanimage -L
```

## 📞 获取帮助

如果遇到问题，请：
1. 查看 [README.md](README.md) 详细文档
2. 检查 [GitHub Issues](https://github.com/eric2023/deepinscan/issues)
3. 联系开发团队：eric2023@163.com 