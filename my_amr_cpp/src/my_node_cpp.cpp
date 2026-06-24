#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/quaternion.hpp>
#include <geometry_msgs/msg/twist.hpp> // Added for teleop subscription
#include <sensor_msgs/msg/joint_state.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <cmath>
#include <thread>
#include <chrono>

using namespace std::chrono;

class StatePublisher : public rclcpp::Node {
public:
    StatePublisher(rclcpp::NodeOptions options = rclcpp::NodeOptions()) :
        Node("state_publisher", options) 
    {
        // Publisher for the actual wheel joints
        joint_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
        
        // Broadcaster for mobile base position (odom -> base_link)
        broadcaster = std::make_shared<tf2_ros::TransformBroadcaster>(this);
        
        // SUBSCRIBE to teleop velocity commands
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10, std::bind(&StatePublisher::cmdVelCallback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "AMR state publisher ready. Run teleop to move it!");
        
        // Loop at ~30 Hz to broadcast the position updates
        timer_ = this->create_wall_timer(33ms, std::bind(&StatePublisher::publish, this));
    }

private:
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_; // Subscriber pointer
    std::shared_ptr<tf2_ros::TransformBroadcaster> broadcaster;
    rclcpp::TimerBase::SharedPtr timer_;

    // Current live tracking states (Initialized to 0 so it spawns still)
    double robot_x = 0.0;
    double robot_y = 0.0;
    double robot_yaw = 0.0;
    double wheel_angle = 0.0;

    // Store incoming velocities from teleop
    double linear_vel_x = 0.0;
    double angular_vel_z = 0.0;

    const double dt = 0.033; // 33ms time step matching the timer loop

    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        // Grab the velocity inputs from your keyboard/joystick teleop
        linear_vel_x = msg->linear.x;
        angular_vel_z = msg->angular.z;
    }

    void publish();
};

void StatePublisher::publish() {
    geometry_msgs::msg::TransformStamped t;
    sensor_msgs::msg::JointState joint_state;
    const auto ts = this->get_clock()->now();

    // 1. UPDATE ROBOT POSITION (Basic Odometry Physics Equations)
    // Update orientation (Yaw) based on angular speed
    robot_yaw += angular_vel_z * dt;
    
    // Update X and Y based on linear speed and current heading
    robot_x += linear_vel_x * cos(robot_yaw) * dt;
    robot_y += linear_vel_x * sin(robot_yaw) * dt;

    // Spin wheels only if the robot is actually moving
    if (std::abs(linear_vel_x) > 0.001 || std::abs(angular_vel_z) > 0.001) {
        wheel_angle += (linear_vel_x * 5.0 * dt); 
    }

    // 2. POPULATE JOINT STATES
    joint_state.header.stamp = ts;
    joint_state.name = {"Wheel_joint1", "Wheel_joint2", "Wheel_joint3", "Wheel_joint4", "Lidar_joint"};
    joint_state.position = {wheel_angle, wheel_angle, wheel_angle, wheel_angle, 0.0};

    // 3. POPULATE ODOMETRY TRANSFORM
    t.header.stamp = ts;
    t.header.frame_id = "odom";       
    t.child_frame_id = "base_footprint";   

    t.transform.translation.x = robot_x;
    t.transform.translation.y = robot_y;
    t.transform.translation.z = 0.0;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, robot_yaw);
    t.transform.rotation.x = q.x();
    t.transform.rotation.y = q.y();
    t.transform.rotation.z = q.z();
    t.transform.rotation.w = q.w();

    // 4. BROADCAST
    broadcaster->sendTransform(t);
    joint_pub_->publish(joint_state);
}

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<StatePublisher>());
    rclcpp::shutdown();
    return 0;
}