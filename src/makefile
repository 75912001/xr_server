#XR_SERVER_PATH=编译完成后导出的路径(include&&xr_server.exe)
#选项 -rdynamic 用来通知链接器将所有符号添加到动态符号表中r(目的是能够通过使用 dlopen 来实现向后跟踪)
EXE=xr_server.exe
CC=g++ -std=c++11

#编译选项
FLAG=-Wall -g -O2 -o
LOCAL_INCLUDE=../include 

#包含的头文件
INCLUDES=-I/usr/include -I/usr/include/libxml2 -I$(XR_PATH)/include -I$(LOCAL_INCLUDE)

LINK=-ldl -rdynamic -lxml2 -lpthread -lxr -Wl,-rpath,$(XR_PATH)

LIBS=-L/usr/lib64 -L/usr/lib -L$(XR_PATH)

#编译的代码文件
LOCAL_SRCS =$(wildcard ./*.cpp)

$(EXE):
	$(CC) $(LOCAL_SRCS) $(INCLUDES) $(LIBS) $(LINK) $(FLAG) $@
	rm $(XR_SERVER_PATH)/include -rf
	mkdir -p $(XR_SERVER_PATH)
	cp $(EXE) $(XR_SERVER_PATH)
	cp -rf $(LOCAL_INCLUDE) $(XR_SERVER_PATH)
	rm -f $(EXE)


