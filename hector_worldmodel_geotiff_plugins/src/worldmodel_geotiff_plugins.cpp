//=================================================================================================
// Copyright (c) 2012, Johannes Meyer, TU Darmstadt
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the Flight Systems and Automatic Control group,
//       TU Darmstadt, nor the names of its contributors may be used to
//       endorse or promote products derived from this software without
//       specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//=================================================================================================

#include <hector_geotiff/map_writer_interface.h>
#include <hector_geotiff/map_writer_plugin_interface.h>

#include <ros/ros.h>
#include <hector_worldmodel_msgs/GetObjectModel.h>

#include <pluginlib/class_loader.h>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/formatters.hpp>

namespace hector_worldmodel_geotiff_plugins {

using namespace hector_geotiff;

class MapWriterPlugin : public MapWriterPluginInterface
{
public:
  MapWriterPlugin();
  virtual ~MapWriterPlugin();

  virtual void initialize(const std::string& name);
  virtual void draw(MapWriterInterface *interface) = 0;

protected:
  ros::NodeHandle nh_;
  ros::ServiceClient service_client_;

  bool initialized_;
  std::string name_;
  bool draw_all_objects_;
  std::string class_id_;
};

MapWriterPlugin::MapWriterPlugin()
  : initialized_(false)
{}

MapWriterPlugin::~MapWriterPlugin()
{}

void MapWriterPlugin::initialize(const std::string& name)
{
  ros::NodeHandle plugin_nh("~/" + name);
  std::string service_name_;

  plugin_nh.param("service_name", service_name_, std::string("worldmodel/get_object_model"));
  plugin_nh.param("draw_all_objects", draw_all_objects_, false);
  plugin_nh.param("class_id", class_id_, std::string());

  service_client_ = nh_.serviceClient<hector_worldmodel_msgs::GetObjectModel>(service_name_);

  initialized_ = true;
  this->name_ = name;
  ROS_INFO_NAMED(name_, "Successfully initialized hector_geotiff MapWriter plugin %s.", name_.c_str());
}

class VictimMapWriter : public MapWriterPlugin
{
public:
  virtual ~VictimMapWriter() {}

  void draw(MapWriterInterface *interface)
  {
    if (!initialized_) return;

    hector_worldmodel_msgs::GetObjectModel data;
    if (!service_client_.call(data)) {
      ROS_ERROR_NAMED(name_, "Cannot draw victims, service %s failed", service_client_.getService().c_str());
      return;
    }

    int counter = 0;
    for(hector_worldmodel_msgs::ObjectModel::_objects_type::const_iterator it = data.response.model.objects.begin(); it != data.response.model.objects.end(); ++it) {
      const hector_worldmodel_msgs::Object& object = *it;
      if (!draw_all_objects_ && object.state.state != hector_worldmodel_msgs::ObjectState::CONFIRMED) continue;
      if (!class_id_.empty() && object.info.class_id != class_id_) continue;

      Eigen::Vector2f coords;
      coords.x() = object.pose.pose.position.x;
      coords.y() = object.pose.pose.position.y;
      interface->drawObjectOfInterest(coords, boost::lexical_cast<std::string>(++counter), MapWriterInterface::Color(240,10,10));
    }
  }
};

class QRCodeMapWriter : public MapWriterPlugin
{
public:
  virtual ~QRCodeMapWriter() {}

  
  void draw(MapWriterInterface *interface)
  {
    if (!initialized_) return;

    hector_worldmodel_msgs::GetObjectModel data;
    if (!service_client_.call(data)) {
      ROS_ERROR_NAMED(name_, "Cannot draw victims, service %s failed", service_client_.getService().c_str());
      return;
    }

    std::string team_name;
    std::string mission_name;
    nh_.getParamCached("/team", team_name);
    nh_.getParamCached("/mission", mission_name);

    boost::posix_time::ptime now = ros::Time::now().toBoost();
    boost::gregorian::date now_date(now.date());
    boost::posix_time::time_duration now_time(now.time_of_day().hours(), now.time_of_day().minutes(), now.time_of_day().seconds(), 0);

    std::ofstream description_file((interface->getBasePathAndFileName() + "_qr.csv").c_str());
    if (description_file.is_open()) {
      if (!team_name.empty()) description_file << team_name << std::endl;
      description_file << now_date << "; " << now_time << std::endl;
      if (!mission_name.empty()) description_file << mission_name << std::endl;
      description_file << std::endl;
    }

    int counter = 0;
    for(hector_worldmodel_msgs::ObjectModel::_objects_type::const_iterator it = data.response.model.objects.begin(); it != data.response.model.objects.end(); ++it) {
      const hector_worldmodel_msgs::Object& object = *it;
      if (!class_id_.empty() && object.info.class_id != class_id_) continue;
      if (!draw_all_objects_ && object.state.state != hector_worldmodel_msgs::ObjectState::CONFIRMED) continue;
      if (object.state.state == hector_worldmodel_msgs::ObjectState::DISCARDED) continue;

      Eigen::Vector2f coords;
      coords.x() = object.pose.pose.position.x;
      coords.y() = object.pose.pose.position.y;
      interface->drawObjectOfInterest(coords, boost::lexical_cast<std::string>(++counter), MapWriterInterface::Color(10,10,240));

      if (description_file.is_open()) {
        boost::posix_time::time_duration time_of_day(object.header.stamp.toBoost().time_of_day());
        boost::posix_time::time_duration time(time_of_day.hours(), time_of_day.minutes(), time_of_day.seconds(), 0);
        description_file << counter << ";" << time << ";" << object.info.object_id << ";" << object.pose.pose.position.x << ";" << object.pose.pose.position.y << ";" << object.pose.pose.position.z << std::endl;
      }
    }

    description_file.close();
  }
};

} // namespace

//register this planner as a MapWriterPluginInterface plugin
#include <pluginlib/class_list_macros.h>
#ifdef PLUGINLIB_EXPORT_CLASS
  PLUGINLIB_EXPORT_CLASS(hector_worldmodel_geotiff_plugins::VictimMapWriter, hector_geotiff::MapWriterPluginInterface)
  PLUGINLIB_EXPORT_CLASS(hector_worldmodel_geotiff_plugins::QRCodeMapWriter, hector_geotiff::MapWriterPluginInterface)
#else
  PLUGINLIB_DECLARE_CLASS(hector_worldmodel_geotiff_plugins, VictimMapWriter, hector_worldmodel_geotiff_plugins::VictimMapWriter, hector_geotiff::MapWriterPluginInterface)
  PLUGINLIB_DECLARE_CLASS(hector_worldmodel_geotiff_plugins, QRCodeMapWriter, hector_worldmodel_geotiff_plugins::QRCodeMapWriter, hector_geotiff::MapWriterPluginInterface)
#endif

