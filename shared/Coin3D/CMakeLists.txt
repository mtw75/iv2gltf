find_package(coin CONFIG REQUIRED)
find_package(simage CONFIG REQUIRED)

add_library(Coin3d SHARED IMPORTED GLOBAL)
target_link_libraries(Coin3d INTERFACE Coin::Coin)

set(VCPKGDIR ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})

#-- coin3d library for 3d scenegraph
if (WIN32)
set(COIN4_DLL ${VCPKGDIR}/bin/Coin4.dll)
set(COIN4_DEBUG_DLL ${VCPKGDIR}/debug/bin/Coin4d.dll)

set_property(TARGET Coin3d PROPERTY IMPORTED_LOCATION ${COIN4_DLL})
set_property(TARGET Coin3d PROPERTY IMPORTED_LOCATION_DEBUG ${COIN4_DEBUG_DLL})
set_property(TARGET Coin3d PROPERTY IMPORTED_IMPLIB ${VCPKGDIR}/lib/Coin4.lib)
set_property(TARGET Coin3d PROPERTY IMPORTED_IMPLIB_DEBUG ${VCPKGDIR}/debug/lib/Coin4d.lib) 
target_compile_definitions(Coin3d INTERFACE -DCOIN_DLL)

install(FILES ${COIN4_DLL} DESTINATION . CONFIGURATIONS Release) 
install(FILES ${COIN4_DEBUG_DLL} DESTINATION . COMPONENT ${IPO_INSTALL_COMPONENT} CONFIGURATIONS Debug)
endif()

#-- simage library for coin3d textures 
add_library(SImage SHARED IMPORTED GLOBAL)
target_link_libraries(SImage INTERFACE simage::simage)
if (WIN32)

set(SIMAGE_DLL ${VCPKGDIR}/bin/simage1.dll)
set(SIMAGE_DEBUG_DLL ${VCPKGDIR}/debug/bin/simage1d.dll)
set_property(TARGET SImage PROPERTY IMPORTED_LOCATION ${SIMAGE_DLL})
set_property(TARGET SImage PROPERTY IMPORTED_LOCATION_DEBUG ${SIMAGE_DEBUG_DLL})
set_property(TARGET SImage PROPERTY IMPORTED_IMPLIB ${VCPKGDIR}/lib/simage1.lib)
set_property(TARGET SImage PROPERTY IMPORTED_IMPLIB_DEBUG ${VCPKGDIR}/debug/lib/simage1d.lib) 
install(FILES ${SIMAGE_DLL} DESTINATION . CONFIGURATIONS Release )
install(FILES ${SIMAGE_DEBUG_DLL} DESTINATION . CONFIGURATIONS Debug)
endif()