#besstandalone -d"cerr,all" -c tests/bes.conf -i tests/nc/nc4_nc_classic_compressed.nc.1.bescmd 
#besstandalone -d"cerr,all" -c tests/bes.conf -i tests/nc/fnoc1.nc.3.bescmd | getdap -M - 
besstandalone -c tests/bes.conf -i tests/nc/fnoc1.nc.3.bescmd | getdap -M - 
#besstandalone -c tests/bes.conf -i tests/nc/nc4_nc_classic_compressed.nc.3.bescmd | getdap -M -
besstandalone -c tests/bes.conf -i tests/nc/nc4_nc_classic_compressed.nc.3.bescmd | getdap -M -
#besstandalone -d"cerr,all" -c tests/bes.conf -i tests/nc/nc4_nc_classic_compressed.nc.3.bescmd | getdap -M -
#besstandalone -c tests/bes.conf -i tests/nc/fnoc1.nc.2.bescmd 


