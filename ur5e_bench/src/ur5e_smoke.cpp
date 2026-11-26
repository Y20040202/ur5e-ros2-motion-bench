#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <moveit/move_group_interface/move_group_interface.hpp>
#include <moveit/robot_state/robot_state.hpp>
#include <moveit_msgs/msg/move_it_error_codes.hpp>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("ur5e_smoke");
  const auto group_name = node->declare_parameter<std::string>(
      "planning_group", "ur_manipulator");
  const bool execute = node->declare_parameter<bool>("execute_motion", false);
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);
  std::thread spinner([&executor] { executor.spin(); });

  int status = 0;
  try {
    moveit::planning_interface::MoveGroupInterface arm(node, group_name);
    arm.startStateMonitor();
    const auto start_state = arm.getCurrentState(5.0);
    if (!start_state) {
      RCLCPP_ERROR(node->get_logger(), "No recent joint state");
      status = 2;
    } else {
      std::vector<double> target;
      start_state->copyJointGroupPositions(group_name, target);
      if (target.size() != 6) {
        RCLCPP_ERROR(node->get_logger(), "Expected six UR5e joints");
        status = 3;
      } else {
        RCLCPP_INFO(node->get_logger(), "frame=%s eef=%s",
                    arm.getPlanningFrame().c_str(), arm.getEndEffectorLink().c_str());
        arm.setStartState(*start_state);
        arm.setPlanningTime(2.0);
        arm.setMaxVelocityScalingFactor(0.1);
        arm.setMaxAccelerationScalingFactor(0.1);
        const double start_rad = target[0];
        target[0] += 0.20;
        RCLCPP_INFO(node->get_logger(),
                    "start_rad=%.6f target_rad=%.6f delta_rad=%.6f",
                    start_rad, target[0], target[0] - start_rad);
        if (!arm.setJointValueTarget(target)) {
          RCLCPP_ERROR(node->get_logger(), "Target outside limits");
          status = 4;
        } else {
          moveit::planning_interface::MoveGroupInterface::Plan plan;
          const auto planned = arm.plan(plan);
          RCLCPP_INFO(node->get_logger(), "plan_code=%d", planned.val);
          if (planned.val != moveit_msgs::msg::MoveItErrorCodes::SUCCESS) {
            status = 5;
          } else if (execute) {
            const auto& trajectory = plan.trajectory.joint_trajectory;
            if (trajectory.joint_names.empty() || trajectory.points.empty() ||
                trajectory.points.back().positions.size() != trajectory.joint_names.size()) {
              throw std::runtime_error("Invalid trajectory endpoint");
            }

            const auto eef = arm.getEndEffectorLink();
            if (eef.empty()) {
              throw std::runtime_error("No end-effector link");
            }

            auto goal_state = *start_state;
            goal_state.setJointGroupPositions(group_name, target);
            goal_state.update();
            const Eigen::Vector3d goal_xyz =
                goal_state.getGlobalLinkTransform(eef).translation();

            auto planned_state = *start_state;
            planned_state.setVariablePositions(
                trajectory.joint_names, trajectory.points.back().positions);
            planned_state.update();
            const Eigen::Vector3d planned_xyz =
                planned_state.getGlobalLinkTransform(eef).translation();

            const auto executed = arm.execute(plan);
            RCLCPP_INFO(node->get_logger(), "execute_code=%d", executed.val);
            if (executed.val != moveit_msgs::msg::MoveItErrorCodes::SUCCESS) {
              status = 6;
            } else {
              const auto actual_state = arm.getCurrentState(5.0);
              if (!actual_state) {
                throw std::runtime_error("No joint feedback after execution");
              }

              actual_state->update();
              const Eigen::Vector3d actual_xyz =
                  actual_state->getGlobalLinkTransform(eef).translation();

              RCLCPP_INFO_STREAM(
                  node->get_logger(),
                  "frame=" << arm.getRobotModel()->getModelFrame()
                  << " eef=" << eef
                  << "\ngoal_xyz_m=" << goal_xyz.transpose()
                  << "\nplanned_xyz_m=" << planned_xyz.transpose()
                  << "\nactual_xyz_m=" << actual_xyz.transpose()
                  << "\nplan_goal_error_mm=" << 1000.0 * (planned_xyz - goal_xyz).norm()
                  << "\ntracking_error_mm=" << 1000.0 * (actual_xyz - planned_xyz).norm());
            }
          }
        }
      }
    }
  } catch (const std::exception& error) {
    RCLCPP_ERROR(node->get_logger(), "%s", error.what());
    status = 7;
  }

  executor.cancel();
  spinner.join();
  rclcpp::shutdown();
  return status;
}
