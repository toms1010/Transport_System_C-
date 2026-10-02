cmake_minimum_required(VERSION 3.16)

project(StudentTransport
        VERSION 2.0.0
        DESCRIPTION "Student Transport Management System"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# Lets editors (VS Code C/C++, clangd, CLion) pick up the exact include paths
# and flags this project builds with, instead of guessing.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

# The domain layer: models, repositories, services and utilities. It has no
# dependency on the console front end, so both the CLI and the Qt GUI link it.
# This is what lets the GUI reuse the tested logic verbatim.
set(ST_DOMAIN_SOURCES
    src/utils/TextUtils.cpp
    src/utils/CsvUtils.cpp
    src/utils/FileUtils.cpp
    src/utils/Security.cpp
    src/utils/Validation.cpp
    src/models/User.cpp
    src/models/FeeBreakdown.cpp
    src/repositories/UserRepository.cpp
    src/repositories/PaymentRepository.cpp
    src/repositories/ComplaintRepository.cpp
    src/services/AuthService.cpp
    src/services/RouteService.cpp
    src/services/SeatService.cpp
    src/services/PaymentService.cpp
    src/services/ComplaintService.cpp
    src/services/UserService.cpp
)

add_library(transport_domain STATIC ${ST_DOMAIN_SOURCES})
target_include_directories(transport_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# The console front end on top of the domain layer.
set(ST_CORE_SOURCES
    src/ui/ConsoleUI.cpp
    src/ui/Menu.cpp
    src/ui/Screens.cpp
    src/ui/CommonScreens.cpp
    src/ui/StudentScreens.cpp
    src/ui/StaffScreens.cpp
    src/ui/AdminScreens.cpp
)

add_library(transport_core STATIC ${ST_CORE_SOURCES})
target_link_libraries(transport_core PUBLIC transport_domain)

add_executable(transport_system src/main.cpp)
target_link_libraries(transport_system PRIVATE transport_core)

# ---------------------------------------------------------------------------
# Qt Widgets desktop front end.
#
# Guarded so the console build still works on machines without Qt installed:
# configure with -DST_BUILD_GUI=OFF to skip it entirely.
# ---------------------------------------------------------------------------
option(ST_BUILD_GUI "Build the Qt Widgets desktop application" ON)

if(ST_BUILD_GUI)
    find_package(Qt6 6.2 COMPONENTS Widgets PrintSupport QUIET)

    if(Qt6_FOUND)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)

        set(ST_GUI_SOURCES
            gui/main.cpp
            gui/Theme.cpp
            gui/Notify.cpp
            gui/AppContext.cpp
            gui/Widgets.cpp
            gui/controllers/AuthController.cpp
            gui/controllers/RouteController.cpp
            gui/controllers/SeatController.cpp
            gui/controllers/PaymentController.cpp
            gui/controllers/ComplaintController.cpp
            gui/controllers/NoticeController.cpp
            gui/controllers/StudentController.cpp
            gui/controllers/StaffController.cpp
            gui/controllers/Controllers.cpp
            gui/LoginWindow.cpp
            gui/RegistrationWizard.cpp
            gui/PublicWindow.cpp
            gui/Pickers.cpp
            gui/MainWindow.cpp
            gui/DashboardPage.cpp
            gui/RoutesPage.cpp
            gui/SeatMapPage.cpp
            gui/PaymentsPage.cpp
            gui/RosterPage.cpp
            gui/ComplaintsPage.cpp
            gui/NoticesPage.cpp
            gui/ProfilePage.cpp
            gui/StaffAccountsPage.cpp
            gui/ReportsPage.cpp
            gui/SettingsPage.cpp
            gui/HelpPage.cpp
            gui/AllotmentPage.cpp
            resources/resources.qrc
        )

        set(ST_GUI_HEADERS
            gui/Theme.hpp
            gui/Notify.hpp
            gui/AppContext.hpp
            gui/Widgets.hpp
            gui/LoginWindow.hpp
            gui/RegistrationWizard.hpp
            gui/PublicWindow.hpp
            gui/Pickers.hpp
            gui/MainWindow.hpp
            gui/DashboardPage.hpp
            gui/RoutesPage.hpp
            gui/SeatMapPage.hpp
            gui/PaymentsPage.hpp
            gui/RosterPage.hpp
            gui/ComplaintsPage.hpp
            gui/NoticesPage.hpp
            gui/ProfilePage.hpp
            gui/StaffAccountsPage.hpp
            gui/ReportsPage.hpp
            gui/SettingsPage.hpp
            gui/HelpPage.hpp
            gui/AllotmentPage.hpp
        )

        add_executable(transport_gui ${ST_GUI_SOURCES} ${ST_GUI_HEADERS})
        target_link_libraries(transport_gui PRIVATE transport_domain Qt6::Widgets Qt6::PrintSupport)
        target_include_directories(transport_gui PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/gui
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

        install(TARGETS transport_gui RUNTIME DESTINATION bin)

        # Optional development tool that regenerates the screenshots in
        # README.md and docs/. Off by default: it links every page only to drive
        # them offscreen, which is not something the application needs.
        option(ST_BUILD_GUI_CAPTURE "Build the screenshot capture tool" OFF)
        if(ST_BUILD_GUI_CAPTURE)
            set(ST_GUI_CAPTURE_SOURCES ${ST_GUI_SOURCES})
            list(REMOVE_ITEM ST_GUI_CAPTURE_SOURCES gui/main.cpp)

            add_executable(gui_capture tools/gui_capture.cpp ${ST_GUI_CAPTURE_SOURCES}
                                        ${ST_GUI_HEADERS})
            target_link_libraries(gui_capture PRIVATE transport_domain Qt6::Widgets
                                                Qt6::PrintSupport)
            target_include_directories(gui_capture PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/gui
                ${CMAKE_CURRENT_SOURCE_DIR}/include
            )
        endif()
    else()
        message(STATUS "Qt6 Widgets not found - skipping the transport_gui target.")
    endif()
endif()

enable_testing()

set(ST_TEST_SOURCES
    tests/test_auth.cpp
    tests/test_routes.cpp
    tests/test_seats.cpp
    tests/test_payments.cpp
    tests/test_complaints.cpp
    tests/test_robustness.cpp
)

foreach(test_src ${ST_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} tests/TestHarness.cpp)
    target_link_libraries(${test_name} PRIVATE transport_core)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

install(TARGETS transport_system RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/student-transport/data)