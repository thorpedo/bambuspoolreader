idf_component_register(
    SRCS "app_main.c" "battery.c" "bambu_keys.c" "bambu_tag.c" "display.c" "pn532.c"
    INCLUDE_DIRS "."
)

if(DEMO_TAGS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE DEMO_TAGS)
endif()
