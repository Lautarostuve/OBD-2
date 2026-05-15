idf_component_register(SRCS "web_server.c" "can_driver.c" "time_manager.c" "http_sender.c" "wifi_manager.c" "main.c" "pid_parser.c" "json_builder.c" "data_buffer.c"
                        INCLUDE_DIRS ".")
set(PARTITION_TABLE_CSV_PATH "partitions.csv")