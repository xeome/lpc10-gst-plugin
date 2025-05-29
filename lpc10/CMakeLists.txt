cmake_policy(SET CMP0115 NEW) # Enforce explicit source file extensions

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(lpc10
    analys.c
    bsynz.c
    chanwr.c
    dcbias.c
    decode.c
    deemp.c
    difmag.c
    dyptrk.c
    encode.c
    energy.c
    f2clib.c
    ham84.c
    hp100.c
    invert.c
    irc2pc.c
    ivfilt.c
    lpcini.c
    lpfilt.c
    median.c
    mload.c
    onset.c
    pitsyn.c
    placea.c
    placev.c
    preemp.c
    prepro.c
    random.c
    rcchk.c
    synths.c
    tbdm.c
    voicin.c
    vparms.c
    lpcenc.c
    lpcdec.c
)

set_target_properties(lpc10 PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(lpc10
    PUBLIC
        # link against f2c-translated code
        LINKER:--whole-archive
        f2c # Link against the f2c library
        LINKER:--no-whole-archive
    PRIVATE
        # link against the math library
        LINKER:--whole-archive
        m # Link against the math library
        LINKER:--no-whole-archive
)

# The following line seems unusual given the lpc10 directory structure.
# If lpc10.h is in the current directory (lpc10/), it will be found.
# If it's meant to find headers from the main plugin's src/ for some reason,
# it might need adjustment or could be removed if not necessary for lpc10 lib itself.
# For now, let's keep it but comment on its potential redundancy if lpc10 is self-contained.
# include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
# For clarity, if lpc10.h is in the lpc10 directory and used by lpc10 sources,
# CMAKE_CURRENT_SOURCE_DIR is implicitly an include directory for its own target.
# If lpc10.h is intended to be a public header for the lpc10 library,
# target_include_directories(lpc10 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) could be used,
# though add_subdirectory handles this reasonably well for direct linking.

# Let's remove the potentially problematic include_directories line for now,
# as the lpc10 library should be self-contained or its public headers
# handled by the main CMakeLists.txt when linking.
# The main CMakeLists.txt already adds "${CMAKE_CURRENT_SOURCE_DIR}/lpc10"
# for the GstLpc10Plugin target, which makes lpc10.h available to the plugin.
# include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
