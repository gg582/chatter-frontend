set(SOURCES
    main.cpp
    MainWindow.cpp
    ChatterClient.cpp
    CommandCatalog.cpp
    TerminalWidget.cpp
)

set(HEADERS
    MainWindow.h
    ChatterClient.h
    CommandCatalog.h
    TerminalWidget.h
)

add_executable(chatter-frontend
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(chatter-frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(chatter-frontend PRIVATE ${QT_PACKAGE}::Widgets)
