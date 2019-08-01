#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.0.bescmd
#valgrind besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd
besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.3.bescmd
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.3.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data3.bin.data.bescmd | getdap -M -
#besstandalone -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd 
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data3.bin.data.bescmd 
#besstandalone -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data.bin.dds.bescmd 
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/test0.dat.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd


