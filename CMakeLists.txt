cmake_minimum_required(VERSION 2.8)
project(CtrBootManager)
enable_language(ASM)

#set(CMAKE_VERBOSE_MAKEFILE on)
set(NETLOAD_IP "172.20.138.31")

# Note that you must copy the cmake folder and the DevkitArm3DS.cmake file in this directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake) # Add the cmake folder to the modules paths, so that we can use the tools and find_package for ctrulib
include(Tools3DS) # Include all the macros and tools needed for 3ds development.

## source files
file(GLOB_RECURSE SOURCE_FILES
    source/*
)
file(GLOB_RECURSE SOURCE_FILES_ARM9
    source/arm9/source/*
)
file(GLOB_RECURSE SOURCE_FILES_SCREENINIT
    screeninit/source/*
)
file(GLOB_RECURSE SOURCE_FILES_LOADER
    loader/source/*
)

# Filter
file(GLOB FILTER_FILES
    ${SOURCE_FILES_ARM9}
    source/arm9/stage2/payload/source/*
    source/CakeBrah/source/main.c
    source/CakeBrah/source/hid.c
    source/CakeBrah/source/libkhax/demo/*
)
list(REMOVE_ITEM SOURCE_FILES
    ${FILTER_FILES}
)


###############################
# Fix Clion define manually...
###############################
set(CMAKE_C_FLAGS "")
set(CMAKE_C_FLAGS "-DARM9")
#set(CMAKE_C_FLAGS "-DARM11")

###################
# CtrBootManager
###################

add_executable(CtrBootManager ${SOURCE_FILES}) # Create the elf file
set(ARCH "-march=armv6k -mtune=mpcore -mfloat-abi=hard")
set_target_properties(CtrBootManager PROPERTIES COMPILE_FLAGS
        "-mword-relocations ${ARCH} -fomit-frame-pointer -ffast-math -O2 -mtp=soft -D_3DS -DARM11 -DHAVE_CONFIG_H -DINI_USE_STACK")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DKA_SUGGESTED_CXX_FLAGS}")

find_package(CTRULIB REQUIRED) # Look for ctrulib
target_link_libraries(CtrBootManager 3ds::ctrulib)
target_include_directories(CtrBootManager PUBLIC ${DEVKITPRO}/portlibs/armv6k/include)
target_link_libraries(CtrBootManager ${DEVKITPRO}/portlibs/armv6k/lib/libz.a)
target_include_directories(CtrBootManager PUBLIC source source/hb_menu source/CakeBrah/include)
target_include_directories(CtrBootManager PUBLIC source/CakeBrah/source/libkhax)

add_3dsx_target(CtrBootManager NO_SMDH) # Generate the .3dsx and .smdh from the CtrBootManager target
add_netload_target(CtrBootManager_3dslink CtrBootManager)
add_citra_target(CtrBootManager_Citra CtrBootManager)
#set_target_properties(CtrBootManager PROPERTIES COMPILE_FLAGS "-DCITRA")

###################
# Screen init
###################
add_executable(screeninit ${SOURCE_FILES_SCREENINIT})
set(ASFLAGS "-mcpu=mpcore -mfloat-abi=hard")
set(CFLAGS "-Wall -Wextra -MMD -MP -mthumb -mthumb-interwork $(ASFLAGS) -fno-builtin -std=c11 -Wno-main -O2 -flto -ffast-math")
set_target_properties(screeninit PROPERTIES COMPILE_FLAGS ${CFLAGS})
set_source_files_properties(screeninit/source/start.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp ${CFLAGS}")
set_target_properties(screeninit PROPERTIES LINK_FLAGS
        "-nostdlib -nostartfiles -g -mthumb -mthumb-interwork -Wl,-Map,screeninit.map -T ${CMAKE_SOURCE_DIR}/screeninit/linker.ld ")
add_a9lh_target(screeninit_a9lh screeninit)

###################
# FIRM loader
###################
add_executable(loader ${SOURCE_FILES_LOADER})
set(ASFLAGS "-mcpu=mpcore -mfloat-abi=hard")
set(CFLAGS "-Wall -Wextra -MMD -MP -mthumb -mthumb-interwork $(ASFLAGS) -fno-builtin -std=c11 -Wno-main -O2 -flto -ffast-math")
set_target_properties(loader PROPERTIES COMPILE_FLAGS ${CFLAGS})
set_source_files_properties(loader/source/start.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp ${CFLAGS}")
set_target_properties(loader PROPERTIES LINK_FLAGS
        "-nostdlib -nostartfiles -g -mthumb -mthumb-interwork -Wl,-Map,loader.map -T ${CMAKE_SOURCE_DIR}/loader/linker.ld ")
add_a9lh_target(loader_a9lh loader)

###################
# CtrBootManager9
###################
set(SOURCE_FILES_ARM9 ${SOURCE_FILES} ${SOURCE_FILES_ARM9})
file(GLOB FILTER_FILES_ARM9
        source/arm9/source/fatfs/option/syscall.c
        source/arm9/stage2/payload/source/*
        source/CakeBrah/source/*
        source/CakeBrah/source/libkhax/*
        source/hb_menu/*
        source/menu_netloader.c
        source/menu_netloaderarm9.c
)
list(REMOVE_ITEM SOURCE_FILES_ARM9
    ${FILTER_FILES_ARM9}
)

add_executable(CtrBootManager9 ${SOURCE_FILES_ARM9})
set(ARCH9 "-march=armv5te -mtune=arm946e-s")
set(ARM9_FLAGS "-mthumb -mthumb-interwork -g ${ARCH9} -Wall -O2 -fomit-frame-pointer -ffast-math -std=c99 -DARM9 -DINI_USE_STACK -DEXEC_BOOTSTRAP -DHAVE_CONFIG_H")
set_target_properties(CtrBootManager9 PROPERTIES COMPILE_FLAGS ${ARM9_FLAGS})
set_source_files_properties(source/arm9/source/bs-start.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp ${ARM9_FLAGS}")
set_target_properties(CtrBootManager9 PROPERTIES LINK_FLAGS
        "-nostartfiles -g -mthumb -mthumb-interwork -Wl,-Map,CtrBootManager9.map -T ${CMAKE_SOURCE_DIR}/source/arm9/bootstrap.ld ")
target_include_directories(CtrBootManager9 PUBLIC source/arm9/source source/arm9/source/fatfs)
target_embed_file(CtrBootManager9 source/arm9/stage2/data/stage2.bin)
add_a9lh_target(CtrBootManager_a9lh CtrBootManager9)
