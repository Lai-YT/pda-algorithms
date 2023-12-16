.subckt two n1 n2 n5 vdd vss
mM1 n3 n1 vdd vdd pmos_rvt w=81.0n  l=20n nfin=3
mM2 n4 n2 vdd vdd pmos_rvt w=81.0n  l=20n nfin=3
mM3 n6 n5 n7  n7  pmos_rvt w=81.0n  l=20n nfin=3
mM4 n3 n1 vss vss nmos_rvt w=162.0n l=20n nfin=6
mM5 n3 n2 vss vss nmos_rvt w=162.0n l=20n nfin=6
mM6 n6 n5 n8  n8  nmos_rvt w=162.0n l=20n nfin=6
.ENDS