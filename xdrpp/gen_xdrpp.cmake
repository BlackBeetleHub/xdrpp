set(OUTPUT_XDRPP_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/rpc_msg.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/rpcb_prot.hh)

add_custom_command(
    OUTPUT ${OUTPUT_XDRPP_FILES}
    COMMAND xdrc -hh -o rpc_msg.hh rpc_msg.x
    COMMAND xdrc -hh -o rpcb_prot.hh rpcb_prot.x
    MAIN_DEPENDENCY
        ${CMAKE_CURRENT_SOURCE_DIR}/rpc_msg.x
        ${CMAKE_CURRENT_SOURCE_DIR}/rpcb_prot.x
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)