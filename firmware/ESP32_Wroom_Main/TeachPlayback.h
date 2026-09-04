#pragma once
#include <Arduino.h>
#include <RobotArmProtocol.h>
#include "Config.h"
#include "MotorControl.h"
#include "ServoControl.h"
#include "LinkC3.h"

// Tính năng "cầm tay chỉ việc": thả trơn (freewheel) các trục stepper để
// người dùng di chuyển tay máy bằng tay, ghi lại các điểm (waypoint) theo
// góc AS5600 thật (vì bộ đếm bước mất tác dụng khi freewheel), rồi phát lại
// tuần tự (playback) khi có lực giữ trở lại.
class TeachPlayback {
 public:
  void begin();  // mount LittleFS

  void startTeach(MotorControl &motors, LinkC3 &linkC3);
  void stopTeach(MotorControl &motors, LinkC3 &linkC3);
  bool savePoint(ServoControl &servos, LinkC3 &linkC3);
  void clearWaypoints() { count_ = 0; }

  bool saveProgram(const char *name);
  bool loadProgram(const char *name);  // nạp vào bộ đệm waypoints_ hiện tại

  // Nạp thẳng 1 chuỗi điểm (vd từ lệnh RUN_CYCLE/RUN_QUEUE của web) vào
  // waypoints_ hiện tại, bỏ qua bước teach/ghi tay, để dùng lại nguyên
  // startPlayback()/update() bên dưới. points[i] có AXIS_COUNT phần tử
  // (X,Y,Z,A độ/mm + B,C độ servo), count bị kẹp trong kMaxWaypoints.
  void loadAdHocSequence(const float points[][AXIS_COUNT], uint8_t count);

  bool startPlayback(MotorControl &motors, ServoControl &servos);
  void pausePlayback(MotorControl &motors);
  void resumePlayback(MotorControl &motors, ServoControl &servos);
  void stopPlayback(MotorControl &motors);
  void setLoop(bool enable) { loop_ = enable; }

  // Gọi mỗi vòng loop(): tiến trạng thái playback khi các trục đã tới đích.
  void update(MotorControl &motors, ServoControl &servos);

  uint8_t waypointCount() const { return count_; }
  uint8_t playIndex() const { return playIdx_; }
  uint8_t playTotal() const { return count_; }
  bool isPlaying() const { return playing_; }
  bool isPaused() const { return paused_; }
  bool isTeaching() const { return teaching_; }
  bool loopEnabled() const { return loop_; }

 private:
  struct Waypoint {
    float axisPos[AXIS_COUNT];  // X,Y,Z,A (độ/mm) + B,C (độ servo)
  };

  static const uint8_t kMaxWaypoints = MAX_WAYPOINTS;
  Waypoint waypoints_[kMaxWaypoints];
  uint8_t count_ = 0;

  bool teaching_ = false;
  float teachStartUnwrapped_[AXIS_STEPPER_COUNT] = {0, 0, 0, 0};
  float teachStartPosUnits_[AXIS_STEPPER_COUNT] = {0, 0, 0, 0};

  bool playing_ = false;
  bool paused_ = false;
  bool loop_ = false;
  uint8_t playIdx_ = 0;

  bool fsReady_ = false;

  // Vị trí trục stepper ước lượng theo AS5600 sống trong lúc đang thả trơn:
  // = vị trí lúc bắt đầu teach + độ lệch góc động cơ (unwrap) quy đổi ra
  // đơn vị đầu ra của khớp.
  float liveEstimatedPositionUnits(uint8_t axisIdx, LinkC3 &linkC3) const;

  void commandWaypoint(uint8_t idx, MotorControl &motors, ServoControl &servos);
};
