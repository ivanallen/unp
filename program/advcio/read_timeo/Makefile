main:read_timeo_alarm read_timeo_select read_timeo_sockopt
read_timeo_alarm:../../include/common.h ../../util/common.cc read_timeo_alarm.cc
	g++ -o read_timeo_alarm -I../../include ../../util/common.cc read_timeo_alarm.cc
read_timeo_select:../../include/common.h ../../util/common.cc read_timeo_select.cc
	g++ -o read_timeo_select -I../../include ../../util/common.cc read_timeo_select.cc
read_timeo_sockopt:../../include/common.h ../../util/common.cc ../../util/showopts.cc read_timeo_sockopt.cc
	g++ -o read_timeo_sockopt -I../../include ../../util/common.cc ../../util/showopts.cc read_timeo_sockopt.cc
