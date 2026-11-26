# 项目 2 · 基于 ROS 2 与 Gazebo 的 UR5e 运动规划与视觉分拣工站

计划于 2026 年 11 月 26 日提前开工，四个工作日，每天约 3–4 小时。使用本机 Ubuntu 24.04 图形桌面、ROS 2 Jazzy、Gazebo Harmonic、MoveIt 2、ros2_control 和 C++17。11 月 30 日至 12 月 2 日留给环境故障和复跑。表中次数是实验目标，成果以实测记录为准。

先完成 U01–U09 的规划与执行基线。U05 开始把手工搭建的最小工位用于碰撞实验。通过 U09 后，按 V01–V04 逐项接入夹爪、相机、任务状态机和点云。

## 四天安排

| 日期 | 工作阶段 | 当天验收 | Git 提交建议 |
| --- | --- | --- | --- |
| 11/26 · 第一天 | U01 官方启动，U02 RViz 执行与反馈 | Gazebo 中执行一次实际运动，保存控制器、Action、时钟和关节反馈 | `docs: record UR5e simulation baseline` |
| 11/27 · 第二天 | U03 C++ 单目标规划，U04 执行与误差，U05 三类场景 | 最小工位有可达、不可达和障碍物用例，能区分规划、执行和反馈失败 | `feat: add UR5e planning and execution cases` |
| 11/28 · 第三天 | U06 双规划器配对对照 | 目标 40 次规划请求，输出条件、逐试次 CSV 与代表性执行记录 | `eval: compare RRTConnect and RRTstar` |
| 11/29 · 第四天 | U07 一次备选重试，U08 Pilz LIN，U09 复跑与演示 | 可复现的重试日志、直线通行和阻挡记录、复跑 README | `test: verify retry, linear approach and replay` |

## 第一天 · U01–U02 官方仿真与关节反馈

### U01 启动环境

在本机 Ubuntu 图形桌面开终端 A。若显示 `(base)`，先运行 `conda deactivate`。

```bash
source /opt/ros/jazzy/setup.bash
echo "$ROS_DISTRO"
ros2 pkg prefix ur_simulation_gz
ros2 pkg prefix ur_moveit_config
ros2 pkg prefix ur_robot_driver
```

若找不到包，沿用已恢复的官方源执行 `sudo apt install ros-jazzy-ur-simulation-gz`，装好后再次执行 `ros2 pkg prefix ur_simulation_gz`。APT 找不到候选时先看 `apt-cache policy ros-jazzy-ur-simulation-gz`。

11 月 26 日首次启动时，仿真已进入 MoveIt 配置阶段，随后因缺少 `ur_robot_driver` 中止。若复现此错误，只补装 `sudo apt install ros-jazzy-ur-robot-driver`，再用 `ros2 pkg prefix ur_robot_driver` 核对。Jazzy 的 MoveIt 启动文件通过该包中的 `wait_for_robot_description` 等待 `/robot_description`。仿真入口已启用 RViz。[Jazzy 启动源码](https://github.com/UniversalRobots/Universal_Robots_ROS2_Driver/blob/jazzy/ur_moveit_config/launch/ur_moveit.launch.py)

三个包确认后，沿用本机已经完成 RViz 执行的控制器配置。下例使用项目约定的 YAML 路径。

```bash
cfg="$HOME/ur5e_ws/config/ur5e-gazebo-controllers.yaml"
out="$HOME/ur5e_ws/evidence/d06"
mkdir -p "$out"
set -o pipefail
ros2 launch ur_simulation_gz ur_sim_moveit.launch.py ur_type:=ur5e \
  controllers_file:="$cfg" 2>&1 | tee "$out/launch.log"
```

Gazebo 显示 UR5e、RViz 显示 MotionPlanning 后，保持终端 A 运行。记下实际软件包版本。

### U02 核对控制器与运动前后反馈

本机已实测 RViz 执行能驱动 Gazebo 中的 UR5e。控制器状态转录和运动前后的关节状态已归档在 [U02–U04 实测记录](../evidence/u03-u04/README.md#u02-rviz-执行与关节反馈)。

在终端 B 进入项目仓库，推荐用 `bash scripts/collect_u02.sh before` 采集，执行运动并停稳后运行 `bash scripts/collect_u02.sh after`。也可在终端 B 手动采集。

```bash
source /opt/ros/jazzy/setup.bash
out="$HOME/ur5e_ws/evidence/d06"
ros2 control list_controllers | tee "$out/controllers.txt"
ros2 action list -t | tee "$out/actions.txt"
ros2 param get /move_group use_sim_time | tee "$out/move-group-sim-time.txt"
timeout 10s ros2 topic echo /clock --once | tee "$out/clock.txt"
timeout 10s ros2 topic echo /joint_states --once | tee "$out/joints-before.yaml"
```

1. 确认实际使用的轨迹控制器和关节状态广播器为 `active`，记录 `FollowJointTrajectory` 的实际 Action 名。确认 `/move_group` 的 `use_sim_time` 为 `true`，隔一秒再读一次 `/clock`，确认时间在推进。
2. 在 RViz 选择实际规划组，UR 配置通常为 `ur_manipulator`。沿末端平移箭头轻移约 2–3 cm，保持朝向一致。先点 **Plan** 查看预览，再点 **Execute**。
3. 仿真机械臂停稳后，在终端 B 运行 `timeout 10s ros2 topic echo /joint_states --once | tee "$out/joints-after.yaml"`。按 `name` 匹配前后两个快照中的同一关节，记下实测角度的变化。
4. 用 `ros2 run tf2_tools view_frames` 查实际帧名。确认存在 `base_link` 和 `tool0` 后，可运行 `ros2 run tf2_ros tf2_echo base_link tool0` 查看末端位姿，记录后按 Ctrl+C。
5. 保存启动日志、两个关节快照和运动录像。若话题快照为空，先核对 Gazebo 是否暂停，再看控制器状态和启动日志。

约 60–90 分钟检查安装与启动，60–90 分钟执行并确认反馈，30 分钟归档。当天验收以 Gazebo 中确实发生运动和关节反馈变化为准。

## 第二天 · U03–U05 C++ 节点与三类用例

### U03 编译并验证只规划

把已有起步包中的 `ur5e-ros2-motion-bench` 解压至 `~/ur5e_ws/src/`。其中 C++ 包为 `ur5e_bench`，可执行文件为 `ur5e_smoke`。也可按本仓库[实施与面试手册的 U03](https://github.com/Y20040202/RESEARCH/blob/main/docs/execution-and-interview.md)创建同名包。

```bash
source /opt/ros/jazzy/setup.bash
cd "$HOME/ur5e_ws"
colcon build --packages-select ur5e_bench --symlink-install
source install/setup.bash
ros2 pkg executables ur5e_bench
ros2 run ur5e_bench ur5e_smoke --ros-args \
  -p use_sim_time:=true -p execute_motion:=false
```

起步节点等待当前状态，再从现有六个关节位置构造小幅目标。检查 `plan_code`、规划参考系和末端链节，观察 Gazebo 保持原位。若读不到当前状态，核对 `/joint_states`、`/clock` 及 `use_sim_time`。

### U04 执行与末端误差

1. 将 `execute_motion` 改为 `true`。`plan()` 返回成功且轨迹非空后，再调用 `execute(plan)`。
2. 读取执行结束后更新的实际关节反馈，做 FK 得到实际末端位姿，也可读取与反馈同步的 TF。目标、规划终点、反馈终点统一到同一参考系与末端链节。
3. **规划误差**用规划终点与任务目标的欧氏距离，**跟踪误差**用反馈终点与规划终点的欧氏距离，均记为 mm。关节目标先用 FK 得到任务目标。反馈超时单独记录。
4. 给节点增加 YAML 用例入口及 `plan_only` 和 `execute` 模式，保存用例目标、起始关节状态和场景 ID。

### U05 最小工位、三类用例与 CSV

1. 亲手编辑 `sorting_workcell.sdf`。先搭上表面高 `0.22 m` 的工作台，再用一块底板和四面箱壁做一个料箱。边长 `0.06 m` 的方块直接放在台面上，静置时中心约为 `Z = 0.25 m`。
2. 用上次实测可用的控制器配置启动，并通过 `world_file` 加载工位。统一 Gazebo 与 MoveIt 中的参考系、关键障碍物尺寸和位姿。分别确认 Gazebo 中的模型和 MoveIt 中的对象 ID。
3. 在 RViz 亲手选定来料上方、箱口上方两个位姿，保存位置、朝向和起始关节状态。先验证正常规划，再加入已知障碍复测。记录障碍挡在目标处还是途中。将这组位姿留给 U08 的直线接近测试。

| 用例 | 构造方式 | 验收 |
| --- | --- | --- |
| `reachable` | 复用第一天或 U04 已验证的目标 | 保存规划、执行返回码和实际末端误差 |
| `out_of_reach` | 设置明显超出机械臂工作空间的目标 | 记录规划失败，确认轨迹未下发 |
| `obstacle_detour` | 在直达路径附近加入箱体，保留绕行空间 | 确认规划场景包含箱体；保存绕行轨迹或原始错误码 |

用 `PlanningSceneInterface` 应用箱体并查其对象 ID。切换场景时按 ID 移除旧物体。若演示需要在 Gazebo 里也看到箱体，则按相同尺寸和位姿配置两边的场景。

每次规划尝试写一条 CSV，字段示例：

```text
case_id,scene_id,planner,repeat,attempt,plan_code,planning_ms,path_length_m,execution_sent,execute_code,plan_goal_error_mm,tracking_error_mm,failure_stage
```

规划失败时记 `execution_sent=false`，执行结果和误差留空。只规划的试次也保留独立记录。当天约 60 分钟编译和单目标规划，60 分钟做执行与反馈，60–100 分钟做用例和 CSV。

## 第三天 · U06 RRTConnect 与 RRT* 配对对照

1. 选无遮挡工位和加入挡板的工位各 2 个可达目标。每个用例保存固定的起始关节状态、目标、参考系、碰撞场景和时间预算。建议起步用 2 秒规划预算，并固定速度缩放。
2. 两个规划器各跑一条冒烟用例。核对实际加载的管线和规划器 ID。常见 OMPL ID 是 `RRTConnectkConfigDefault` 和 `RRTstarkConfigDefault`。若后者未加载，为实际规划组补配置，并从日志复核所用规划器。
3. 两个规划器交替运行，每个用例各重复 5 次，合计 **40 次规划请求**。每次只请求一次规划，关闭任务层重试，以 `plan_only` 为主。时间紧时每组先跑 3 次，以实际完成的 24 次报告。
4. 分别汇总规划成功数及分母、耗时中位数和成功轨迹的路径长度中位数。用 FK 计算各轨迹点的末端位置，累加相邻点的距离，统一用 m。若同时记录 MoveIt 内部规划时间和节点墙钟耗时，分别命名。
5. 选同一用例的两条成功轨迹做执行复查，分别恢复并核对同一实际起点，再记录执行码和跟踪误差。**规划请求数和轨迹实际执行数分开统计。**

约 45–60 分钟冻结用例并冒烟，60–90 分钟批跑，45–60 分钟统计和复查。

## 第四天 · U07–U09 重试、Pilz LIN 与复跑

### U07 一次备选重试

首选姿态按「规划 → 校验 → 执行 → 读取反馈」推进。规划或路径校验失败、任务又明确允许备选姿态时，重新读取当前状态，只尝试一个预先配置的备选姿态。备选通过校验才执行。两次失败后结束当前任务并记录原因。控制器故障和反馈丢失记录后结束当前任务。

- `fallback_success`。首选受已知碰撞条件阻挡，备选姿态满足任务约束且通过校验。保存两次尝试及最终执行反馈。
- `fallback_exhausted`。两个候选均失败，日志应有两条失败记录，且轨迹均未下发。
- 优先扩展「规划后加障碍，执行前复核」。先生成轨迹，再加箱体并确认规划场景已同步，用更新后的场景检查轨迹的采样状态。记录原轨迹被拦截和备选结果，写明采样口径。

### U08 Pilz LIN 直线接近

确认 `move_group` 加载了 Pilz 管线。机械臂在预接近位姿停稳，保持朝向一致，设一个短距目标。在同一起终点分别验证无遮挡和中段有箱体。无遮挡时保存一条成功轨迹及执行反馈，有阻挡时保存错误码。可用 FK 计算规划轨迹到目标直线的最大偏差。插件配置耗尽预算时，先归档 U07，把 LIN 留到缓冲日。

### U09 从新终端复跑

按 README 启动 Gazebo、编译和运行，至少复跑一条成功及一条失败用例。核对软件包版本、运行次数、逐试次 CSV 和代表性回放。用 60–90 秒演示串起 C++ 请求、仿真运动、实测误差和一次失败后的处理。简历填写实际规划次数、执行次数和误差。

## V01–V04 · 分拣工站扩展

通过 U09 后再接入下列模块。每个阶段先用单独的测试验证新增接口，再复跑上一个阶段的成功用例。夹爪适配、接触与视觉定位按实测排期。

### V01 夹爪与固定坐标分拣

- 先测空夹及夹住 6 cm 方块时的指间距。验证夹爪开合、关节反馈和工件在 Gazebo 中的抬升与释放。
- 用已知工件位姿完成分拣。OMPL 负责自由空间进场和搬运，Pilz LIN 用于直线接近、抬升或退出。测量物体离桌、搬运途中掉落和落箱后停稳。抓取和释放时同步更新 MoveIt 的附着碰撞体。
- 接触夹持耗时过长时，先单测 Gazebo 的固定关节绑定与解绑。若采用该方式，标明固定关节搬运模式，并单独报告接触夹持结果。

### V02 AprilTag 视觉定位与 A/B 分流

- 固定顶视相机，先覆盖来料区。核对图像、相机内参和时间戳，通过 `tf2` 将位姿转到基座坐标系，再应用标签到抓取点的偏移。
- 自己布置中心、左侧、右侧三个工件位置，用 Gazebo 真值评测定位误差。添加第二个料箱，以两个标签 ID 测试分流。记录目标箱、实际落箱结果和用时。

### V03 C++ 任务状态机与故障实验

- 用状态机串起定位、预抓取、直线接近、夹持、搬运与释放。在线判断使用夹爪反馈和相机，独立评测用 Gazebo 真值记录物体是否被抬升、搬运和落箱。
- 分别复现标签丢失、夹空和规划后新增挡板。记录失败阶段、错误码、是否下发轨迹及重试结果。保存正常分拣和受阻流程的代表性回放。

### V04 RGB-D 点云与 OctoMap（可选）

- 确认点云与机器人坐标对齐，并在 RViz 核对过滤后的点云和占据地图。用相机观察到的挡板复现 U07 执行前轨迹复核，记录占据地图更新与拦截结果。

## 按阶段同步到 GitHub 的材料

本仓库按阶段归档 UR5e 的代码、配置和运行证据。四项目手册中的计划副本存于 `Y20040202/RESEARCH`。本机项目根目录建议用 `~/ur5e_ws/src/ur5e-ros2-motion-bench`。独立仓库连接并核对 `git remote -v` 后，按第一张表的节点推送。当天只提交实测所需的文件，提交前检查 `git status --short` 和 `git diff --cached --stat`。

| 阶段 | 提交内容 |
| --- | --- |
| U01–U02 | 从 `~/ur5e_ws/evidence/d06` 取启动日志、版本、控制器、Action 及前后反馈；README 写复现命令 |
| U03–U05 | C++ 源码、CMake 与包清单、工位 SDF、用例 YAML、三类结果 CSV 及失败日志 |
| U06 | 两个规划器的实际配置、冻结用例、逐试次 CSV、汇总与一组执行反馈 |
| U07–U09 | 重试证据、LIN 通行与阻挡记录、基线复跑说明和代表性回放链接 |
| V01–V04 | 按已验收阶段提交夹爪与传感器配置、任务代码、误差或分拣 CSV、异常日志及代表性回放 |

工作区生成的 `build/`、`install/`、`log/` 和大体积原始录像留在本机或附件中。

例如 U01–U02 验收后，在已初始化且已关联独立远端的项目仓库执行下列命令。执行前先看文件内容与 `origin`，在日志中写明实际达成的验收项。

```bash
project_repo="$HOME/ur5e_ws/src/ur5e-ros2-motion-bench"
source_dir="$HOME/ur5e_ws/evidence/d06"
target_dir="$project_repo/evidence/d06"
git -C "$project_repo" remote -v
mkdir -p "$target_dir"
cp "$source_dir"/{launch.log,controllers.txt,actions.txt,move-group-sim-time.txt,clock.txt,joints-before.yaml,joints-after.yaml} "$target_dir/"
git -C "$project_repo" add -- evidence/d06/
git -C "$project_repo" diff --cached --stat
git -C "$project_repo" commit -m "docs: record UR5e simulation baseline"
git -C "$project_repo" push -u origin "$(git -C "$project_repo" branch --show-current)"
```

## 参考

- [Universal Robots Jazzy 仿真启动](https://docs.universal-robots.com/Universal_Robots_ROS2_Documentation/jazzy/doc/ur_simulation_gz/ur_simulation_gz/doc/usage.html)
- [MoveIt 2 C++ 接口教程](https://moveit.picknik.ai/main/doc/examples/move_group_interface/move_group_interface_tutorial.html)
- [ros2_control Jazzy 关节轨迹控制器](https://control.ros.org/jazzy/doc/ros2_controllers/joint_trajectory_controller/doc/userdoc.html)
- [OMPL 规划器](https://ompl.kavrakilab.org/planners.html) · [Pilz LIN](https://moveit.picknik.ai/main/doc/how_to_guides/pilz_industrial_motion_planner/pilz_industrial_motion_planner.html)
