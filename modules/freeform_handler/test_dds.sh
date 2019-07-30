#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.0.bescmd
#valgrind besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd
#besstandalone -c tests/bes.conf -i tests/ff/avhrr.dat.3.bescmd | getdap -M -
besstandalone -c tests/bes.conf -i tests/ff/dbl_data.bin.data.bescmd | getdap -M - >tmp
#besstandalone -d"cerr,ff" -c tests/bes.conf -i tests/ff/avhrr.dat.1.bescmd

