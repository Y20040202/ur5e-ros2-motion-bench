# U02–U04 仿真验收记录

实验日期为 2026 年 11 月 26 日。环境为本机 Ubuntu 24.04、ROS 2 Jazzy、MoveIt 2、Gazebo Harmonic 和 UR5e。

## U02 RViz 执行与关节反馈

控制器列表中，`scaled_joint_trajectory_controller`（`joint_trajectory_controller/JointTrajectoryController`）和 `joint_state_broadcaster` 均为 `active`。下表记录了两帧实测关节反馈。独立在 RViz 完成 Plan → Execute 待验收。U04 的 C++ 执行结果见下文。

| 快照 | 仿真时间（s） | `shoulder_pan_joint`（rad） |
| --- | ---: | ---: |
| [运动前](../d06/joints-before.yaml) | 85.224 | 约 0 |
| [运动后](../d06/joints-after.yaml) | 149.030 | 1.3009775200625904 |

运动前后按关节名匹配，`shoulder_pan_joint` 增量约为 `+1.300978 rad`，即 `+74.54°`。两份 YAML 提取自当时上传的终端快照，保留全部关节状态字段。采集器的丢消息提示和文档分隔符已从 YAML 中去除。

## U03 C++ 只规划

在 `execute_motion:=false` 下，`ur5e_bench` 编译通过。一次实测的终端摘录如下。

```text
frame=world eef=tool0
start_rad=1.325668 target_rad=1.525668 delta_rad=0.200000
time taken to generate plan: 0.0139585 seconds
plan_code=1
```

## U04 C++ 执行与末端误差

开启 `execute_motion` 后，两项返回码均为 1。首次 C++ 执行的终端摘录如下。

```text
start_rad=1.325668 target_rad=1.525668 delta_rad=0.200000
plan_code=1
Execute request accepted
Execute request success!
execute_code=1
```

增加正运动学记录后，另一次仿真执行输出了以下末端数据。坐标位于 `world` 下，对应 `tool0`。

```text
goal_xyz_m=-0.283178  0.792898  0.623976
planned_xyz_m=-0.283121  0.792853   0.62408
actual_xyz_m=-0.283099  0.792861   0.62408
plan_goal_error_mm=0.127266
tracking_error_mm=0.0226108
```

规划终点误差是规划末点到请求目标的欧氏距离。执行跟踪误差是反馈终点到规划末点的欧氏距离。所列数值为单次仿真结果，程序用完整精度计算距离。上面两段数据分别摘自执行验证和末端误差验证。

早期日志中的 URDF/SRDF 名称差异和 C++ 客户端 IK 插件提示保留为后续位姿规划的排查线索。

## 复跑

用 [仓库启动步骤](../../README.md#u02-采集第一次运动证据) 启动 Gazebo 与 MoveIt，沿用本机已验证的控制器 YAML。在 Ubuntu 工作区编译并运行。

```bash
source /opt/ros/jazzy/setup.bash
cd "$HOME/ur5e_ws" &&
colcon build --packages-select ur5e_bench --symlink-install &&
source install/setup.bash &&
ros2 run ur5e_bench ur5e_smoke --ros-args \
  -p use_sim_time:=true -p execute_motion:=false
```

只规划通过后，用同一工作区里的节点将 `execute_motion` 切到 `true`，保留 `plan_code`、`execute_code` 和末端误差。节点以每轮当前关节反馈为起点，给规划组的第一个关节增加 `0.20 rad`。对比不同运行结果时先核对起始关节状态。

U05 将复用这个可达用例，再测试不可达目标和碰撞场景。
