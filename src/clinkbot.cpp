// Copyright (c) 2013-2016 Barobo, Inc.
//
// This file is part of liblinkbot.
//
// liblinkbot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// liblinkbot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with liblinkbot.  If not, see <http://www.gnu.org/licenses/>.

#include <linkbot/linkbot.hpp>

#include <algorithm>
#include <cstdlib>
#include <thread>


#include <util/asio/iothread.hpp>
#include <util/asio/ws/acceptor.hpp>
#include <util/asio/ws/connector.hpp>
#include <util/global.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/asio/use_future.hpp>

#include "message.pb.h"

#include "rgbhashtable.h"


#define LINKBOT_MAX_SPEED 200

using std::this_thread::sleep_for;
using std::chrono::seconds;
using std::chrono::milliseconds;

namespace barobo {

CLinkbot::CLinkbot(const std::string& serialId) : 
    Linkbot(boost::to_upper_copy<std::string>(serialId)), 
    mSerialId(boost::to_upper_copy<std::string>(serialId))  
{ }

CLinkbot::CLinkbot() : 
    Linkbot()
{ }

void CLinkbot::getAccelerometerData(double &x, double &y, double &z) {
    int timestamp;
    Linkbot::getAccelerometer(timestamp, x, y, z);
}

void CLinkbot::getBatteryVoltage(double &voltage) {
    Linkbot::getBatteryVoltage(voltage);
}

void CLinkbot::getDistance(double &distance, double radius)
{
    int timestamp;
    double a1, a2, a3;
    Linkbot::getJointAngles(timestamp, a1, a2, a3);
    // Convert to radians
    a1 = a1 * M_PI / 180.0;
    distance = a1 * radius;
}

void CLinkbot::getFormFactor(LinkbotFormFactor &form) {
    Linkbot::getFormFactor(form);
}

void CLinkbot::getJointAngle(LinkbotJoint joint, double &angle) {
    double angles[3];
    int timestamp;
    Linkbot::getJointAngles(timestamp, angles[0], angles[1], angles[2]);
    angle = angles[joint];
}

void CLinkbot::getJointAngles(double &angle1, double &angle2, double &angle3) {
    int timestamp;
    Linkbot::getJointAngles(timestamp, angle1, angle2, angle3);
}

void CLinkbot::getJointSpeed(LinkbotJoint id, double &speed) {
    double speeds[3];
    Linkbot::getJointSpeeds(speeds[0], speeds[1], speeds[2]);
    speed = speeds[id];
}

void CLinkbot::getJointSpeedRatio(LinkbotJoint id, double &ratio) {
    double speed;
    getJointSpeed(id, speed);
    ratio = speed/LINKBOT_MAX_SPEED;
}

void CLinkbot::getJointSpeeds(double &speed1, double &speed2, double &speed3) {
    Linkbot::getJointSpeeds(speed1, speed2, speed3);
}

void CLinkbot::getJointSpeedRatios(double &ratio1, double &ratio2, double &ratio3) {
    getJointSpeeds(ratio1, ratio2, ratio3);
    ratio1 /= LINKBOT_MAX_SPEED;
    ratio2 /= LINKBOT_MAX_SPEED;
    ratio3 /= LINKBOT_MAX_SPEED;
}

void CLinkbot::getLEDColor(char color[]) {
  int getRGB[3];
  int retval;
  rgbHashTable * rgbTable = NULL;

  getLEDColorRGB(getRGB[0], getRGB[1], getRGB[2]);

  rgbTable = HT_Create();
  retval = HT_GetKey(rgbTable, getRGB, color);
  HT_Destroy(rgbTable);
}

void CLinkbot::getLEDColorRGB(int &r, int &g, int &b) {
    Linkbot::getLedColor(r, g, b);
}

// SETTERS

void CLinkbot::setBuzzerFrequency(int frequency, double time) {
    Linkbot::setBuzzerFrequency(frequency);
    sleep_for(milliseconds(int(time*1000)));
    Linkbot::setBuzzerFrequency(0);
}

void CLinkbot::setBuzzerFrequencyOn(int frequency) {
    Linkbot::setBuzzerFrequency(frequency);
}

void CLinkbot::setBuzzerFrequencyOff() {
    Linkbot::setBuzzerFrequency(0);
}

void CLinkbot::setJointMovementStateNB(LinkbotJoint id, LinkbotDirection dir)
{
    auto coefficient = 0.0;
    auto state = LINKBOT_JOINT_STATE_COAST;
    switch(dir) {
        case LINKBOT_POSITIVE:
            coefficient = 1;
            break;
        case LINKBOT_NEGATIVE:
            coefficient = -1;
            break;
        case LINKBOT_FORWARD:
            coefficient = (id == LINKBOT_JOINT_THREE)? -1 : 1;
            break;
        case LINKBOT_BACKWARD:
            coefficient = (id == LINKBOT_JOINT_THREE)? 1 : -1;
            break;
        default:
            break;
    }
    switch(dir) {
        case LINKBOT_POSITIVE:
        case LINKBOT_NEGATIVE:
        case LINKBOT_FORWARD:
        case LINKBOT_BACKWARD:
            state = LINKBOT_JOINT_STATE_MOVING;
            break;
        default:
            break;
    }
    Linkbot::setJointStates(1<<id, 
        state, coefficient,
        state, coefficient,
        state, coefficient);
}

void CLinkbot::setJointMovementStateTime(LinkbotJoint id, LinkbotDirection dir, double seconds)
{
    setJointMovementStateTimeNB(id, dir, seconds);
    moveWait(1<<id);
}

void CLinkbot::setJointMovementStateTimeNB(LinkbotJoint id, LinkbotDirection dir, double seconds)
{
    auto coefficient = 0.0;
    auto state = LINKBOT_JOINT_STATE_COAST;
    switch(dir) {
        case LINKBOT_POSITIVE:
            coefficient = 1;
            break;
        case LINKBOT_NEGATIVE:
            coefficient = -1;
            break;
        case LINKBOT_FORWARD:
            coefficient = (id == LINKBOT_JOINT_THREE)? -1 : 1;
            break;
        case LINKBOT_BACKWARD:
            coefficient = (id == LINKBOT_JOINT_THREE)? 1 : -1;
            break;
        default:
            break;
    }
    switch(dir) {
        case LINKBOT_POSITIVE:
        case LINKBOT_NEGATIVE:
        case LINKBOT_FORWARD:
        case LINKBOT_BACKWARD:
            state = LINKBOT_JOINT_STATE_MOVING;
            break;
        default:
            break;
    }

    Linkbot::setJointStates(1<<id,
        state, coefficient, seconds, LINKBOT_JOINT_STATE_HOLD,
        state, coefficient, seconds, LINKBOT_JOINT_STATE_HOLD,
        state, coefficient, seconds, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::setJointSpeed(LinkbotJoint id, double speed) {
    Linkbot::setJointSpeeds(1<<id, speed, speed, speed);
}

void CLinkbot::setJointSpeeds(double speed1, double speed2, double speed3) {
    Linkbot::setJointSpeeds(7, speed1, speed2, speed3);
}

void CLinkbot::setJointSpeedRatio(LinkbotJoint id, double ratio) {
    setJointSpeed(id, ratio*LINKBOT_MAX_SPEED);
}

void CLinkbot::setJointSpeedRatios(double ratio1, double ratio2, double ratio3) {
    setJointSpeeds(
        ratio1*LINKBOT_MAX_SPEED,
        ratio2*LINKBOT_MAX_SPEED,
        ratio3*LINKBOT_MAX_SPEED );
}

void CLinkbot::setJointPower(LinkbotJoint id, double power) {
    Linkbot::motorPower(1<<id, power*255, power*255, power*255);
}

void CLinkbot::setLEDColor(const char* color)
{
  int htRetval;
  int getRGB[3];
  rgbHashTable * rgbTable = HT_Create();

  htRetval = HT_Get(rgbTable, color, getRGB);
  HT_Destroy(rgbTable);

  setLEDColorRGB(getRGB[0], getRGB[1], getRGB[2]);
}

void CLinkbot::setLEDColorRGB(int r, int g, int b) {
    Linkbot::setLedColor(r, g, b);
}

void CLinkbot::setMotorPowers(double p1, double p2, double p3) {
    Linkbot::motorPower(7, p1*255, p2*255, p3*255);
}

void CLinkbot::setMovementStateNB(LinkbotDirection dir1,
            LinkbotDirection dir2,
            LinkbotDirection dir3)
{
    LinkbotJointState states[3];
    double c[3];
    std::vector<LinkbotDirection> dirs = {dir1, dir2, dir3};
    for(auto i = 0; i < 3; i++) {
        switch(dirs[i]) { 
            case LINKBOT_NEGATIVE:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = -1;
                break;
            case LINKBOT_NEUTRAL:
                states[i] = LINKBOT_JOINT_STATE_COAST;
                c[i] = 0;
                break;
            case LINKBOT_POSITIVE:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = 1;
                break;
            case LINKBOT_FORWARD:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = 1;
                if(i == 2) {
                    c[i] *= -1;
                }
                break;
            case LINKBOT_BACKWARD:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = -1;
                if(i == 2) {
                    c[i] *= -1;
                }
                break;
        }
    }   
    Linkbot::setJointStates(0x07, 
        states[0], c[0],
        states[1], c[1],
        states[2], c[2]);
}


void CLinkbot::setMovementStateTime( LinkbotDirection dir1,
        LinkbotDirection dir2,
        LinkbotDirection dir3,
        double seconds)
{
    setMovementStateTimeNB(dir1, dir2, dir3, seconds);
    moveWait();
}

void CLinkbot::setMovementStateTimeNB( LinkbotDirection dir1,
        LinkbotDirection dir2,
        LinkbotDirection dir3,
        double seconds)
{
    LinkbotJointState states[3];
    double c[3];
    std::vector<LinkbotDirection> dirs = {dir1, dir2, dir3};
    for(auto i = 0; i < 3; i++) {
        switch(dirs[i]) { 
            case LINKBOT_NEGATIVE:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = -1;
                break;
            case LINKBOT_NEUTRAL:
                states[i] = LINKBOT_JOINT_STATE_COAST;
                c[i] = 0;
                break;
            case LINKBOT_POSITIVE:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = 1;
                break;
            case LINKBOT_FORWARD:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = 1;
                if(i == 2) {
                    c[i] *= -1;
                }
                break;
            case LINKBOT_BACKWARD:
                states[i] = LINKBOT_JOINT_STATE_MOVING;
                c[i] = -1;
                if(i == 2) {
                    c[i] *= -1;
                }
                break;
        }
    }   
    Linkbot::setJointStates(0x07,
        states[0], c[0], seconds, LINKBOT_JOINT_STATE_HOLD,
        states[1], c[1], seconds, LINKBOT_JOINT_STATE_HOLD,
        states[2], c[2], seconds, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::setSpeed(double speed, double radius) {
    auto omega = speed / radius;
    omega *= 180/M_PI;
    setJointSpeeds(omega, omega, omega);
}

void CLinkbot::accelJointAngleNB(LinkbotJoint id, double acceleration, double angle)
{
    auto timeout = sqrt( (2*angle)/acceleration );
    Linkbot::setJointAccelI(1<<id, acceleration, acceleration, acceleration);
    Linkbot::moveAccel(1<<id, 0x07, 
        0, timeout, LINKBOT_JOINT_STATE_HOLD,
        0, timeout, LINKBOT_JOINT_STATE_HOLD,
        0, timeout, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::accelJointTimeNB(LinkbotJoint id, double acceleration, double time)
{
    Linkbot::setJointAccelI(1<<id, acceleration, acceleration, acceleration);
    Linkbot::moveAccel(1<<id, 0x07,
        0, time, LINKBOT_JOINT_STATE_HOLD,
        0, time, LINKBOT_JOINT_STATE_HOLD,
        0, time, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::accelJointToVelocityNB(LinkbotJoint id, double acceleration, double speed)
{
    auto timeout = speed/acceleration;
    Linkbot::setJointAccelI(1<<id, acceleration, acceleration, acceleration);
    Linkbot::setJointSpeeds(0x07, 
        LINKBOT_MAX_SPEED,
        LINKBOT_MAX_SPEED,
        LINKBOT_MAX_SPEED);
    Linkbot::moveAccel(1<<id, 0x07, 
        0, timeout, LINKBOT_JOINT_STATE_MOVING,
        0, timeout, LINKBOT_JOINT_STATE_MOVING,
        0, timeout, LINKBOT_JOINT_STATE_MOVING);
}

void CLinkbot::accelJointToMaxSpeedNB(LinkbotJoint id, double acceleration)
{
    accelJointToVelocityNB(id, acceleration, LINKBOT_MAX_SPEED);
}

void CLinkbot::driveAccelJointTimeNB(double radius, double acceleration,
            double time)
{
    auto alpha = (acceleration / radius) * 180 / M_PI;
    Linkbot::setJointAccelI(0x05, alpha, 0, -alpha);
    Linkbot::moveAccel(0x05, 0x07, 
        0, time, LINKBOT_JOINT_STATE_HOLD,
        0, time, LINKBOT_JOINT_STATE_HOLD,
        0, time, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::driveAccelToVelocityNB(double radius, double acceleration,
            double velocity)
{
    auto alpha = (acceleration / radius) * 180 / M_PI;
    Linkbot::setJointAccelI(0x06, alpha, 0, -alpha);
    auto time = velocity / acceleration;
    Linkbot::moveAccel(0x05, 0x07, 
        0, time, LINKBOT_JOINT_STATE_MOVING,
        0, time, LINKBOT_JOINT_STATE_MOVING,
        0, time, LINKBOT_JOINT_STATE_MOVING);
}

void CLinkbot::driveAccelToMaxSpeedNB(double radius, double acceleration)
{
    driveAccelToVelocityNB(radius, acceleration, LINKBOT_MAX_SPEED);
}

void CLinkbot::driveAccelDistanceNB(double radius, double acceleration, 
        double distance)
{
    auto timeout = sqrt( (2*distance) / acceleration );
    driveAccelJointTimeNB(radius, acceleration, timeout);
}

void CLinkbot::holdJoint(LinkbotJoint id) {
    Linkbot::move(1<<id, 0, 0, 0);
}

void CLinkbot::holdJoints() {
    Linkbot::move(0x07, 0, 0, 0);
}

void CLinkbot::holdJointsAtExit() {
    Linkbot::setPeripheralResetMask(0x07, 0);
}

void CLinkbot::resetToZero() {
    Linkbot::resetEncoderRevs();
    moveTo(0, 0, 0);
}

void CLinkbot::resetToZeroNB() {
    Linkbot::resetEncoderRevs();
    moveToNB(0, 0, 0);
}

// MOVEMENT

void CLinkbot::move(double j1, double j2, double j3) {
    moveNB(j1, j2, j3);
    moveWait();
}

void CLinkbot::moveNB(double j1, double j2, double j3) {
    Linkbot::move(7, j1, j2, j3);
}

void CLinkbot::moveWait(int mask) {
    Linkbot::moveWait(mask);
}

void CLinkbot::moveForeverNB()
{
    Linkbot::setJointStates(0x07, 
        LINKBOT_JOINT_STATE_MOVING, 1,
        LINKBOT_JOINT_STATE_MOVING, 1,
        LINKBOT_JOINT_STATE_MOVING, 1);
}

void CLinkbot::moveJoint(LinkbotJoint id, double angle) {
    moveJointNB(id, angle);
    moveJointWait(id);
}

void CLinkbot::moveJointNB(LinkbotJoint id, double angle) {
    Linkbot::move(1<<id, angle, angle, angle);
}

void CLinkbot::moveJointForeverNB(LinkbotJoint id)
{
    Linkbot::setJointStates(1<<id, 
        LINKBOT_JOINT_STATE_MOVING, 1,
        LINKBOT_JOINT_STATE_MOVING, 1,
        LINKBOT_JOINT_STATE_MOVING, 1);
}

void CLinkbot::moveJointTime(LinkbotJoint id, double time)
{
    moveJointTimeNB(id, time);
    moveWait(1<<id);
}

void CLinkbot::moveJointTimeNB(LinkbotJoint id, double time)
{
    Linkbot::setJointStates(1<<id,
        LINKBOT_JOINT_STATE_MOVING, 1, time, LINKBOT_JOINT_STATE_HOLD,
        LINKBOT_JOINT_STATE_MOVING, 1, time, LINKBOT_JOINT_STATE_HOLD,
        LINKBOT_JOINT_STATE_MOVING, 1, time, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::moveJointWait(LinkbotJoint id) {
    Linkbot::moveWait(1<<id);
}

void CLinkbot::moveJointTo(LinkbotJoint id, double angle)
{
    moveJointToNB(id, angle);
    moveWait(1<<id);
}

void CLinkbot::moveJointToNB(LinkbotJoint id, double angle)
{
    Linkbot::moveTo(1<<id, angle, angle, angle);
}

void CLinkbot::moveTime(double time)
{
    moveTimeNB(time);
    moveWait();
}

void CLinkbot::moveTimeNB(double time)
{
    Linkbot::setJointStates(0x07,
        LINKBOT_JOINT_STATE_MOVING, 1, time, LINKBOT_JOINT_STATE_HOLD,
        LINKBOT_JOINT_STATE_MOVING, 1, time, LINKBOT_JOINT_STATE_HOLD,
        LINKBOT_JOINT_STATE_MOVING, 1, time, LINKBOT_JOINT_STATE_HOLD);
}

void CLinkbot::moveTo(double angle1, double angle2, double angle3) {
    moveToNB(angle1, angle2, angle3);
    moveWait(7);
}

void CLinkbot::moveToNB(double angle1, double angle2, double angle3) {
    Linkbot::moveTo(7, angle1, angle2, angle3);
}

void CLinkbot::moveToZero()
{
    moveToZeroNB();
    moveWait();
}

void CLinkbot::moveToZeroNB()
{
    moveToNB(0, 0, 0);
}

void CLinkbot::relaxJoint(LinkbotJoint id)
{
    stop(1<<id);
}

void CLinkbot::relaxJoints()
{
    stop();
}

void CLinkbot::stop(int mask) {
    Linkbot::stop(mask);
}

void CLinkbot::stopOneJoint(LinkbotJoint id)
{
    stop(1<<id);
}

void CLinkbot::setButtonEventCallback( LinkbotButtonEventCallback cb, void* userData) {
    Linkbot::setButtonEventCallback(cb, userData);
}

void CLinkbot::setButtonEventCallback (std::function<void(LinkbotButton, LinkbotButtonState, int)> cb) {
    Linkbot::setButtonEventCallback(cb);
}

void CLinkbot::setEncoderEventCallback (LinkbotEncoderEventCallback cb, double granularity, void* userData) {
    Linkbot::setEncoderEventCallback(cb, granularity, userData);
}

void CLinkbot::setEncoderEventCallback (std::function<void(int, double, int)> cb, double granularity) {
    Linkbot::setEncoderEventCallback(cb, granularity);
}

void CLinkbot::setAccelerometerEventCallback (LinkbotAccelerometerEventCallback cb, void* userData) {
    Linkbot::setAccelerometerEventCallback(cb, userData);
}

void CLinkbot::setAccelerometerEventCallback (std::function<void(double, double, double, int)> cb) {
    Linkbot::setAccelerometerEventCallback(cb);
}

void CLinkbot::delaySeconds(double seconds) {
    sleep_for(milliseconds(int(seconds*1000)));
}

void CLinkbot::recordAnglesBegin()
{
    // Clear the vectors
    for (auto&& v : _plotData) {
        v.clear();
    }

    Linkbot::setEncoderEventCallback(
        [this] (int motor, double angle, int timestamp) {
            if( (motor < 0) || (motor > 2) ) {
                return;
            }
            _plotData[motor*2].push_back(timestamp);
            _plotData[motor*2 + 1].push_back(angle);
        },
        2.0
    );
}

PlotData CLinkbot::recordAnglesEnd()
{
    Linkbot::setEncoderEventCallback(nullptr, 0);
    return _plotData;
}

void sendToPrex(std::string json) {
    auto ioThread = util::global<util::asio::IoThread>();
    auto host = "localhost";
    auto service = std::getenv("PREX_IPC_PORT");

    if(!service) {
        return;
    }

    /* Pack the json string into a protobuf message */
    PrexMessage msg;
    Image image;
    image.set_payload(json);
    image.set_format("JSON");
    std::ostringstream image_buffer;
    image.SerializeToOstream(&image_buffer);
    msg.set_payload(image_buffer.str());
    msg.set_type(PrexMessage_MessageType_IMAGE);
    std::ostringstream msg_buffer;
    msg.SerializeToOstream(&msg_buffer);

    auto connector = util::asio::ws::Connector{ioThread->context()};
    // a `ws::Connector` wraps a `websocketpp::client`
    auto clientMq = util::asio::ws::Connector::MessageQueue{ioThread->context()};
    // a `ws::Connector::MessageQueue` wraps a `websocketpp::client::connection_ptr`

    auto use_future = boost::asio::use_future_t<std::allocator<char>>{};
    // We need this special use_future to work around an Asio bug on gcc 5+.

    connector.asyncConnect(clientMq, host, service, use_future).get();

    clientMq.asyncSend(boost::asio::buffer(msg_buffer.str()), use_future).get();

    util::log::Logger lg;

    auto ec = boost::system::error_code{};
    clientMq.close(ec);
    if (ec) { BOOST_LOG(lg) << "client message queue close: " << ec.message(); }
    connector.close(ec);
    if (ec) { BOOST_LOG(lg) << "connector close: " << ec.message(); }
}

void scatterPlot(PlotData data) {
    scatterPlot(data[0], data[1], data[2], data[3], data[4], data[5]);
}

} // namespace barobo
