# 基于 ROS 2 与 Gazebo 的 UR5e 运动规划与视觉分拣工站

使用 Ubuntu 24.04、ROS 2 Jazzy、Gazebo Harmonic、MoveIt 2、ros2_control 和 C++17，搭建 UR5e 仿真工站。项目先验证运动规划、轨迹执行和故障拦截，再扩展到按工件 ID 分拣的工站。分拣任务以方块离桌、搬运和落入目标料箱后停稳为验收依据。

## U01–U09 · 运动规划与执行基线

| 阶段 | 状态 | 验收证据 |
| --- | --- | --- |
| 1 · 仿真启动与首次执行（U01–U02） | **已验收** | 在 RViz 手动完成 Plan → Execute，并在 Gazebo 观察到机械臂运动。已归档两帧关节反馈，`shoulder_pan_joint` 增加约 `1.300978 rad`。[U02 验收记录](evidence/u03-u04/README.md#u02-rviz-执行与关节反馈) |
| 2 · C++ 规划、执行与工位用例（U03–U05） | **U03–U04 已验收，U05 绕障规划已验收** | 单次仿真规划终点误差 `0.127266 mm`，跟踪误差 `0.0226108 mm`。[U02–U04 验收记录](evidence/u03-u04/README.md)。RViz 中记录了盒口挡板导致的规划失败。C++ 用例验证了旧轨迹中段受阻，并在同一 B 盒目标下复跑规划成功。[U05 实测记录](evidence/u05/README.md)。待补不可达目标及轨迹未下发记录。 |
| 3 · 双规划器配对对照（U06） | 待开展 | 固定起终点、碰撞场景和时间预算，对比 RRTConnect 与 RRT*，保存逐试次 CSV。 |
| 4 · 直线接近、轨迹复核与复跑（U07–U09） | 待开展 | Pilz LIN 通行及阻挡用例、执行前新增障碍后的复核、一次备选重试及复跑记录 |

## V01–V04 · 分拣工站扩展

| 阶段 | 状态 | 验收证据 |
| --- | --- | --- |
| V01 · 夹爪与固定坐标分拣 | 待开展 | 夹爪开合反馈。使用已知工件位姿完成抓取、搬运和释放，记录工件是否落入箱内。 |
| V02 · AprilTag 视觉定位与分流 | 待开展 | 固定顶视相机及 TF。记录三个来料位置的定位误差，用两个标签 ID 分配 A/B 料箱。 |
| V03 · C++ 任务状态机与故障实验 | 待开展 | 一次视觉引导分拣，以及标签丢失、夹空和执行前新增挡板的日志。在线判断使用传感器与控制器反馈，评测脚本使用 Gazebo 真值。 |
| V04 · RGB-D 点云与 OctoMap | 可选扩展 | 可视化过滤后的点云，使用点云观察到的障碍复现执行前轨迹拦截。 |

按阶段归档源码、配置和实测证据。阶段状态随验收结果更新。

- [实施与验收计划](docs/implementation.md)
- [面试准备](docs/interview.md)
- [逐试次 CSV 模板](evidence/episodes.template.csv)

## U02 采集第一次运动证据

在 Ubuntu 启动 UR5e 仿真。若使用前一步生成的 Gazebo 专用控制器配置，使用下列启动命令。

```bash
source /opt/ros/jazzy/setup.bash
cfg="$HOME/ur5e_ws/config/ur5e-gazebo-controllers.yaml"
ros2 launch ur_simulation_gz ur_sim_moveit.launch.py ur_type:=ur5e controllers_file:="$cfg"
```

在项目仓库的第二个终端执行：

```bash
source /opt/ros/jazzy/setup.bash
bash scripts/collect_u02.sh before
```

等轨迹控制器处于 `active` 后在 RViz 做一次小幅 **Plan → Execute**，观察 Gazebo 里的机械臂。停稳后记录反馈并按关节名称比较。

```bash
bash scripts/collect_u02.sh after
diff -u evidence/d06/joints-before.yaml evidence/d06/joints-after.yaml
```

## U03 C++ 起步节点

`ur5e_smoke` 默认只规划关节目标。启用 `execute_motion` 后，节点请求执行并由执行后的关节反馈计算 `tool0` 的位置。可按[编译和执行步骤](docs/implementation.md#u03-编译并验证只规划)运行，并对照[实测记录](evidence/u03-u04/README.md)。
