# Algorithms of Physical Design Automation

This repository contains several subprojects, covering the basic physical design cycle.

| Subproject                    | Brief                                                                                                                                                                                     |
| ----------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [Partitioning](./partition/)  | Performs bi-partitioning on a network consisting of a set of cells connected by a set of nets using the _Fiduccia-Mattheyses Algorithm_.                                                  |
| [Floorplanning](./floorplan/) | Conducts [slicing floorplanning](https://en.wikipedia.org/wiki/Floorplan_(microelectronics)#Sliceable_floorplans) using the slicing tree structure and the simulated annealing algorithm. |
| [Placement](./euler_path/)    | Identifies _[Hamilton Paths](https://en.wikipedia.org/wiki/Hamiltonian_path)_ within the HSPICE netlist.                                                                                  |
| [Routing](./routing/)         | Addresses the 2-layer channel routing problem by implementing the _Constraint Left-Edge algorithm_.                                                                                       |

## License

Please check out the LICENSE file in each subproject.
