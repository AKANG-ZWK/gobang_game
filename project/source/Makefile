gobang:gobang.cc logger.hpp db.hpp util.hpp online.hpp room.hpp matcher.hpp server.hpp
	g++ $^ -o $@ -L/usr/lib64/mysql -lmysqlclient -ljsoncpp -lboost_system -lpthread

.PHONY:clean
clean:
	rm -f gobang