#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.0.bescmd
#valgrind besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.0.bescmd
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd
#besstandalone -d"cerr,all" -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd>&tmp
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.3.bescmd
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.3.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M -
#besstandalone -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd >dbl_data.bin.dods
#valgrind besstandalone -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M -
besstandalone -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M -
besstandalone -c tests/bes.conf -i tests/ff/dbl_data3.bin.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data3.bin.data.bescmd | getdap -M -
#valgrind besstandalone -c tests/bes.conf -i tests/ff/dbl_data3.bin.data.bescmd | getdap -M -
#valgrind --leak-check=full besstandalone -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd | getdap -M -
#valgrind --leak-check=full besstandalone -c tests/bes.conf -i tests/ff/test5.dab.das.bescmd
#valgrind besstandalone -c tests/bes.conf -i tests/ff/test5.dab.das.bescmd
besstandalone -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd | getdap -M -
#besstandalone -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd | getdap -M -
#valgrind besstandalone -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd | getdap -M - 
#besstandalone -d"cerr,all" -c tests/bes.conf -i tests/ff/test5.dab.data.bescmd | getdap -M - >&tmp
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data3.bin.data.bescmd 
#besstandalone -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/dbl_data.bin.dds.bescmd 
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/test0.dat.data.bescmd | getdap -M -
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd


