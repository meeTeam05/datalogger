file(GLOB_RECURSE app_srcs *.c)

idf_component_register(
    SRCS ${app_srcs}
    INCLUDE_DIRS "."
)