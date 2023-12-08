#ifndef EULER_PATH_MOS_H_
#define EULER_PATH_MOS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace euler {

struct Net;

/// @note The MOS transistor serve as the nodes in the graph. They are connected
/// through the nets. Two MOS transistors are connected if they share a net.
class Mos : public std::enable_shared_from_this<Mos> {
 public:
  enum class Type { kP, kN };

  /// @note One must register the MOS transistors to the nets after creating.
  void RegisterToConnections();

  /// @note To ensure that the MOS transistors are created as a shared pointer.
  static std::shared_ptr<Mos> Create(std::string name, Type type,
                                     std::shared_ptr<Net> drain,
                                     std::shared_ptr<Net> gate,
                                     std::shared_ptr<Net> source,
                                     std::shared_ptr<Net> substrate);

  std::string GetName() const {
    return name_;
  }

  Type GetType() const {
    return type_;
  }

  std::shared_ptr<Net> GetDrain() const {
    return drain_;
  }

  std::shared_ptr<Net> GetGate() const {
    return gate_;
  }

  std::shared_ptr<Net> GetSource() const {
    return source_;
  }

  std::shared_ptr<Net> GetSubstrate() const {
    return substrate_;
  }

 private:
  std::string name_;
  Type type_;
  std::shared_ptr<Net> drain_;
  std::shared_ptr<Net> gate_;
  std::shared_ptr<Net> source_;
  std::shared_ptr<Net> substrate_;

  Mos(std::string name, Type type, std::shared_ptr<Net> drain,
      std::shared_ptr<Net> gate, std::shared_ptr<Net> source,
      std::shared_ptr<Net> substrate);
};

}  // namespace euler

#endif  // EULER_PATH_MOS_H_
