add_executable(waveletanalysis
    main.c
    common.c
    frequencyGrid.c
    trajectory.c
    analysis.c
    results.c
    config.c
    output.c
    app.c
    main.c
)

target_include_directories(waveletanalysis PRIVATE
    # ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
