
# 🤖 RAICOM CAIR 2026 — 工程竞技赛道

> 睿抗机器人开发者大赛 | CAIR 工程竞技赛道  
> 团队：**6++（SLAM）** · 嵌入式同学 · 视觉zzp  
> 状态：⚡ 备赛中

---

## 🎯 比赛任务

机器人自主识别场地中的**物块**，将其搬运至**指定位置**。

```
摄像头识别物块 → YOLO 检测 → 定位物块坐标 →
SLAM 建图导航 → 开到物块前 → 机械臂抓取 →
导航到目标区域 → 放置物块
```

---

## 🧠 技术栈

| 模块 | 技术方案 | 负责 |
|------|---------|------|
| 🗺️ SLAM + 导航 | ROS2 + Cartographer | 6++ |
| 👁️ 物块识别 | YOLOv8 | 视觉同学zzp |
| 🦾 底层控制 | ESP32 + STM32 | 嵌入式同学 |
| 🧠 大脑（板载电脑） | 待定 | 团队 |

---

## 📁 项目结构

```
raicom-cair-2026/
├── slam/              # SLAM 建图 + 导航代码
│   ├── cartographer/  # Cartographer 配置与启动
│   ├── nav/           # 导航、路径规划
│   └── maps/          # 建好的地图文件
├── vision/            # YOLO 视觉识别
│   ├── dataset/       # 训练数据集（物块照片）
│   ├── train/         # 训练脚本
│   └── deploy/        # 部署推理代码
├── control/           # ESP32 + STM32 底层控制
│   ├── esp32/         # ESP32 固件
│   ├── stm32/         # STM32 固件（机械臂+轮子）
│   └── protocol/      # 串口通信协议定义
├── docs/              # 比赛文档、场地示意图
└── README.md          # 本文件
```

---

## 🚀 开发路线

| 阶段 | 时间 | 里程碑 |
|------|------|--------|
| 🧱 环境搭建 | Week 1 | 大脑就绪，ROS2 跑通，串口通信打通 |
| 🔧 模块开发 | Week 2 | SLAM 建图 + YOLO 识别 + 机械臂控制 |
| 🔗 系统集成 | Week 3 | 全流程串联：识别→导航→抓取→放置 |
| 🎯 冲刺联调 | Week 4 | 跑完整流程，录演示视频，查缺补漏 |

---

## 👥 团队成员

| 角色 | 姓名 | 职责 |
|------|------|------|
| 🗺️ SLAM | 6++ | ROS2 + SLAM 建图 + 导航规划 |
| ⚙️ 嵌入式 | TBD | ESP32/STM32 底层控制 + 机械臂 |
| 👁️ 视觉 | TBD | YOLO 模型训练 + 部署推理 |

---

## 📌 快速开始

```bash
# 克隆仓库
git clone https://github.com/Liuuu-J1028/raicom-cair-2026.git
cd raicom-cair-2026

# 各模块详细说明见对应目录下的 README
```

_各模块的详细搭建教程将在开发过程中陆续更新。_

---

## 📄 参考

- [RAICOM 睿抗机器人开发者大赛官网](https://www.raicom.com.cn)
- [Cartographer 官方文档](https://google-cartographer-ros.readthedocs.io)
- [Ultralytics YOLOv8](https://docs.ultralytics.com)

---

> 🚗 **致自己：** 大一零基础开局，一个月从无到有。不论结果如何，冲！
