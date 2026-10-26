cmake_minimum_required(VERSION 3.16)

set(PROJECT_VERSION "0.3.2")
set(ORGANIZATION_NAME "SpeckenbachNet")

project(
    PDFDocumentScannerM
    VERSION ${PROJECT_VERSION}
    LANGUAGES CXX
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg LinguistTools)

set(TS_FILES
    i18n/PDFDocumentScannerM_en.ts
    i18n/PDFDocumentScannerM_de.ts
)

set(PROJECT_SOURCES
        main.cpp
        mainwin.cpp
        mainwin.h
        mainwin.ui
)

set(QM_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/i18n")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PDFDocumentScannerM
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        translation.qrc
        icons.qrc
        toolbarm.h
        toolbarm.cpp
        settingsoverlay.h
        settingsoverlay.cpp

        settingsmanager.h settingsmanager.cpp
        statusoverlay.h statusoverlay.cpp
        uicolors.h
        initialize.cpp
        pagelistwidget.h pagelistwidget.cpp
        pagedelegate.h pagedelegate.cpp
        previewview.h previewview.cpp
        helppage.h helppage.cpp
        twolinedelegate.h twolinedelegate.cpp
        Help.qrc
    )

target_include_directories(PDFDocumentScannerM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(PDFDocumentScannerM
    PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
        PROJECT_NAME="${PROJECT_NAME}"
        COMPANY_NAME="${ORGANIZATION_NAME}"
)

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET PDFDocumentScannerM APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

qt_add_translations(PDFDocumentScannerM
    TS_FILES ${TS_FILES}
    QM_OUTPUT_DIRECTORY "${QM_OUTPUT_DIR}"
)
else()
    if(ANDROID)
        add_library(PDFDocumentScannerM SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(PDFDocumentScannerM
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(PDFDocumentScannerM PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg   # SVG icons (tinted toolbar icons, window icon)
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.PDFDocumentScannerM)
endif()
set_target_properties(PDFDocumentScannerM PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS PDFDocumentScannerM
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PDFDocumentScannerM)
endif()
