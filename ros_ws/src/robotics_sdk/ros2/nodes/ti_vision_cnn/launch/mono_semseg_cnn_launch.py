import os
from ament_index_python.packages import get_package_share_directory
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import TextSubstitution
from launch.substitutions import LaunchConfiguration

# Input image format: 0 - VX_DF_IMAGE_U8, 1 - VX_DF_IMAGE_NV12, 2 - VX_DF_IMAGE_UYVY
image_format = 2
enable_ldc_node = 1
lut_file_path = "/opt/robotics_sdk/ros1/drivers/mono_capture/config/C920_HD_LUT.bin"
dl_model_path = "/opt/model_zoo/ONR-SS-7618-deeplabv3lite-mobv2-qat-robokit-768x432"

def get_launch_file(pkg, file_name):
    pkg_dir = get_package_share_directory(pkg)
    return os.path.join(pkg_dir, 'launch', file_name)

def generate_launch_description():
    ld = LaunchDescription()

    exportPerfStats_arg = DeclareLaunchArgument(
        "exportPerfStats", default_value=TextSubstitution(text="0")
    )

    exportPerfStats_str_arg = DeclareLaunchArgument(
        "exportPerfStats_str", default_value=[LaunchConfiguration('exportPerfStats')]
    )

    # ti_vision_cnn node
    params = [
        os.path.join(get_package_share_directory('ti_vision_cnn'), 'config', 'params.yaml'),
        {
            "width":                    1280,
            "height":                   720,
            "image_format":             image_format,
            "enable_ldc_node":          enable_ldc_node,
            "lut_file_path":            lut_file_path,
            "dl_model_path":            dl_model_path,
            "input_topic_name":         "camera/image_raw",
            "rectified_image_topic":    "camera/image_rect_nv12",
            "rectified_image_frame_id": "right_frame",
            "vision_cnn_tensor_topic":  "vision_cnn/tensor",
            "exportPerfStats":           LaunchConfiguration('exportPerfStats_str'),
        },
    ]
    cnn_node = Node(
        package = "ti_vision_cnn",
        executable = "vision_cnn",
        name = "vision_cnn",
        output = "screen",
        emulate_tty = True,
        parameters = params
    )

    # Include mono_capture launch file
    mono_launch_file = get_launch_file(
        pkg='mono_capture',
        file_name='mono_capture_launch.py'
    )
    cam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(mono_launch_file)
    )

    ld.add_action(exportPerfStats_arg)
    ld.add_action(exportPerfStats_str_arg)
    ld.add_action(cnn_node)
    ld.add_action(cam_launch)

    return ld
