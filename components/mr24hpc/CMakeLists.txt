
idf_component_register(SRCS "src/mr24hpc.c" "src/mr24hpc_parser.c" "src/mr24hpc_frame_parser.c"
                            "src/mr24hpc_state_update.c" "src/mr24hpc_standard.c" "src/mr24hpc_uof.c"
                       REQUIRES "mr24hpc_uart"
                       PRIV_REQUIRES "esp_timer"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "private_include")
