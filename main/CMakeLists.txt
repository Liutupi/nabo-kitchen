set(KITCHEN_SOURCES
    "kitchen/kitchen_bundle.c"
    "kitchen/kitchen_canvas.c"
    "kitchen/kitchen_catalog.c"
    "kitchen/kitchen_input.c"
    "kitchen/kitchen_journal.c"
    "kitchen/kitchen_kan.c"
    "kitchen/kitchen_kfn.c"
    "kitchen/kitchen_krc.c"
    "kitchen/kitchen_ksp.c"
    "kitchen/kitchen_lists.c"
    "kitchen/kitchen_metadata.c"
    "kitchen/kitchen_model.c"
    "kitchen/kitchen_recommend.c"
    "kitchen/kitchen_save.c"
    "kitchen/kitchen_session.c"
    "kitchen/kitchen_timer.c"
    "kitchen/kitchen_ui.c"
    "kitchen/kitchen_view.c"
    "kitchen/kitchen_view_cooking.c"
    "kitchen/kitchen_view_home.c"
    "kitchen/kitchen_view_lists.c"
    "kitchen/nabo_kitchen_mixer.c"
    "kitchen/nabo_kitchen_store.c"
)
idf_component_register(
    SRCS "main.c" "offline_kitchen.c" ${KITCHEN_SOURCES}
    INCLUDE_DIRS "." "kitchen"
    REQUIRES bsp esp_lcd esp_timer nvs_flash esp_partition spi_flash json esp_driver_usb_serial_jtag vfs esp_hw_support
)
target_add_binary_data(${COMPONENT_LIB} "${CMAKE_CURRENT_LIST_DIR}/kitchen/nabo_kitchen_startup.pcm" BINARY)
set_source_files_properties(${KITCHEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-O2")
# Stage the exact bundle under the build directory: the verified archive accepts
# only safe relative flash_args and reconfigures when the source bundle changes.
configure_file("${CMAKE_SOURCE_DIR}/assets/kitchen/kitchen_bundle.bin"
               "${CMAKE_BINARY_DIR}/kitchen_bundle.bin" COPYONLY)
esptool_py_flash_to_partition(flash "kitchen_assets" "${CMAKE_BINARY_DIR}/kitchen_bundle.bin")
