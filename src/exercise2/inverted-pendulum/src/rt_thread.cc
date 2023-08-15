#include "inverted_pendulum/rt_thread.h"
// TODO add tracing
double RtThread::ReadSensor(int64_t cycle_time) {
  const double dt = static_cast<double>(cycle_time) / 1E9;

  const double g = 9.81;  // m / s^2
  // const double g = 0;

  // Calculate anglular acceleration
  double alpha = g / length_ * sin(current_position_);

  // Multiply by dt to get the change in angular velocity, and add the velocity command
  current_velocity_ += alpha * dt + velocity_command_;

  // Multiply by dt to get change in current position
  current_position_ += current_velocity_ * dt;

  // The pendulum has hit the floor :(
  if (abs(current_position_) > M_PI_2) {
    current_position_ = std::clamp(current_position_, -M_PI_2, M_PI_2);
    current_velocity_ = -0.4 * current_velocity_;  // Bounce!
  }

  return current_position_;
}

double RtThread::GetCommand(const double current_position, const double desired_position, int64_t cycle_time) {
  const double dt = static_cast<double>(cycle_time) / 1E9;

  // Calculate error between desired and current position
  double error = desired_position - current_position;

  // Calculate integral of error with saturation
  error_sum_ = std::clamp(error_sum_ + error * dt, -100.0, 100.0);

  // Calculate PID control law
  double position_command = kp_ * error + ki_ * error_sum_ + kd_ * (error - prev_error_) / dt;

  // Update previous error
  prev_error_ = error;

  // Divide by dt to get the output as a velocity
  // Apply velocity limits with clamp
  double velocity_command = std::clamp(position_command / dt, -M_PI_4, M_PI_4);

  return velocity_command;
}

void RtThread::WriteCommand(const double output) {
  velocity_command_ = output;
}

void RtThread::BeforeRun() {
  current_position_ = initial_position_;
}

bool RtThread::Loop(int64_t ellapsed_ns) noexcept {
  const int64_t cycle_time_ns = ellapsed_ns - prev_ns_;
  prev_ns_ = ellapsed_ns;

  if (shared_context_->reset) {
    current_position_ = initial_position_;
    current_velocity_ = 0;
    shared_context_->reset = false;
  }
  const double current_position = ReadSensor(cycle_time_ns);
  const double output = GetCommand(current_position, 0, cycle_time_ns);
  WriteCommand(output);

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  shared_context_->EmplaceData(ts, current_position);

  // Loop forever
  return false;
}
