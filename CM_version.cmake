# DarkMark (C) 2019-2023 Stephane Charette <stephanecharette@gmail.com>


FILE ( READ version.txt VERSION_TXT )
STRING ( STRIP "${VERSION_TXT}" VERSION_TXT )
STRING ( REGEX MATCHALL "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)$" OUTPUT ${VERSION_TXT} )

SET ( DM_VER_MAJOR	${CMAKE_MATCH_1} )
SET ( DM_VER_MINOR	${CMAKE_MATCH_2} )
SET ( DM_VER_PATCH	${CMAKE_MATCH_3} )
SET ( DM_VER_COMMIT	${CMAKE_MATCH_4} )

SET ( DM_VERSION ${DM_VER_MAJOR}.${DM_VER_MINOR}.${DM_VER_PATCH}-${DM_VER_COMMIT} )
MESSAGE ( "Building ver: ${DM_VERSION}" )

ADD_DEFINITIONS ( -DDARKMARK_VERSION="${DM_VERSION}" )
