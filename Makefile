
CC=g++ -g -O3 -Wall -Werror
DPDK=1
CFLAGS=-DMAX_CPUS=1 -lm -lpthread

# Add arch-specific optimization
ifeq ($(shell uname -m),x86_64)
LIBS += -m64
endif

# mtcp library and header 
MTCP_FLD    =../mtcp/mtcp/
MTCP_INC    =-I${MTCP_FLD}/include
MTCP_LIB    =-L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a

UTIL_FLD = ../mtcp/util
UTIL_INC = -I${UTIL_FLD}/include
UTIL_OBJ =

# util library and header
INC = -I./include/ ${UTIL_INC} ${MTCP_INC} -I${UTIL_FLD}/include
LIBS = ${MTCP_LIB}

ifeq ($(DPDK),1)
DPDK_MACHINE_LINKER_FLAGS=$${RTE_SDK}/$${RTE_TARGET}/lib/ldflags.txt
DPDK_MACHINE_LDFLAGS=$(shell cat ${DPDK_MACHINE_LINKER_FLAGS})
LIBS += -g -O3 -pthread -lrt -march=native ${MTCP_FLD}/lib/libmtcp.a -lnuma -lmtcp -lpthread -lrt -ldl -lgmp -L${RTE_SDK}/${RTE_TARGET}/lib ${DPDK_MACHINE_LDFLAGS}
endif


ifeq ($V,) # no echo
	export MSG=@echo
	export HIDE=@
else
	export MSG=@\#
	export HIDE=
endif


all: build/main_mtcp

build/main_mtcp.o: src/main_mtcp.cpp
	$(MSG) "   CC $<"
	${CC} -g -c $< -o $@ ${CFLAGS} ${INC}

%: %.o
	$(MSG) "   LD $<"
	${CC} $< ${INC} ${UTIL_OBJ} -g -o $@ ${LIBS} 

clean:
	rm -f *~ build/main_mtcp build/main_mtcp.o log_*

distclean: clean
	rm -rf Makefile
