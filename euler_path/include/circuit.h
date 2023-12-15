#ifndef EULER_PATH_CIRCUIT_H_
#define EULER_PATH_CIRCUIT_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace euler {
class Mos;

/// @brief Connects the MOS transistors.
class Net {
 public:
  Net(std::string name) : name_{std::move(name)} {}

  std::string GetName() const {
    return name_;
  }

  void AddConnection(std::weak_ptr<Mos> mos);
  const std::vector<std::weak_ptr<Mos>>& Connections() const {
    return mos_;
  }

 private:
  std::string name_;
  std::vector<std::weak_ptr<Mos>> mos_{};
};

struct Circuit {
  std::vector<std::shared_ptr<Mos>> mos;
  std::map<std::string, std::shared_ptr<Net>> nets;

  Circuit(std::vector<std::shared_ptr<Mos>> mos,
          std::map<std::string, std::shared_ptr<Net>> nets)
      : mos{std::move(mos)}, nets{std::move(nets)} {}
};

}  // namespace euler

#endif  // EULER_PATH_CIRCUIT_H_
