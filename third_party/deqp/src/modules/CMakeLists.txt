# dEQP Modules
add_subdirectory(glshared)
add_subdirectory(gles2)
add_subdirectory(gles3)
add_subdirectory(gles31)
add_subdirectory(egl)

# Misc
add_subdirectory(internal)

# Pass DEQP_MODULE_LIBRARIES and DEQP_MODULE_ENTRY_POINTS
set(DEQP_MODULE_LIBRARIES ${DEQP_MODULE_LIBRARIES} PARENT_SCOPE)
set(DEQP_MODULE_ENTRY_POINTS ${DEQP_MODULE_ENTRY_POINTS} PARENT_SCOPE)
