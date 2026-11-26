# UR5e · ROS 2 运动规划与轨迹执行测试

使用 Ubuntu 24.04、ROS 2 Jazzy、Gazebo Harmonic、MoveIt 2 与 ros2_control，验证 UR5e 从 C++ 目标请求、运动规划到仿真执行及反馈核对的流程。

## 当前进度

UR5e 已进入 MoveIt 2 启动阶段，仿真时间标志为真。原默认轨迹控制器配置失败，U02 的有效运动和关节变化等待本机复测。四天计划包含两类末端误差、40 次规划器配对请求、一次备选重试和 Pilz LIN 的通行及阻挡用例。

- [四天实施与验收计划](docs/implementation.md)
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

`ur5e_bench/` 内已有 `ur5e_smoke`，默认只规划一个小幅关节目标。先验收 U02，再按[编译和执行步骤](docs/implementation.md#u03-编译并验证只规划)运行。
