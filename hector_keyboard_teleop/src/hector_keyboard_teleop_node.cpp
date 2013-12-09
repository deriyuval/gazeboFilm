#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main(int argc, char **argv)
{
  //Initialize the node and connect to master
  ros::init(argc, argv, "hector_keyboad_node");

  //generate a node handler to handle all messages
  ros::NodeHandle n;

  ros::Publisher vel_pub = n.advertise<geometry_msgs::Twist>("/cmd_vel", 1000);

  ros::Rate rate(5);
  //structs to hold the shell buffer
  struct termios stdio;
  struct termios old_stdio;

  unsigned char c='D';
  tcgetattr(STDOUT_FILENO,&old_stdio);

  memset(&stdio,0,sizeof(stdio));
  stdio.c_iflag=0;
  stdio.c_oflag=0;
  stdio.c_cflag=0;
  stdio.c_lflag=0;
  stdio.c_cc[VMIN]=1;
  stdio.c_cc[VTIME]=0;
  tcsetattr(STDOUT_FILENO,TCSANOW,&stdio);
  tcsetattr(STDOUT_FILENO,TCSAFLUSH,&stdio);
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);       // make the reads non-blocking

  while (ros::ok() && c!='q')
  {
    /**
     * This is a message object. You stuff it with data, and then publish it.
     */
	geometry_msgs::Twist msg;
	while(read(STDIN_FILENO,&c,1)>0){}

    if (c!=' ') {
    	switch(c){
    		case 'i':
    			msg.linear.x=1;
    			vel_pub.publish(msg);
    			break;
    		case 'j':
    			msg.linear.y=1;
    			vel_pub.publish(msg);
    			break;
    		case 'l':
    			msg.linear.y=-1;
    			vel_pub.publish(msg);
    			break;
    		case 'k':
    			msg.linear.x=-1;
    			vel_pub.publish(msg);
    			break;
    		case 't':
    			msg.linear.z=1;
    			vel_pub.publish(msg);
    			break;
    		case 'g':
    			msg.linear.z=-1;
    			vel_pub.publish(msg);
    			break;
    		case 'o':
    			msg.angular.z=-1;
    			vel_pub.publish(msg);
    			break;
    		case 'u':
    			msg.angular.z=1;
    			vel_pub.publish(msg);
    			break;
    	}
        ros::spinOnce();
        rate.sleep();
        c=' ';
    }else{
    	  //stop all activity
        vel_pub.publish(msg);
	    ros::spinOnce();
        rate.sleep();
    }

  }
  tcsetattr(STDOUT_FILENO,TCSANOW,&old_stdio);

  return 0;
}
