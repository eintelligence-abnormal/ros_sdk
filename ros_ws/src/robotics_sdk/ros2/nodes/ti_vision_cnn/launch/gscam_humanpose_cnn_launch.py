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
image_format = 1
enable_ldc_node = 0
lut_file_path = "/opt/robotics_sdk/ros1/drivers/mono_capture/config/C920_HD_LUT.bin"
dl_model_path = "/opt/model_zoo/ONR-KD-7060-human-pose-yolox-s-640x640"

def get_launch_file(pkg, file_name):
    pkg_dir = get_package_share_directory(pkg)
    return os.path.join(pkg_dir, 'launch', file_name)

def generate_launch_description():
    cam_id_arg = DeclareLaunchArgument(
        'cam_id',
        default_value='0',
        description='ID of the video device to use.'
    )

    framerate_arg = DeclareLaunchArgument(
        'framerate',
        default_value='30',
        description='framerate.'
    )

    exportPerfStats_arg = DeclareLaunchArgument(
        "exportPerfStats", default_value=TextSubstitution(text="0")
    )

    width_arg = DeclareLaunchArgument(
        "width", default_value=TextSubstitution(text="1280")
    )

    height_arg = DeclareLaunchArgument(
        "height", default_value=TextSubstitution(text="720")
    )

    cam_calib_arg = DeclareLaunchArgument(
        "cam_calib", default_value=TextSubstitution(text="C920_HD_camera_info.yaml")
    )

    exportPerfStats_str_arg = DeclareLaunchArgument(
        "exportPerfStats_str", default_value=[LaunchConfiguration('exportPerfStats')]
    )

    detVizThreshold_arg = DeclareLaunchArgument(
        "detVizThreshold", default_value=TextSubstitution(text="0.8")
    )

    detVizThreshold_str_arg = DeclareLaunchArgument(
        "detVizThreshold_str", default_value=[LaunchConfiguration('detVizThreshold')]
    )

    # ti_vision_cnn node
    params = [
        os.path.join(get_package_share_directory('ti_vision_cnn'), 'config', 'params.yaml'),
        {
            "width":                    LaunchConfiguration('width'),
            "height":                   LaunchConfiguration('height'),
            "image_format":             image_format,
            "enable_ldc_node":          enable_ldc_node,
            "lut_file_path":            lut_file_path,
            "dl_model_path":            dl_model_path,
            "input_topic_name":         "camera/image_raw",
            "rectified_image_topic":    "camera/image_rect_nv12",
            "rectified_image_frame_id": "right_frame",
            "vision_cnn_tensor_topic":  "vision_cnn/tensor",
            "exportPerfStats":           LaunchConfiguration('exportPerfStats_str'),
            "detVizThreshold":           LaunchConfiguration('detVizThreshold_str'),
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

    # Include gscam2 launch file
    cam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(get_launch_file('gscam2', 'v4l_mjpg_launch.py')),
        launch_arguments={
            "cam_id":  LaunchConfiguration('cam_id'),
            "framerate": LaunchConfiguration('framerate'),
            "width":     LaunchConfiguration('width'),
            "height":    LaunchConfiguration('height'),
            "cam_calib": LaunchConfiguration('cam_calib')
        }.items()
    )

    ld = LaunchDescription()
    ld.add_action(cam_id_arg)
    ld.add_action(framerate_arg)
    ld.add_action(width_arg)
    ld.add_action(height_arg)
    ld.add_action(cam_calib_arg)
    ld.add_action(exportPerfStats_arg)
    ld.add_action(exportPerfStats_str_arg)
    ld.add_action(detVizThreshold_arg)
    ld.add_action(detVizThreshold_str_arg)
    ld.add_action(cnn_node)
    ld.add_action(cam_launch)

    return ld