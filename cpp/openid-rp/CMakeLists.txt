PROJECT(openidrp)
#INCLUDE(../../../../cmake-build-tools/cmake/local_hostname.cmake)
#INCLUDE(../../../cmake-local-config.cmake)

SET(Boost_ADDITIONAL_VERSIONS "1.42" )
#SET(Boost_ADDITIONAL_VERSIONS "1.49" )

find_package( Boost  COMPONENTS system thread filesystem date_time )
   if(Boost_FOUND)
      MESSAGE("Found Boost libraries: " ${Boost_LIB_VERSION} )
   endif()


set(  SRCBASEROOT "/home/maxim/SRC" )

#cmake_minimum_required(VERSION 2.4)
SET( openidrpsrv_SRCs
    ${SRCBASEROOT}/FFN/cppnet/asio_http_server.cpp
    ${SRCBASEROOT}/FFN/cppnet/asio_io_service_wrapper.cpp
    ${SRCBASEROOT}/FFN/cppnet/url.cpp
    ./rpsrv.cpp
    ${SRCBASEROOT}/MM-Exper/deco/deco1.cpp
    ${SRCBASEROOT}/MM_classes/src/util/sysfuncs.cpp

      ./rpsrv.cpp
      ./rp_sqlite.cpp  
 )

SET( openidrpcmd_SRCs
      ./rpcmd.cpp
      ./rp_sqlite.cpp  
   )

SET ( MONGO_CXX_DRIVER_PATH 
        "/media/40GB/data/maxim/src/mongo-cxx-driver-v2.0"
    )
SET ( LIBOPKELE_PATH 
        "/media/40GB/data/maxim/src/libopkele"
    )


INCLUDE_DIRECTORIES( 
		/usr/local/include/boost-${Boost_LIB_VERSION} 	
		/opt/boost/include/boost-${Boost_LIB_VERSION} 	
		${SRCBASEROOT}/MM_classes 
		${SRCBASEROOT}/MM_classes/include 
		${SRCBASEROOT}/MM_classes/src 
		${SRCBASEROOT}/MM-Exper
		${SRCBASEROOT}/MM-Exper/mxmsrv01 
		${SRCBASEROOT}/MM-Exper/mxmsrv01/m2f 
		${SRCBASEROOT}/FFN/cppnet 
		${SRCBASEROOT}/FFN 

        ${MONGO_CXX_DRIVER_PATH}
        ${LIBOPKELE_PATH}/include
        ../httpsrv
        ..
        ../../../../mxmz-json-cpp-lib/branches/main/src

	)



# Linux console
 SET( openidrp_COMPILE_FLAGS "-O3 -D_REENTRANT -std=c++0x -Dlinux -Dunix" ) 
 SET( openidrp_LINK_FLAGS "-pthread  -lsqlite3 -lopkele -luuid" ) 
 SET( openidrp_LINK_LIBRARIES 
#      ${MONGO_CXX_DRIVER_PATH}/libmongoclient.a
      ${Boost_SYSTEM_LIBRARY_RELEASE} 
      ${Boost_FILESYSTEM_LIBRARY_RELEASE} 
      ${Boost_THREAD_LIBRARY_RELEASE} 
      ${Boost_DATE_TIME_LIBRARY_RELEASE} 
	
	${openidrp_LINK_FLAGS}
    )



SET_SOURCE_FILES_PROPERTIES( ${openidrpcmd_SRCs} PROPERTIES COMPILE_FLAGS "${openidrp_COMPILE_FLAGS}" )
SET_SOURCE_FILES_PROPERTIES( ${openidrpsrv_SRCs} PROPERTIES COMPILE_FLAGS "${openidrp_COMPILE_FLAGS}" )


ADD_EXECUTABLE( "openidrpcmd" ${openidrpcmd_SRCs} )
#SET_TARGET_PROPERTIES( "openidrpcmd" PROPERTIES LINK_FLAGS "${openidrp_LINK_FLAGS}" )
TARGET_LINK_LIBRARIES( "openidrpcmd" ${openidrp_LINK_LIBRARIES} )
 

ADD_EXECUTABLE( "openidrpsrv" ${openidrpsrv_SRCs} )
#SET_TARGET_PROPERTIES( "openidrpsrv" PROPERTIES LINK_FLAGS "${openidrp_LINK_FLAGS}" )
TARGET_LINK_LIBRARIES( "openidrpsrv" ${openidrp_LINK_LIBRARIES} )




