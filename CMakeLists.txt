cmake_minimum_required(VERSION 3.5)

project(micro_brico_app VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        connect_db.cpp
        datamodel.cpp
        datamodel.h
        utilisateur.cpp
        utilisateur.h
        tests/automation/test_database_config.cpp
        tests/automation/test_database_config.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(micro_brico_app
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

        datamodel.h
        connect_db.h connect_db.cpp
        datamodel.cpp
        utilisateur.h utilisateur.cpp


        popups/popupaddusers.h popups/popupaddusers.cpp popups/popupaddusers.ui
        popups/login_connect.h popups/login_connect.cpp popups/login_connect.ui
        popups/delete_user.h popups/delete_user.cpp popups/delete_user.ui
        logger/logbrowser.h logger/logbrowser.cpp
        logger/logbrowserdialog.h logger/logbrowserdialog.cpp
        popups/poppupaddkit.h popups/poppupaddkit.cpp popups/poppupaddkit.ui
        kit.h kit.cpp
        item.h item.cpp
        money.h money.cpp
        resa.h resa.cpp
        utils.h utils.cpp
        popups/popupsortirresa.h popups/popupsortirresa.cpp popups/popupsortirresa.ui
        sortie.h sortie.cpp
        log.h log.cpp
        logger/log_user_kit_appli.h logger/log_user_kit_appli.cpp logger/log_user_kit_appli.ui
        types.h
        popups/popuppchangeitemquantities.h popups/popuppchangeitemquantities.cpp popups/popuppchangeitemquantities.ui







    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET micro_brico_app APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(micro_brico_app SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(micro_brico_app
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(micro_brico_app PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

find_package(Qt6 REQUIRED COMPONENTS Sql)
target_link_libraries(micro_brico_app PRIVATE Qt6::Sql)

option(MICROBRICO_BUILD_TESTS "Build the database-free test foundation" ON)

if(MICROBRICO_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)

    # This target intentionally contains no QSqlDatabase usage.  It verifies
    # that a future TC-001 run can only be pointed at an explicitly named test
    # database before an IHM/integration test is allowed to start.
    add_executable(micro_brico_test_foundation
        tests/automation/test_database_config.cpp
        tests/automation/test_database_config.h
        tests/automation/tc001_login_session_foundation.cpp
    )
    target_link_libraries(micro_brico_test_foundation PRIVATE Qt6::Core Qt6::Test)

    add_test(NAME CAMP-001.TC-001.foundation
             COMMAND micro_brico_test_foundation)
    set_tests_properties(CAMP-001.TC-001.foundation PROPERTIES
        SKIP_RETURN_CODE 77)

    add_executable(micro_brico_tc001
        tests/automation/tc001_login_session.cpp
        tests/automation/test_database_config.cpp
        tests/automation/test_database_config.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        connect_db.cpp
        connect_db.h
        datamodel.cpp
        datamodel.h
        utilisateur.cpp
        utilisateur.h
        popups/popupaddusers.h popups/popupaddusers.cpp popups/popupaddusers.ui
        popups/login_connect.h popups/login_connect.cpp popups/login_connect.ui
        popups/delete_user.h popups/delete_user.cpp popups/delete_user.ui
        logger/logbrowser.h logger/logbrowser.cpp
        logger/logbrowserdialog.h logger/logbrowserdialog.cpp
        popups/poppupaddkit.h popups/poppupaddkit.cpp popups/poppupaddkit.ui
        kit.h kit.cpp
        item.h item.cpp
        money.h money.cpp
        resa.h resa.cpp
        utils.h utils.cpp
        popups/popupsortirresa.h popups/popupsortirresa.cpp popups/popupsortirresa.ui
        sortie.h sortie.cpp
        log.h log.cpp
        logger/log_user_kit_appli.h logger/log_user_kit_appli.cpp logger/log_user_kit_appli.ui
        types.h
        popups/popuppchangeitemquantities.h popups/popuppchangeitemquantities.cpp popups/popuppchangeitemquantities.ui
    )
    target_include_directories(micro_brico_tc001 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(micro_brico_tc001 PRIVATE Qt6::Widgets Qt6::Sql Qt6::Test)
    add_test(NAME CAMP-001.TC-001.login-session
             COMMAND micro_brico_tc001)
    set_tests_properties(CAMP-001.TC-001.login-session PROPERTIES
        SKIP_RETURN_CODE 77)

    # Each IHM scenario runs in its own process: the legacy MainWindow shutdown
    # path is unstable, while the test fixture still resets its named db4qtest
    # records before the process exits.
    get_target_property(MICRO_BRICO_TEST_APP_SOURCES micro_brico_tc001 SOURCES)
    list(REMOVE_ITEM MICRO_BRICO_TEST_APP_SOURCES tests/automation/tc001_login_session.cpp)
    add_executable(micro_brico_campaign_cases
        tests/automation/tc_campaign_cases.cpp
        ${MICRO_BRICO_TEST_APP_SOURCES}
    )
    target_include_directories(micro_brico_campaign_cases PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(micro_brico_campaign_cases PRIVATE Qt6::Widgets Qt6::Sql Qt6::Test)

    foreach(CAMP001_CASE
            tc002_login_invalid
            tc010_create_and_find_user
            tc011_duplicate_utinfo_rejected
            tc020_create_kit_with_initial_items
            tc021_broken_kit_not_added_to_reservation_basket
            tc040_output_requires_recipient_password
            tc042_output_with_admin_signature_keeps_recipient)
        add_test(NAME CAMP-001.${CAMP001_CASE}
                 COMMAND micro_brico_campaign_cases ${CAMP001_CASE})
        set_tests_properties(CAMP-001.${CAMP001_CASE} PROPERTIES
            SKIP_RETURN_CODE 77
            RUN_SERIAL TRUE)
    endforeach()
endif()

include(GNUInstallDirs)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.micro_brico_app)
endif()
set_target_properties(micro_brico_app PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
install(TARGETS micro_brico_app
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(micro_brico_app)
endif()
