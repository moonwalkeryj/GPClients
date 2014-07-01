cmake_minimum_required(VERSION 2.8 FATAL_ERROR) 

project(pcl_gp_client)
set(EXE_NAME pcl_gp_client)
set(PROJECT_NAME pcl_gp_client)
SET(CMAKE_CXX_FLAGS_DEBUG "/MTd")
set(DEFAULT FALSE) # I dont know the usage
set(BEAGLE_INCLUDE_DIRS C:/GP/beagle-3.0.3/PACC
						C:/GP/beagle-3.0.3/beagle/include
						C:/GP/beagle-3.0.3/beagle/GA/include
						C:/GP/beagle-3.0.3/beagle/GP/include
						)
include_directories(${BEAGLE_INCLUDE_DIRS})

set(GP_LIBS C:/GP/beagle-3.0.3/MSVCPP/pacc-math/Debug/pacc-math.lib
			C:/GP/beagle-3.0.3/MSVCPP/pacc-util/Debug/pacc-util.lib
			C:/GP/beagle-3.0.3/MSVCPP/pacc-xml/Debug/pacc-xml.lib
			C:/GP/beagle-3.0.3/MSVCPP/beagle/Debug/beagle.lib
			C:/GP/beagle-3.0.3/MSVCPP/beagle-GP/Debug/beagle-GP.lib
			C:/GP/beagle-3.0.3/MSVCPP/beagle-GA/Debug/beagle-GA.lib
			)
# Package for PCL
#set(ADDITIONAL_LIBS)
#find_package(PCL 1.2 REQUIRED)
#include_directories(${PCL_INCLUDE_DIRS})
##link_directories(${PCL_LIBRARY_DIRS})
##add_definitions(${PCL_DEFINITIONS})
#list(APPEND ADDITIONAL_LIBS ${PCL_LIBRARIES})

# Package for OpenCV
#include_directories("C:/OpenCV/opencv/build/include")
#set(OpenCV_LIBS C:/OpenCV/opencv/build/x64/vc10/lib/opencv_gpu246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_contrib246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_legacy246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_objdetect246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_calib3d246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_features2d246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_video246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_highgui246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_ml246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_nonfree246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_imgproc246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_flann246.lib
#                C:/OpenCV/opencv/build/x64/vc10/lib/opencv_core246.lib
#)
include(C:/OpenCV/build/install/OpenCVConfig.cmake)
set(OpenCV "C:/OpenCV/build/cmake")
find_package(OpenCV REQUIRED)

# Package for Qt
find_package(Qt4 REQUIRED)
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtNetwork QtXml  REQUIRED)
include(${QT_USE_FILE})
list(APPEND ADDITIONAL_LIBS ${QT_LIBRARIES})

if(BUILD)
  # Qt
  if(NOT QT4_FOUND)
    message(WARNING "Qt4 is needed for the app! It will not be built!")
    set(BUILD FALSE)
  endif()

  # OpenGL
  find_package(OpenGL REQUIRED)
  if(OPENGL_FOUND AND OPENGL_GLU_FOUND)
    list(APPEND ADDITIONAL_LIBS ${OPENGL_LIBRARIES})
  else()
    message(WARNING "OpenGL & GLU are needed for the app! It will not be built!")
    set(BUILD FALSE)
  endif()
endif()
################################################################################

set(INCS
)

set(IMPL_INCS
)

set(SRCS
	main.cpp    
	mainwindow.cpp
	Widgets/qcustomplot.cpp
	gpthread.cpp
        gpudp.cpp
)
# Qt Moc Files ( Classes using Macro Q_OBJECT)
set(MOC_QCUSTOM_PLOT_INC 		Widgets/qcustomplot.h)
set(MOC_GP_THREAD_INC                   gpthread.h)
set(MOC_GP_UDP_INC                      gpudp.h)
set(MOC_MAIN_WINDOW_INC 		mainwindow.h)
set(UI_MAIN_WINDOW                      mainwindow.ui)

# Qt
# http://qtnode.net/wiki/Qt4_with_cmake
# http://qt-project.org/quarterly/view/using_cmake_to_build_qt_projects
# set(QT_USE_QTOPENGL TRUE)

QT4_WRAP_CPP(MOC_QCUSTOM_PLOT_SRC      	${MOC_QCUSTOM_PLOT_INC})
QT4_WRAP_CPP(MOC_GP_THREAD_SRC      	${MOC_GP_THREAD_INC})
QT4_WRAP_CPP(MOC_GP_UDP_SRC      	${MOC_GP_UDP_INC})
QT4_WRAP_CPP(MOC_MAIN_WINDOW_SRC        ${MOC_MAIN_WINDOW_INC})
QT4_WRAP_UI(UI_MAIN_WINDOW_INC          ${UI_MAIN_WINDOW})

list(APPEND INCS    ${MOC_QCUSTOM_PLOT_INC}
                    ${MOC_GP_THREAD_INC}
                    ${MOC_GP_UDP_INC}
                    ${MOC_MAIN_WINDOW_INC}
                    ${UI_MAIN_WINDOW_INC}
)

list(APPEND SRCS    ${MOC_QCUSTOM_PLOT_SRC}
                    ${MOC_GP_THREAD_SRC}
                    ${MOC_GP_UDP_SRC}
                    ${MOC_MAIN_WINDOW_SRC}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}) # For the ui files
add_executable(${EXE_NAME} ${SRCS} ${INCS} ${IMPL_INCS})
target_link_libraries(${EXE_NAME} ${SUBSYS_LIBS} ${ADDITIONAL_LIBS} ${OpenCV_LIBS} ${GP_LIBS} GlU32 )
