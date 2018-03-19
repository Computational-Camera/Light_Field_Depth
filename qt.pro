TEMPLATE	= app
CONFIG		+= qt debug
QT          += widgets
CONFIG      += link_pkgconfig

QT_CONFIG -= no-pkg-config
CONFIG    += link_pkgconfig

PKGCONFIG += opencv

HEADERS		= src/*.h\
			  src/gco/*.h\
			  src/WMF/*.h			  
SOURCES		= src/*.cpp\
			  src/gco/*.cpp\
                             
TARGET		= lf2depth
INCLUDEPATH +=  /usr/include/hdf5/serial/
QMAKE_CXXFLAGS += -O3 -march=native -std=c++11 -m64 -pipe -ffast-math -Waggressive-loop-optimizations -Wall -fpermissive -fopenmp
linux-g++: QMAKE_CXXFLAGS += -O99 
LIBS +=    -lhdf5_serial -lz -lX11 -fopenmp
LIBS +=  -L/usr/lib/x86_64-linux-gnu
LIBS +=  -L/usr/local/lib



OBJECTS_DIR = obj
DESTDIR     = bin
#-lgsl -lgslcblas
