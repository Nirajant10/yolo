from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import FileContent, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue



def generate_launch_description():
    # ''use_sim_time'' is used to have ros2 use /clock topic for the time source
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')

    urdf = FileContent(
        PathJoinSubstitution([FindPackageShare('my_amr_cpp'), 'urdf', 'Assem1_urdf.urdf']))

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use simulation (Gazebo) clock if true'),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{'use_sim_time': use_sim_time, 
                        'robot_description': ParameterValue(urdf, value_type=str)}],
            arguments=[urdf]),
        Node(
            package='my_amr_cpp',
            executable='my_amr_cpp',
            name='my_amr_cpp',
            output='screen'),
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
        arguments=[
            '-d', 
            PathJoinSubstitution([FindPackageShare('my_amr_cpp'), 'launch', 'rviz_config.rviz'])
        ])
    ])