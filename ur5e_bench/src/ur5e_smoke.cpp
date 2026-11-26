#include <exception>
#include <string>
#include <thread>

#include <moveit/move_group_interface/move_group_interface.hpp>
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
    if (!arm.getCurrentState(5.0)) {
      RCLCPP_ERROR(node->get_logger(), "No recent joint state");
      status = 2;
    } else {
      auto target = arm.getCurrentJointValues();
      if (target.size() != 6) {
        RCLCPP_ERROR(node->get_logger(), "Expected six UR5e joints");
        status = 3;
      } else {
        RCLCPP_INFO(node->get_logger(), "frame=%s eef=%s",
                    arm.getPlanningFrame().c_str(), arm.getEndEffectorLink().c_str());
        arm.setStartStateToCurrentState();
        arm.setPlanningTime(2.0);
        arm.setMaxVelocityScalingFactor(0.1);
        arm.setMaxAccelerationScalingFactor(0.1);
        target[0] += 0.10;
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
            const auto executed = arm.execute(plan);
            RCLCPP_INFO(node->get_logger(), "execute_code=%d", executed.val);
            if (executed.val != moveit_msgs::msg::MoveItErrorCodes::SUCCESS) {
              status = 6;
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
