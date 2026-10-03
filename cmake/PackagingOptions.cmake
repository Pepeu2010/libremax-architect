if(CPACK_GENERATOR STREQUAL "DEB" AND CPACK_LMX_BUNDLED_RUNTIME_DIR)
  # Scan all CPU/Python libraries, retaining optional GPU backends in the payload
  # without installing AMD/NVIDIA/Intel drivers on every user's machine.
  set(SHLIBDEPS_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/../scripts/debian-shlibdeps.py")
endif()
