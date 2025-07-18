cmake_minimum_required(VERSION 3.20.0)

target_include_directories(app PRIVATE .)

file(GLOB CERTS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
target_sources(app PRIVATE ${CERTS_SOURCES})

# CONFIG_WIFI_CREDENTIALS still does not support enterprise as per
# https://github.com/zephyrproject-rtos/zephyr/pull/88653
# https://github.com/zephyrproject-rtos/zephyr/issues/91790
# external/zephyr/include/zephyr/net/wifi_credentials.h#L115
# Copied from external/zephyr/subsys/net/lib/wifi_credentials/CMakeLists.txt
if(DEFINED CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE)
  # Wi-Fi Enterprise test certificates handling
  set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
  set(gen_dir ${gen_inc_dir}/wifi_certs)
  if(NOT DEFINED WIFI_TEST_CERTS_DIR)
    set(WIFI_TEST_CERTS_DIR ${ZEPHYR_BASE}/samples/net/wifi/test_certs/rsa3k)
  endif()
  # Create output directory for test certs
  file(MAKE_DIRECTORY ${gen_dir})

  # convert .pem files to array data at build time
  zephyr_include_directories(${gen_inc_dir})

  foreach(cert_file IN ITEMS
      ${WIFI_TEST_CERTS_DIR}/client.pem
      ${WIFI_TEST_CERTS_DIR}/client-key.pem
      ${WIFI_TEST_CERTS_DIR}/ca.pem
      ${WIFI_TEST_CERTS_DIR}/client2.pem
      ${WIFI_TEST_CERTS_DIR}/client-key2.pem
      ${WIFI_TEST_CERTS_DIR}/ca2.pem
      )

  if(EXISTS ${cert_file})
      get_filename_component(cert_name ${cert_file} NAME)
      generate_inc_file_for_target(
          app
          ${cert_file}
          ${gen_dir}/${cert_name}.inc
          )
    else()
      get_filename_component(cert_name ${cert_file} NAME)
      file(WRITE ${gen_dir}/${cert_name}.inc "// Empty file generated because ${cert_file} does not exist\n")
    endif()
  endforeach()

  # Add explicit dependency on app target for ZEPHYR_CURRENT_LIBRARY, so these
  # headers are generated at the correct point in the build
  add_dependencies(${ZEPHYR_CURRENT_LIBRARY} app)
endif()
