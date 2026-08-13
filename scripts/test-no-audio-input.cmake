if(NOT DEFINED PROCESSOR_SOURCE OR NOT EXISTS "${PROCESSOR_SOURCE}")
    message(FATAL_ERROR "PLAY950 processor source was not provided")
endif()

file(READ "${PROCESSOR_SOURCE}" processor_source)

foreach(forbidden_token IN ITEMS "addAudioInput" "data.inputs" "Sampling Input")
    string(FIND "${processor_source}" "${forbidden_token}" token_offset)
    if(NOT token_offset EQUAL -1)
        message(FATAL_ERROR
            "PLAY950 must expose no audio input or sampling pass-through; found ${forbidden_token}")
    endif()
endforeach()

if(NOT processor_source MATCHES "numInputs != 0")
    message(FATAL_ERROR "PLAY950 bus negotiation must require zero audio inputs")
endif()

message(STATUS "PLAY950 processor exposes MIDI input and zero audio inputs")
