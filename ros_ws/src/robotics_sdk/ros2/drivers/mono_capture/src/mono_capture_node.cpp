/*
 *
 * Copyright (c) 2021 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in appCntxtect code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any appCntxtect code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <signal.h>
#include "mono_capture_node.h"

/**
 * @brief USB mono camera ROS node
 *
 * @param [in] resolution  Resolution
 * @param [in] frame_rate  Frame rate
 */
MonoCamNode::MonoCamNode(const std::string& name,
                         const rclcpp::NodeOptions& options):
    Node(name, options)
{
    // parse ROS parameters
    get_parameter_or("camera_mode", camera_mode_, std::string("HD"));
    get_parameter_or("frame_rate", frame_rate_, 15.0);
    get_parameter_or("frame_id", frame_id_, std::string("camera"));
    get_parameter_or("device_name", device_name_, std::string("/dev/video1"));
    get_parameter_or("encoding", encoding_, std::string("yuv422"));
    get_parameter_or("image_topic",  image_topic_,  std::string("camera/image_raw"));
    get_parameter_or("camera_info_topic",  camera_info_topic_,  std::string("camera/camera_info"));
    get_parameter_or("camera_info_yaml", camera_info_yaml_, std::string(""));

    // validate frame rate and update if necessary
    validateFrameRate(camera_mode_, frame_rate_);

    RCLCPP_INFO(this->get_logger(), "Initialize the Webcam");
    MonoCamera mono_cam(device_name_, camera_mode_, frame_rate_, encoding_);

    // setup publisher
    auto imgTrans    = new ImgTrans(static_cast<rclcpp::Node::SharedPtr>(this));
    auto pub_image   = imgTrans->advertise(image_topic_, 1);
    auto pub_caminfo = this->create_publisher<CameraInfo>(camera_info_topic_, 1);

    // populate camera_info
    RCLCPP_INFO(this->get_logger(), "Loading camera info from yaml files");

    CameraInfoManager caminfo_manager(this, "camera", camera_info_yaml_);

    auto caminfo             = caminfo_manager.getCameraInfo();
    caminfo.header.frame_id  = frame_id_;

    // capture and publish images
    cv::Mat   cv_img;
    rclcpp::Rate framerate(frame_rate_);

    while (rclcpp::ok())
    {
        if (!mono_cam.getImages(cv_img))
        {
            RCLCPP_INFO_ONCE(this->get_logger(), "Cannot find the monocam");
        }
        else
        {
            RCLCPP_INFO_ONCE(this->get_logger(), "Successfully found the monocam");
        }

        rclcpp::Time now = this->get_clock()->now();

        publishImage(pub_image, cv_img, "monocam_frame", now);
        publishCamInfo(pub_caminfo, caminfo, now);

        framerate.sleep();
    }
}

MonoCamNode::~MonoCamNode() { }

/**
 * @brief Publish the camera_info
 *
 * @param [in] pub_cam_info  camera_info publisher
 * @param [in] cam_info_msg  camera_info message
 * @param [in] tstamp        timestamp
 */
void MonoCamNode::publishCamInfo(const rclcpp::Publisher<CameraInfo>::SharedPtr pub_cam_info,
                                   CameraInfo &cam_info_msg,
                                   rclcpp::Time tstamp)
{
    cam_info_msg.header.stamp = tstamp;
    pub_cam_info->publish(cam_info_msg);
}

/**
 * @brief Publish the image
 *
 * @param [in]  pub_img       Image publisher
 * @param [in]  img           Image message
 * @param [in]  img_frame_id  Image frame identifier
 * @param [in]  tstamp        Timestamp
 * @param [in]  encoding      image_transport encoding method
 */
void MonoCamNode::publishImage(const ImgPub &pub_img,
                                 const cv::Mat &img,
                                 const std::string &img_frame_id,
                                 rclcpp::Time tstamp)
{
    cv_bridge::CvImage cv_img;
    cv_img.image           = img;
    cv_img.encoding        = encoding_;
    cv_img.header.frame_id = img_frame_id;
    cv_img.header.stamp    = tstamp;
    pub_img.publish(cv_img.toImageMsg());
}

/**
 * @brief Validate the frame rate and update it if necessary
 *
 * @param [in] camera_mode Camera mode string
 * @param [in] frame_rate  Camera frame rate
 */
void MonoCamNode::validateFrameRate(std::string camera_mode, double& frame_rate)
{
    double max_frame_rate;

    if(camera_mode.compare("FHD")==0)
        max_frame_rate= 5;
    else if(camera_mode.compare("HD")==0)
        max_frame_rate= 10;
    else if(camera_mode.compare("VGA")==0)
        max_frame_rate= 30;
    else
        RCLCPP_FATAL(this->get_logger(), "Unknow camera_mode passed");

    if (frame_rate > max_frame_rate)
    {
        RCLCPP_WARN(this->get_logger(), "frame_rate = %f exceeds the max rate with %s, set to %f",
                 frame_rate, camera_mode.c_str(), max_frame_rate);
        frame_rate = max_frame_rate;
    }
}

int main(int argc, char** argv)
{
    try
    {
        rclcpp::InitOptions initOptions{};
        rclcpp::NodeOptions nodeOptions{};

        rclcpp::init(argc, argv, initOptions);

        nodeOptions.allow_undeclared_parameters(true);
        nodeOptions.automatically_declare_parameters_from_overrides(true);

        auto monocam_ros_node = std::make_shared<MonoCamNode>("mono_camera", nodeOptions);

        rclcpp::shutdown();

        return EXIT_SUCCESS;
    }
    catch (std::runtime_error& e)
    {
        rclcpp::shutdown();
        return EXIT_FAILURE;
    }
}
