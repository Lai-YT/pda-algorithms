#include "mos.h"

#include <utility>

#include "circuit.h"

using namespace euler;

#include <memory>

void Mos::RegisterToConnections() {
  drain_->AddConnection(shared_from_this());
  gate_->AddConnection(shared_from_this());
  source_->AddConnection(shared_from_this());
  substrate_->AddConnection(shared_from_this());
}

std::shared_ptr<Mos> Mos::Create(std::string name, Type type,
                                 std::shared_ptr<Net> drain,
                                 std::shared_ptr<Net> gate,
                                 std::shared_ptr<Net> source,
                                 std::shared_ptr<Net> substrate, double width,
                                 double length) {
  return std::shared_ptr<Mos>{new Mos{std::move(name), type, drain, gate,
                                      source, substrate, width, length}};
}

Mos::Mos(std::string name, Type type, std::shared_ptr<Net> drain,
         std::shared_ptr<Net> gate, std::shared_ptr<Net> source,
         std::shared_ptr<Net> substrate, double width, double length)
    : name_{std::move(name)},
      type_{type},
      drain_{drain},
      gate_{gate},
      source_{source},
      substrate_{substrate},
      width_{width},
      length_{length} {}
