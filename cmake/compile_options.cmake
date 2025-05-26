

set(DEBUG_PRINT_OUTPUT ON CACHE BOOL "DEBUG_PRINT_OUTPUT")

function(switch_definition definition_name)
  message(STATUS "${definition_name}: ${${definition_name}}")
endfunction()

switch_definition(DEBUG_PRINT_OUTPUT)

if(DEBUG_PRINT_OUTPUT)
  # SPDLOG_DEBUG
  add_compile_definitions(DEBUG_PRINT_OUTPUT=ON)
  add_compile_definitions(SPDLOG_ACTIVE_LEVEL=1)
endif()