
idf_component_register(SRCS "src/mr24hpc.c" "src/parser.c" "src/frame_parser.c"
                            "src/state_update.c" "src/handle_frame_standard.c" "src/handle_frame_uof.c"
                       REQUIRES "mr24hpc_uart"
                       PRIV_REQUIRES "esp_timer"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "private_include")
